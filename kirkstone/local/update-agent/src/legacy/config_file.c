#include "legacy/config_file.h"
#include "legacy/context.h"
#include "legacy/utils.h"
#include "legacy/slot.h"
#include <string.h>

#ifdef WITH_DLT
#include <dlt/dlt.h>
DLT_DECLARE_CONTEXT(dlt_context_config);
static gboolean dlt_initialized = FALSE;
#endif

#define R_CONFIG_ERROR r_config_error_quark()

GQuark r_config_error_quark(void) {
    return g_quark_from_static_string("r-config-error-quark");
}

static void ensure_dlt_init(void) {
#ifdef WITH_DLT
    if (!dlt_initialized) {
        DLT_REGISTER_CONTEXT(dlt_context_config, "RCFG", "RAUC Config");
        dlt_initialized = TRUE;
    }
#endif
}

// 유효한 부트로더 타입들
static const gchar *valid_bootloaders[] = {
    "grub", "uboot", "barebox", "efi", "custom", NULL
};

static gboolean is_valid_bootloader(const gchar *bootloader) {
    if (!bootloader) {
        return FALSE;
    }

    for (int i = 0; valid_bootloaders[i]; i++) {
        if (g_strcmp0(bootloader, valid_bootloaders[i]) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

gboolean load_config_file(const gchar *filename, GError **error) {
    ensure_dlt_init();

    if (!r_context) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_INVALID_FORMAT,
                   "Context not initialized");
        return FALSE;
    }

    gchar *config_path = (gchar*)filename;
    if (!config_path) {
        config_path = (gchar*)r_config_get_default_path();
    }

    if (!r_file_exists(config_path)) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_MISSING_SECTION,
                   "Config file not found: %s", config_path);
        return FALSE;
    }

    r_debug("Loading config file: %s", config_path);

    // 설정 경로 저장
    r_context_set_config_path(config_path);

    // 시스템 정보 로드
    if (!r_config_load_system_info(config_path, error)) {
        return FALSE;
    }

    // 슬롯 정보 로드
    GHashTable *slots = NULL;
    if (!r_config_load_slots(config_path, &slots, error)) {
        return FALSE;
    }

    // 컨텍스트에 슬롯들 추가
    if (slots) {
        GHashTableIter iter;
        gpointer key, value;

        g_hash_table_iter_init(&iter, slots);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            RaucSlot *slot = (RaucSlot *)value;
            if (slot) {
                r_context_add_slot(slot);
            }
        }

        g_hash_table_unref(slots);
    }

    // 부모-자식 관계 설정
    GHashTable *system_slots = r_context_get_system_slots();
    if (system_slots && !r_config_setup_slot_parents(system_slots, error)) {
        return FALSE;
    }

    // 설정 유효성 검증
    if (!r_config_validate(error)) {
        return FALSE;
    }

    r_context->initialized = TRUE;
    r_info("Config file loaded successfully: %s", config_path);
    return TRUE;
}

gboolean r_config_load_slots(const gchar *filename, GHashTable **slots, GError **error) {
    ensure_dlt_init();

    if (!filename || !slots) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_INVALID_FORMAT,
                   "Invalid filename or slots parameter");
        return FALSE;
    }

    GKeyFile *key_file = g_key_file_new();
    if (!g_key_file_load_from_file(key_file, filename, G_KEY_FILE_NONE, error)) {
        g_key_file_free(key_file);
        return FALSE;
    }

    *slots = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, r_slot_free);

    // 모든 그룹 가져오기
    gchar **groups = g_key_file_get_groups(key_file, NULL);

    for (int i = 0; groups[i]; i++) {
        const gchar *group = groups[i];

        // 슬롯 그룹인지 확인 ("slot." 접두사)
        if (!g_str_has_prefix(group, "slot.")) {
            continue;
        }

        // 슬롯 이름 추출
        const gchar *slot_name = group + 5;  // "slot." 이후

        if (strlen(slot_name) == 0) {
            r_warn("Empty slot name in group: %s", group);
            continue;
        }

        r_debug("Parsing slot: %s", slot_name);

        RaucSlot *slot = r_config_parse_slot(key_file, group, slot_name, error);
        if (!slot) {
            g_strfreev(groups);
            g_key_file_free(key_file);
            g_hash_table_unref(*slots);
            *slots = NULL;
            return FALSE;
        }

        g_hash_table_insert(*slots, g_strdup(slot_name), slot);
    }

    g_strfreev(groups);
    g_key_file_free(key_file);

    r_debug("Loaded %u slots from config", g_hash_table_size(*slots));
    return TRUE;
}

