#include "rauc/context.h"
#include "rauc/utils.h"
#include "rauc/slot.h"
#include <string.h>

#ifdef WITH_DLT
#include <dlt/dlt.h>
DLT_DECLARE_CONTEXT(dlt_context_context);
static gboolean dlt_initialized = FALSE;
#endif

// 전역 RAUC 컨텍스트 인스턴스
RaucContext *r_context = NULL;

static void ensure_dlt_init(void) {
#ifdef WITH_DLT
    if (!dlt_initialized) {
        DLT_REGISTER_CONTEXT(dlt_context_context, "RCTX", "RAUC Context");
        dlt_initialized = TRUE;
    }
#endif
}

static void r_context_free_members(RaucContext *ctx) {
    if (!ctx) {
        return;
    }

    g_clear_pointer(&ctx->configpath, g_free);
    g_clear_pointer(&ctx->keyringpath, g_free);
    g_clear_pointer(&ctx->certpath, g_free);
    g_clear_pointer(&ctx->systeminfo_path, g_free);
    g_clear_pointer(&ctx->compatible, g_free);
    g_clear_pointer(&ctx->variant, g_free);
    g_clear_pointer(&ctx->bootslot, g_free);
    g_clear_pointer(&ctx->bootloader, g_free);
    g_clear_pointer(&ctx->grubenv_path, g_free);
    g_clear_pointer(&ctx->data_directory, g_free);
    g_clear_pointer(&ctx->logfile_path, g_free);
    g_clear_pointer(&ctx->install_info_dir, g_free);
    g_clear_pointer(&ctx->mount_prefix, g_free);

    if (ctx->config_slots) {
        g_hash_table_unref(ctx->config_slots);
        ctx->config_slots = NULL;
    }

    if (ctx->system_slots) {
        g_hash_table_unref(ctx->system_slots);
        ctx->system_slots = NULL;
    }
}

gboolean r_context_init(void) {
    ensure_dlt_init();

    if (r_context) {
        r_debug("Context already initialized");
        return TRUE;
    }

    r_context = g_new0(RaucContext, 1);

    // config 구조체 생성 및 초기화
    r_context->config = g_new0(RaucConfig, 1);
    r_context->config->system_compatible = g_strdup("intel-i7-x64-nuc-rauc");
    r_context->config->slots = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    // 기본값 설정
    r_context->configpath = g_strdup("/etc/rauc/system.conf");
    r_context->keyringpath = NULL;  // 설정에서 로드
    r_context->certpath = NULL;     // 설정에서 로드
    r_context->systeminfo_path = g_strdup("/proc/device-tree/compatible");
    r_context->compatible = NULL;   // 설정에서 로드
    r_context->variant = NULL;
    r_context->bootslot = NULL;     // 런타임에 결정
    r_context->bootloader = g_strdup("grub");  // 기본값
    r_context->grubenv_path = g_strdup("/boot/grub/grubenv");
    r_context->data_directory = g_strdup("/data");
    r_context->debug = FALSE;
    r_context->logfile_path = NULL;
    r_context->ignore_checksum = FALSE;
    r_context->force_install_same = FALSE;
    r_context->initialized = FALSE;
    r_context->install_info_dir = g_strdup("/run/rauc");
    r_context->mount_prefix = g_strdup("/tmp/rauc");

    // 해시 테이블 초기화
    r_context->config_slots = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                    g_free, r_slot_free);
    r_context->system_slots = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                    g_free, r_slot_free);

    r_info("RAUC context initialized");
    return TRUE;
}

void r_context_cleanup(void) {
    ensure_dlt_init();

    if (!r_context) {
        return;
    }

    r_context_free_members(r_context);
    g_free(r_context);
    r_context = NULL;

    r_info("RAUC context cleaned up");
}

gboolean r_context_is_initialized(void) {
    return r_context != NULL && r_context->initialized;
}

void r_context_set_config_path(const gchar *config_path) {
    if (!r_context || !config_path) {
        return;
    }

    g_free(r_context->configpath);
    r_context->configpath = g_strdup(config_path);

    r_debug("Set config path: %s", config_path);
}

void r_context_set_keyring_path(const gchar *keyring_path) {
    if (!r_context || !keyring_path) {
        return;
    }

    g_free(r_context->keyringpath);
    r_context->keyringpath = g_strdup(keyring_path);

    r_debug("Set keyring path: %s", keyring_path);
}

void r_context_set_cert_path(const gchar *cert_path) {
    if (!r_context || !cert_path) {
        return;
    }

    g_free(r_context->certpath);
    r_context->certpath = g_strdup(cert_path);

    r_debug("Set cert path: %s", cert_path);
}

void r_context_set_compatible(const gchar *compatible) {
    if (!r_context || !compatible) {
        return;
    }

    g_free(r_context->compatible);
    r_context->compatible = g_strdup(compatible);

    r_debug("Set compatible: %s", compatible);
}

void r_context_set_bootslot(const gchar *bootslot) {
    if (!r_context || !bootslot) {
        return;
    }

    g_free(r_context->bootslot);
    r_context->bootslot = g_strdup(bootslot);

    r_debug("Set bootslot: %s", bootslot);
}