gboolean r_config_load_system_info(const gchar *filename, GError **error) {
    ensure_dlt_init();

    if (!filename) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_INVALID_FORMAT,
                   "Invalid filename");
        return FALSE;
    }

    GKeyFile *key_file = g_key_file_new();
    if (!g_key_file_load_from_file(key_file, filename, G_KEY_FILE_NONE, error)) {
        g_key_file_free(key_file);
        return FALSE;
    }

    // [system] 섹션 확인
    if (!g_key_file_has_group(key_file, "system")) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_MISSING_SECTION,
                   "Missing [system] section");
        g_key_file_free(key_file);
        return FALSE;
    }

    // 필수 필드들 로드
    gchar *compatible = g_key_file_get_string(key_file, "system", "compatible", NULL);
    if (!compatible) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_MISSING_KEY,
                   "Missing 'compatible' key in [system] section");
        g_key_file_free(key_file);
        return FALSE;
    }
    r_context_set_compatible(compatible);
    g_free(compatible);

    // 선택적 필드들 로드
    gchar *bootloader = g_key_file_get_string(key_file, "system", "bootloader", NULL);
    if (bootloader) {
        if (!is_valid_bootloader(bootloader)) {
            g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_BOOTLOADER_INVALID,
                       "Invalid bootloader: %s", bootloader);
            g_free(bootloader);
            g_key_file_free(key_file);
            return FALSE;
        }
        r_context_set_bootloader(bootloader);
        g_free(bootloader);
    }

    gchar *grubenv = g_key_file_get_string(key_file, "system", "grubenv", NULL);
    if (grubenv) {
        r_context_set_grubenv_path(grubenv);
        g_free(grubenv);
    }

    gchar *data_directory = g_key_file_get_string(key_file, "system", "data-directory", NULL);
    if (data_directory) {
        r_context_set_data_directory(data_directory);
        g_free(data_directory);
    }

    gchar *mount_prefix = g_key_file_get_string(key_file, "system", "mount-prefix", NULL);
    if (mount_prefix) {
        r_context_set_mount_prefix(mount_prefix);
        g_free(mount_prefix);
    }

    // [keyring] 섹션 처리
    if (g_key_file_has_group(key_file, "keyring")) {
        gchar *keyring_path = g_key_file_get_string(key_file, "keyring", "path", NULL);
        if (keyring_path) {
            r_context_set_keyring_path(keyring_path);
            g_free(keyring_path);
        }

        gchar *cert_path = g_key_file_get_string(key_file, "keyring", "cert-path", NULL);
        if (cert_path) {
            r_context_set_cert_path(cert_path);
            g_free(cert_path);
        }
    }

    g_key_file_free(key_file);
    return TRUE;
}

RaucSlot *r_config_parse_slot(GKeyFile *key_file, const gchar *group_name, const gchar *slot_name, GError **error) {
    ensure_dlt_init();

    if (!key_file || !group_name || !slot_name) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_INVALID_FORMAT,
                   "Invalid parameters");
        return NULL;
    }

    RaucSlot *slot = r_slot_new(slot_name);

    // 필수 필드들
    gchar *device = g_key_file_get_string(key_file, group_name, "device", NULL);
    if (!device) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_MISSING_KEY,
                   "Missing 'device' key for slot %s", slot_name);
        r_slot_free(slot);
        return NULL;
    }
    slot->device = device;

    gchar *type = g_key_file_get_string(key_file, group_name, "type", NULL);
    if (type) {
        if (!r_slot_is_valid_type(type)) {
            g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_SLOT_INVALID,
                       "Invalid slot type '%s' for slot %s", type, slot_name);
            g_free(type);
            r_slot_free(slot);
            return NULL;
        }
        slot->type = type;
    } else {
        slot->type = g_strdup("ext4");  // 기본값
    }

    // 선택적 필드들
    slot->description = g_key_file_get_string(key_file, group_name, "description", NULL);
    slot->bootname = g_key_file_get_string(key_file, group_name, "bootname", NULL);
    slot->extra_mount_opts = g_key_file_get_string(key_file, group_name, "extra-mount-opts", NULL);
    slot->parent_name = g_key_file_get_string(key_file, group_name, "parent", NULL);

    // 부울 값들
    slot->allow_mounted = g_key_file_get_boolean(key_file, group_name, "allow-mounted", NULL);
    slot->readonly = g_key_file_get_boolean(key_file, group_name, "readonly", NULL);
    slot->install_same = g_key_file_get_boolean(key_file, group_name, "install-same", NULL);
    slot->resize = g_key_file_get_boolean(key_file, group_name, "resize", NULL);

    // 정수 값들
    slot->region_start = g_key_file_get_uint64(key_file, group_name, "region-start", NULL);
    slot->region_size = g_key_file_get_uint64(key_file, group_name, "region-size", NULL);

    // 슬롯 클래스 추출 (슬롯 이름에서)
    gchar **name_parts = g_strsplit(slot_name, ".", -1);
    if (name_parts && name_parts[0]) {
        slot->sclass = g_intern_string(name_parts[0]);
    }
    g_strfreev(name_parts);

    r_debug("Parsed slot %s: device=%s, type=%s, class=%s",
           slot_name, slot->device, slot->type, slot->sclass ? slot->sclass : "(none)");

    return slot;
}

gboolean r_config_setup_slot_parents(GHashTable *slots, GError **error) {
    ensure_dlt_init();

    if (!slots) {
        return TRUE;
    }

    GHashTableIter iter;
    gpointer key, value;

    // 첫 번째 패스: 부모 관계 설정
    g_hash_table_iter_init(&iter, slots);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucSlot *slot = (RaucSlot *)value;

        if (!slot || !slot->parent_name) {
            continue;
        }

        RaucSlot *parent = g_hash_table_lookup(slots, slot->parent_name);
        if (!parent) {
            g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_SLOT_INVALID,
                       "Parent slot '%s' not found for slot '%s'",
                       slot->parent_name, slot->name);
            return FALSE;
        }

        slot->parent = parent;
        r_debug("Set parent relationship: %s -> %s", slot->name, parent->name);
    }

    // 두 번째 패스: 순환 참조 검사
    g_hash_table_iter_init(&iter, slots);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucSlot *slot = (RaucSlot *)value;

        if (!slot->parent) {
            continue;
        }

        // 순환 참조 검사
        RaucSlot *current = slot->parent;
        int depth = 0;
        const int max_depth = 10;  // 무한 루프 방지

        while (current && depth < max_depth) {
            if (current == slot) {
                g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_SLOT_INVALID,
                           "Circular parent relationship detected for slot '%s'", slot->name);
                return FALSE;
            }
            current = current->parent;
            depth++;
        }

        if (depth >= max_depth) {
            g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_SLOT_INVALID,
                       "Parent chain too deep for slot '%s'", slot->name);
            return FALSE;
        }
    }

    return TRUE;
}