void r_context_set_bootloader(const gchar *bootloader) {
    if (!r_context || !bootloader) {
        return;
    }

    g_free(r_context->bootloader);
    r_context->bootloader = g_strdup(bootloader);

    r_debug("Set bootloader: %s", bootloader);
}

void r_context_set_grubenv_path(const gchar *grubenv_path) {
    if (!r_context || !grubenv_path) {
        return;
    }

    g_free(r_context->grubenv_path);
    r_context->grubenv_path = g_strdup(grubenv_path);

    r_debug("Set grubenv path: %s", grubenv_path);
}

void r_context_set_data_directory(const gchar *data_directory) {
    if (!r_context || !data_directory) {
        return;
    }

    g_free(r_context->data_directory);
    r_context->data_directory = g_strdup(data_directory);

    r_debug("Set data directory: %s", data_directory);
}

void r_context_set_debug(gboolean debug) {
    if (!r_context) {
        return;
    }

    r_context->debug = debug;

    r_debug("Set debug mode: %s", debug ? "enabled" : "disabled");
}

void r_context_set_mount_prefix(const gchar *mount_prefix) {
    if (!r_context || !mount_prefix) {
        return;
    }

    g_free(r_context->mount_prefix);
    r_context->mount_prefix = g_strdup(mount_prefix);

    r_debug("Set mount prefix: %s", mount_prefix);
}

void r_context_add_slot(RaucSlot *slot) {
    if (!r_context || !slot || !slot->name) {
        return;
    }

    // config_slots에 추가 (설정에서 로드된 슬롯)
    g_hash_table_insert(r_context->config_slots,
                       g_strdup(slot->name),
                       slot);

    // system_slots에도 복사본 추가 (런타임 상태 관리용)
    RaucSlot *system_slot = r_slot_copy(slot);
    g_hash_table_insert(r_context->system_slots,
                       g_strdup(system_slot->name),
                       system_slot);

    r_debug("Added slot: %s", slot->name);
}

RaucSlot *r_context_get_slot_by_name(const gchar *slot_name) {
    if (!r_context || !slot_name) {
        return NULL;
    }

    return g_hash_table_lookup(r_context->system_slots, slot_name);
}

GHashTable *r_context_get_config_slots(void) {
    if (!r_context) {
        return NULL;
    }

    return r_context->config_slots;
}

GHashTable *r_context_get_system_slots(void) {
    if (!r_context) {
        return NULL;
    }

    return r_context->system_slots;
}

const gchar *r_context_get_bootslot(void) {
    if (!r_context) {
        return NULL;
    }

    return r_context->bootslot;
}

const gchar *r_context_get_compatible(void) {
    if (!r_context) {
        return NULL;
    }

    return r_context->compatible;
}

const gchar *r_context_get_bootloader(void) {
    if (!r_context) {
        return NULL;
    }

    return r_context->bootloader;
}

const gchar *r_context_get_grubenv_path(void) {
    if (!r_context) {
        return NULL;
    }

    return r_context->grubenv_path;
}

const gchar *r_context_get_data_directory(void) {
    if (!r_context) {
        return NULL;
    }

    return r_context->data_directory;
}

const gchar *r_context_get_mount_prefix(void) {
    if (!r_context) {
        return NULL;
    }

    return r_context->mount_prefix;
}

gboolean r_context_is_debug_enabled(void) {
    if (!r_context) {
        return FALSE;
    }

    return r_context->debug;
}

gchar *r_context_to_string(void) {
    if (!r_context) {
        return g_strdup("Context not initialized");
    }

    GString *str = g_string_new("RAUC Context:\n");

    g_string_append_printf(str, "  Config Path: %s\n",
                          r_context->configpath ? r_context->configpath : "null");
    g_string_append_printf(str, "  Keyring Path: %s\n",
                          r_context->keyringpath ? r_context->keyringpath : "null");
    g_string_append_printf(str, "  Cert Path: %s\n",
                          r_context->certpath ? r_context->certpath : "null");
    g_string_append_printf(str, "  Compatible: %s\n",
                          r_context->compatible ? r_context->compatible : "null");
    g_string_append_printf(str, "  Boot Slot: %s\n",
                          r_context->bootslot ? r_context->bootslot : "null");
    g_string_append_printf(str, "  Bootloader: %s\n",
                          r_context->bootloader ? r_context->bootloader : "null");
    g_string_append_printf(str, "  GRUB Env: %s\n",
                          r_context->grubenv_path ? r_context->grubenv_path : "null");
    g_string_append_printf(str, "  Data Directory: %s\n",
                          r_context->data_directory ? r_context->data_directory : "null");
    g_string_append_printf(str, "  Mount Prefix: %s\n",
                          r_context->mount_prefix ? r_context->mount_prefix : "null");
    g_string_append_printf(str, "  Debug: %s\n",
                          r_context->debug ? "enabled" : "disabled");
    g_string_append_printf(str, "  Initialized: %s\n",
                          r_context->initialized ? "yes" : "no");

    if (r_context->config_slots) {
        g_string_append_printf(str, "  Config Slots: %u\n",
                              g_hash_table_size(r_context->config_slots));
    }

    if (r_context->system_slots) {
        g_string_append_printf(str, "  System Slots: %u\n",
                              g_hash_table_size(r_context->system_slots));
    }

    return g_string_free(str, FALSE);
}