gboolean r_config_validate(GError **error) {
    ensure_dlt_init();

    if (!r_context) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_INVALID_FORMAT,
                   "Context not initialized");
        return FALSE;
    }

    // 호환성 문자열 필수
    if (!r_context->compatible) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_MISSING_KEY,
                   "System compatible string is required");
        return FALSE;
    }

    // 부트로더 검증
    if (r_context->bootloader && !is_valid_bootloader(r_context->bootloader)) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_BOOTLOADER_INVALID,
                   "Invalid bootloader: %s", r_context->bootloader);
        return FALSE;
    }

    // 슬롯 검증
    GHashTable *slots = r_context_get_config_slots();
    if (!slots || g_hash_table_size(slots) == 0) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_MISSING_SECTION,
                   "No slots defined in configuration");
        return FALSE;
    }

    // 각 슬롯 클래스에 대해 최소 하나의 슬롯이 있는지 확인
    gchar **root_classes = r_slot_get_root_classes(slots);
    if (!root_classes || !root_classes[0]) {
        g_set_error(error, R_CONFIG_ERROR, R_CONFIG_ERROR_SLOT_INVALID,
                   "No root slot classes found");
        g_strfreev(root_classes);
        return FALSE;
    }

    r_debug("Config validation passed. Root classes: %u", g_strv_length(root_classes));
    g_strfreev(root_classes);

    return TRUE;
}

const gchar *r_config_get_default_path(void) {
    return "/etc/rauc/system.conf";
}

const gchar *r_config_get_keyring_path(void) {
    return r_context ? r_context->keyringpath : NULL;
}

const gchar *r_config_get_cert_path(void) {
    return r_context ? r_context->certpath : NULL;
}

const gchar *r_config_get_bootloader(void) {
    return r_context ? r_context->bootloader : NULL;
}

const gchar *r_config_get_grubenv_path(void) {
    return r_context ? r_context->grubenv_path : NULL;
}

const gchar *r_config_get_data_directory(void) {
    return r_context ? r_context->data_directory : "/data";
}

const gchar *r_config_get_mount_prefix(void) {
    return r_context ? r_context->mount_prefix : "/tmp/rauc";
}

gboolean r_config_is_loaded(void) {
    return r_context && r_context->initialized;
}

void r_config_set_path_for_test(const gchar *path) {
    if (r_context) {
        r_context_set_config_path(path);
    }
}

gchar *r_config_to_string(void) {
    if (!r_context) {
        return g_strdup("Config not loaded");
    }

    GString *str = g_string_new("RAUC Configuration:\n");

    g_string_append_printf(str, "  Config Path: %s\n",
                          r_context->configpath ? r_context->configpath : "(none)");
    g_string_append_printf(str, "  Compatible: %s\n",
                          r_context->compatible ? r_context->compatible : "(none)");
    g_string_append_printf(str, "  Bootloader: %s\n",
                          r_context->bootloader ? r_context->bootloader : "(none)");
    g_string_append_printf(str, "  Keyring: %s\n",
                          r_context->keyringpath ? r_context->keyringpath : "(none)");
    g_string_append_printf(str, "  Certificate: %s\n",
                          r_context->certpath ? r_context->certpath : "(none)");
    g_string_append_printf(str, "  Data Directory: %s\n",
                          r_context->data_directory ? r_context->data_directory : "(none)");
    g_string_append_printf(str, "  Mount Prefix: %s\n",
                          r_context->mount_prefix ? r_context->mount_prefix : "(none)");

    GHashTable *slots = r_context_get_config_slots();
    if (slots) {
        g_string_append_printf(str, "  Slots: %u\n", g_hash_table_size(slots));

        GHashTableIter iter;
        gpointer key, value;
        g_hash_table_iter_init(&iter, slots);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            RaucSlot *slot = (RaucSlot *)value;
            g_string_append_printf(str, "    %s: %s (%s)\n",
                                  (gchar*)key,
                                  slot->device ? slot->device : "(no device)",
                                  slot->type ? slot->type : "(no type)");
        }
    }

    return g_string_free(str, FALSE);
}
