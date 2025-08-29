#include "rauc/slot.h"
#include "rauc/utils.h"
#include "rauc/context.h"
#include "rauc/checksum.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <mntent.h>
#include <errno.h>

#ifdef WITH_DLT
#include <dlt/dlt.h>
DLT_DECLARE_CONTEXT(dlt_context_slot);
static gboolean dlt_initialized = FALSE;
#endif

#define R_SLOT_ERROR r_slot_error_quark()

GQuark r_slot_error_quark(void) {
    return g_quark_from_static_string("r-slot-error-quark");
}

static void ensure_dlt_init(void) {
#ifdef WITH_DLT
    if (!dlt_initialized) {
        DLT_REGISTER_CONTEXT(dlt_context_slot, "RSLOT", "RAUC Slot");
        dlt_initialized = TRUE;
    }
#endif
}

// 유효한 슬롯 타입들
static const gchar *valid_slot_types[] = {
    "ext2", "ext3", "ext4", "btrfs", "squashfs", "ubifs", "jffs2",
    "raw", "nand", "nor", "boot-emmc", "boot-gpt-switch", "boot-mbr-switch",
    "boot-raw-fallback", NULL
};

void r_slot_free(gpointer value) {
    RaucSlot *slot = (RaucSlot *)value;
    if (!slot) {
        return;
    }

    g_free(slot->description);
    g_free(slot->device);
    g_free(slot->type);
    g_strfreev(slot->extra_mkfs_opts);
    g_free(slot->bootname);
    g_free(slot->extra_mount_opts);
    g_free(slot->parent_name);
    g_free(slot->mount_point);
    g_free(slot->ext_mount_point);
    g_free(slot->data_directory);

    if (slot->status) {
        r_slot_free_status(slot->status);
    }

    g_free(slot);
}

void r_slot_clear_status(RaucSlotStatus *slotstatus) {
    if (!slotstatus) {
        return;
    }

    g_clear_pointer(&slotstatus->bundle_compatible, g_free);
    g_clear_pointer(&slotstatus->bundle_version, g_free);
    g_clear_pointer(&slotstatus->bundle_description, g_free);
    g_clear_pointer(&slotstatus->bundle_build, g_free);
    g_clear_pointer(&slotstatus->bundle_hash, g_free);
    g_clear_pointer(&slotstatus->status, g_free);
    g_clear_pointer(&slotstatus->installed_txn, g_free);
    g_clear_pointer(&slotstatus->installed_timestamp, g_free);
    g_clear_pointer(&slotstatus->activated_timestamp, g_free);

    r_checksum_clear(&slotstatus->checksum);

    slotstatus->installed_count = 0;
    slotstatus->activated_count = 0;
}

void r_slot_free_status(RaucSlotStatus *slotstatus) {
    if (!slotstatus) {
        return;
    }

    r_slot_clear_status(slotstatus);
    g_free(slotstatus);
}

RaucSlot *r_slot_find_by_device(GHashTable *slots, const gchar *device) {
    ensure_dlt_init();

    if (!slots || !device) {
        return NULL;
    }

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, slots);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucSlot *slot = (RaucSlot *)value;
        if (slot && slot->device && g_strcmp0(slot->device, device) == 0) {
            r_debug("Found slot by device: %s -> %s", device, slot->name);
            return slot;
        }
    }

    r_debug("No slot found for device: %s", device);
    return NULL;
}

RaucSlot *r_slot_get_booted(GHashTable *slots) {
    ensure_dlt_init();

    if (!slots) {
        return NULL;
    }

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, slots);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucSlot *slot = (RaucSlot *)value;
        if (slot && (slot->state & ST_BOOTED)) {
            r_debug("Found booted slot: %s", slot->name);
            return slot;
        }
    }

    r_debug("No booted slot found");
    return NULL;
}

const gchar *r_slot_slotstate_to_str(SlotState slotstate) {
    switch (slotstate) {
        case ST_BOOTED:
            return "booted";
        case ST_ACTIVE:
            return "active";
        case ST_INACTIVE:
            return "inactive";
        case ST_UNKNOWN:
        default:
            return "unknown";
    }
}

SlotState r_slot_str_to_slotstate(gchar *str) {
    if (!str) {
        return ST_UNKNOWN;
    }

    if (g_strcmp0(str, "booted") == 0) {
        return ST_BOOTED;
    } else if (g_strcmp0(str, "active") == 0) {
        return ST_ACTIVE;
    } else if (g_strcmp0(str, "inactive") == 0) {
        return ST_INACTIVE;
    }

    return ST_UNKNOWN;
}

gboolean r_slot_is_valid_type(const gchar *type) {
    if (!type) {
        return FALSE;
    }

    for (int i = 0; valid_slot_types[i]; i++) {
        if (g_strcmp0(type, valid_slot_types[i]) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

gboolean r_slot_is_mountable(RaucSlot *slot) {
    if (!slot || !slot->type) {
        return FALSE;
    }

    // Raw 타입은 마운트 불가
    if (g_strcmp0(slot->type, "raw") == 0 ||
        g_str_has_prefix(slot->type, "boot-") ||
        g_str_has_prefix(slot->type, "nand") ||
        g_str_has_prefix(slot->type, "nor")) {
        return FALSE;
    }

    return TRUE;
}

RaucSlot *r_slot_get_parent_root(RaucSlot *slot) {
    if (!slot) {
        return NULL;
    }

    RaucSlot *current = slot;
    while (current->parent) {
        current = current->parent;
    }

    return current;
}

gboolean r_slot_move_checksum_data_directory(const RaucSlot *slot,
                                            const gchar *old_digest,
                                            const gchar *new_digest,
                                            GError **error) {
    ensure_dlt_init();

    if (!slot) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_INVALID_NAME,
                   "Invalid slot");
        return FALSE;
    }

    const gchar *data_dir = r_context_get_data_directory();
    if (!data_dir) {
        r_debug("No data directory configured");
        return TRUE;
    }

    if (!old_digest) {
        r_debug("No old digest, nothing to move");
        return TRUE;
    }

    gchar *old_path = g_build_filename(data_dir, slot->data_directory,
                                      g_strdup_printf("hash-%s", old_digest), NULL);

    if (!r_directory_exists(old_path)) {
        g_free(old_path);
        r_debug("Old data directory doesn't exist");
        return TRUE;
    }

    if (!new_digest) {
        // 삭제만 수행
        gboolean result = r_remove_tree(old_path, error);
        g_free(old_path);
        return result;
    }

    gchar *new_path = g_build_filename(data_dir, slot->data_directory,
                                      g_strdup_printf("hash-%s", new_digest), NULL);

    // 새 디렉토리가 이미 있으면 삭제
    if (r_directory_exists(new_path)) {
        if (!r_remove_tree(new_path, error)) {
            g_free(old_path);
            g_free(new_path);
            return FALSE;
        }
    }

    // 부모 디렉토리 생성
    gchar *parent_dir = g_path_get_dirname(new_path);
    if (!r_mkdir_parents(parent_dir, error)) {
        g_free(old_path);
        g_free(new_path);
        g_free(parent_dir);
        return FALSE;
    }
    g_free(parent_dir);

    // 디렉토리 이동
    if (rename(old_path, new_path) != 0) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_STATUS_SAVE_FAILED,
                   "Failed to move data directory from %s to %s: %s",
                   old_path, new_path, g_strerror(errno));
        g_free(old_path);
        g_free(new_path);
        return FALSE;
    }

    r_debug("Moved slot data directory from %s to %s", old_path, new_path);

    g_free(old_path);
    g_free(new_path);
    return TRUE;
}

gchar *r_slot_get_checksum_data_directory(const RaucSlot *slot,
                                         const RaucChecksum *checksum,
                                         GError **error) {
    ensure_dlt_init();

    if (!slot) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_INVALID_NAME,
                   "Invalid slot");
        return NULL;
    }

    const gchar *data_dir = r_context_get_data_directory();
    if (!data_dir) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_STATUS_LOAD_FAILED,
                   "No data directory configured");
        return NULL;
    }

    const RaucChecksum *target_checksum = checksum;
    if (!target_checksum && slot->status) {
        target_checksum = &slot->status->checksum;
    }

    gchar *digest_str = "unknown";
    if (target_checksum && r_checksum_is_set(target_checksum)) {
        digest_str = target_checksum->digest;
    }

    gchar *checksum_dir = g_build_filename(data_dir, slot->data_directory,
                                          g_strdup_printf("hash-%s", digest_str), NULL);

    // 디렉토리가 없으면 생성
    if (!r_directory_exists(checksum_dir)) {
        if (!r_mkdir_parents(checksum_dir, error)) {
            g_free(checksum_dir);
            return NULL;
        }
        r_debug("Created checksum data directory: %s", checksum_dir);
    }

    return checksum_dir;
}

void r_slot_clean_data_directory(const RaucSlot *slot) {
    ensure_dlt_init();

    if (!slot) {
        return;
    }

    const gchar *data_dir = r_context_get_data_directory();
    if (!data_dir || !slot->data_directory) {
        return;
    }

    gchar *slot_data_dir = g_build_filename(data_dir, slot->data_directory, NULL);

    if (!r_directory_exists(slot_data_dir)) {
        g_free(slot_data_dir);
        return;
    }

    GDir *dir = g_dir_open(slot_data_dir, 0, NULL);
    if (!dir) {
        g_free(slot_data_dir);
        return;
    }

    const gchar *current_digest = NULL;
    if (slot->status && r_checksum_is_set(&slot->status->checksum)) {
        current_digest = slot->status->checksum.digest;
    }

    const gchar *entry;
    while ((entry = g_dir_read_name(dir)) != NULL) {
        if (!g_str_has_prefix(entry, "hash-")) {
            continue;
        }

        const gchar *entry_digest = entry + 5;  // "hash-" 이후

        // 현재 체크섬과 같으면 건너뛰기
        if (current_digest && g_strcmp0(entry_digest, current_digest) == 0) {
            continue;
        }

        gchar *entry_path = g_build_filename(slot_data_dir, entry, NULL);
        r_remove_tree(entry_path, NULL);  // 오류 무시
        r_debug("Cleaned obsolete data directory: %s", entry_path);
        g_free(entry_path);
    }

    g_dir_close(dir);
    g_free(slot_data_dir);
}

gchar **r_slot_get_root_classes(GHashTable *slots) {
    if (!slots) {
        return NULL;
    }

    GHashTable *classes = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, slots);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucSlot *slot = (RaucSlot *)value;
        if (slot && slot->sclass && !slot->parent) {
            g_hash_table_insert(classes, (gpointer)slot->sclass, GINT_TO_POINTER(1));
        }
    }

    guint num_classes = g_hash_table_size(classes);
    gchar **class_array = g_new0(gchar*, num_classes + 1);

    g_hash_table_iter_init(&iter, classes);
    int i = 0;
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        class_array[i++] = g_strdup((const gchar*)key);
    }

    g_hash_table_unref(classes);
    return class_array;
}

gboolean r_slot_list_contains(GList *slotlist, const RaucSlot *testslot) {
    if (!slotlist || !testslot) {
        return FALSE;
    }

    return g_list_find(slotlist, testslot) != NULL;
}

GList *r_slot_get_all_of_class(GHashTable *slots, const gchar *class) {
    if (!slots || !class) {
        return NULL;
    }

    GList *result = NULL;
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, slots);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucSlot *slot = (RaucSlot *)value;
        if (slot && slot->sclass && g_strcmp0(slot->sclass, class) == 0) {
            result = g_list_prepend(result, slot);
        }
    }

    return g_list_reverse(result);
}

GList *r_slot_get_all_children(GHashTable *slots, RaucSlot *parent) {
    if (!slots || !parent) {
        return NULL;
    }

    GList *result = NULL;
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, slots);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucSlot *slot = (RaucSlot *)value;
        if (slot && slot->parent == parent) {
            result = g_list_prepend(result, slot);
        }
    }

    return g_list_reverse(result);
}

RaucSlot *r_slot_new(const gchar *name) {
    if (!name) {
        return NULL;
    }

    RaucSlot *slot = g_new0(RaucSlot, 1);
    slot->name = g_intern_string(name);
    slot->description = NULL;
    slot->sclass = NULL;
    slot->device = NULL;
    slot->type = NULL;
    slot->extra_mkfs_opts = NULL;
    slot->bootname = NULL;
    slot->allow_mounted = FALSE;
    slot->readonly = FALSE;
    slot->install_same = FALSE;
    slot->extra_mount_opts = NULL;
    slot->resize = FALSE;
    slot->region_start = 0;
    slot->region_size = 0;
    slot->state = ST_UNKNOWN;
    slot->boot_good = FALSE;
    slot->parent = NULL;
    slot->parent_name = NULL;
    slot->mount_point = NULL;
    slot->ext_mount_point = NULL;
    slot->status = NULL;
    slot->data_directory = g_strdup(name);

    return slot;
}

RaucSlot *r_slot_copy(const RaucSlot *slot) {
    if (!slot) {
        return NULL;
    }

    RaucSlot *copy = r_slot_new(slot->name);

    copy->description = g_strdup(slot->description);
    copy->sclass = slot->sclass;  // intern string
    copy->device = g_strdup(slot->device);
    copy->type = g_strdup(slot->type);
    copy->extra_mkfs_opts = g_strdupv(slot->extra_mkfs_opts);
    copy->bootname = g_strdup(slot->bootname);
    copy->allow_mounted = slot->allow_mounted;
    copy->readonly = slot->readonly;
    copy->install_same = slot->install_same;
    copy->extra_mount_opts = g_strdup(slot->extra_mount_opts);
    copy->resize = slot->resize;
    copy->region_start = slot->region_start;
    copy->region_size = slot->region_size;
    copy->state = slot->state;
    copy->boot_good = slot->boot_good;
    copy->parent = slot->parent;  // 포인터 복사 (참조)
    copy->parent_name = g_strdup(slot->parent_name);
    copy->mount_point = g_strdup(slot->mount_point);
    copy->ext_mount_point = g_strdup(slot->ext_mount_point);
    copy->data_directory = g_strdup(slot->data_directory);

    // 상태 정보 복사
    if (slot->status) {
        copy->status = g_new0(RaucSlotStatus, 1);
        copy->status->bundle_compatible = g_strdup(slot->status->bundle_compatible);
        copy->status->bundle_version = g_strdup(slot->status->bundle_version);
        copy->status->bundle_description = g_strdup(slot->status->bundle_description);
        copy->status->bundle_build = g_strdup(slot->status->bundle_build);
        copy->status->bundle_hash = g_strdup(slot->status->bundle_hash);
        copy->status->status = g_strdup(slot->status->status);
        copy->status->checksum = *r_checksum_copy(&slot->status->checksum);
        copy->status->installed_txn = g_strdup(slot->status->installed_txn);
        copy->status->installed_timestamp = g_strdup(slot->status->installed_timestamp);
        copy->status->installed_count = slot->status->installed_count;
        copy->status->activated_timestamp = g_strdup(slot->status->activated_timestamp);
        copy->status->activated_count = slot->status->activated_count;
    }

    return copy;
}

gboolean r_slot_load_status(RaucSlot *slot, GError **error) {
    ensure_dlt_init();

    if (!slot) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_INVALID_NAME,
                   "Invalid slot");
        return FALSE;
    }

    // 상태 파일 경로 생성
    const gchar *data_dir = r_context_get_data_directory();
    if (!data_dir) {
        r_debug("No data directory configured for slot %s", slot->name);
        return TRUE;  // 오류가 아님
    }

    gchar *status_file = g_build_filename(data_dir, slot->data_directory, "status", NULL);

    if (!r_file_exists(status_file)) {
        g_free(status_file);
        r_debug("No status file for slot %s", slot->name);
        return TRUE;  // 오류가 아님
    }

    // 기존 상태 정리
    if (slot->status) {
        r_slot_free_status(slot->status);
    }
    slot->status = g_new0(RaucSlotStatus, 1);

    // 상태 파일 로드
    GKeyFile *key_file = g_key_file_new();
    if (!g_key_file_load_from_file(key_file, status_file, G_KEY_FILE_NONE, error)) {
        g_key_file_free(key_file);
        g_free(status_file);
        return FALSE;
    }

    // 상태 정보 파싱
    slot->status->bundle_compatible = g_key_file_get_string(key_file, "slot", "bundle.compatible", NULL);
    slot->status->bundle_version = g_key_file_get_string(key_file, "slot", "bundle.version", NULL);
    slot->status->bundle_description = g_key_file_get_string(key_file, "slot", "bundle.description", NULL);
    slot->status->bundle_build = g_key_file_get_string(key_file, "slot", "bundle.build", NULL);
    slot->status->bundle_hash = g_key_file_get_string(key_file, "slot", "bundle.hash", NULL);
    slot->status->status = g_key_file_get_string(key_file, "slot", "status", NULL);
    slot->status->installed_txn = g_key_file_get_string(key_file, "slot", "installed.transaction", NULL);
    slot->status->installed_timestamp = g_key_file_get_string(key_file, "slot", "installed.timestamp", NULL);
    slot->status->installed_count = g_key_file_get_integer(key_file, "slot", "installed.count", NULL);
    slot->status->activated_timestamp = g_key_file_get_string(key_file, "slot", "activated.timestamp", NULL);
    slot->status->activated_count = g_key_file_get_integer(key_file, "slot", "activated.count", NULL);

    // 체크섬 정보 로드
    gchar *checksum_str = g_key_file_get_string(key_file, "slot", "sha256", NULL);
    if (checksum_str) {
        slot->status->checksum.type = R_CHECKSUM_SHA256;
        slot->status->checksum.digest = checksum_str;
    }

    g_key_file_free(key_file);
    g_free(status_file);

    r_debug("Loaded status for slot %s", slot->name);
    return TRUE;
}

gboolean r_slot_save_status(RaucSlot *slot, GError **error) {
    ensure_dlt_init();

    if (!slot || !slot->status) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_INVALID_NAME,
                   "Invalid slot or no status");
        return FALSE;
    }

    const gchar *data_dir = r_context_get_data_directory();
    if (!data_dir) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_STATUS_SAVE_FAILED,
                   "No data directory configured");
        return FALSE;
    }

    gchar *slot_dir = g_build_filename(data_dir, slot->data_directory, NULL);
    if (!r_mkdir_parents(slot_dir, error)) {
        g_free(slot_dir);
        return FALSE;
    }

    gchar *status_file = g_build_filename(slot_dir, "status", NULL);
    g_free(slot_dir);

    // 상태 파일 생성
    GKeyFile *key_file = g_key_file_new();

    if (slot->status->bundle_compatible) {
        g_key_file_set_string(key_file, "slot", "bundle.compatible", slot->status->bundle_compatible);
    }
    if (slot->status->bundle_version) {
        g_key_file_set_string(key_file, "slot", "bundle.version", slot->status->bundle_version);
    }
    if (slot->status->bundle_description) {
        g_key_file_set_string(key_file, "slot", "bundle.description", slot->status->bundle_description);
    }
    if (slot->status->bundle_build) {
        g_key_file_set_string(key_file, "slot", "bundle.build", slot->status->bundle_build);
    }
    if (slot->status->bundle_hash) {
        g_key_file_set_string(key_file, "slot", "bundle.hash", slot->status->bundle_hash);
    }
    if (slot->status->status) {
        g_key_file_set_string(key_file, "slot", "status", slot->status->status);
    }
    if (slot->status->installed_txn) {
        g_key_file_set_string(key_file, "slot", "installed.transaction", slot->status->installed_txn);
    }
    if (slot->status->installed_timestamp) {
        g_key_file_set_string(key_file, "slot", "installed.timestamp", slot->status->installed_timestamp);
    }
    g_key_file_set_integer(key_file, "slot", "installed.count", slot->status->installed_count);
    if (slot->status->activated_timestamp) {
        g_key_file_set_string(key_file, "slot", "activated.timestamp", slot->status->activated_timestamp);
    }
    g_key_file_set_integer(key_file, "slot", "activated.count", slot->status->activated_count);

    // 체크섬 저장
    if (r_checksum_is_set(&slot->status->checksum)) {
        g_key_file_set_string(key_file, "slot", "sha256", slot->status->checksum.digest);
    }

    // 파일 저장
    gchar *key_file_data = g_key_file_to_data(key_file, NULL, NULL);
    gboolean result = write_file_str(status_file, key_file_data, error);

    g_free(key_file_data);
    g_key_file_free(key_file);
    g_free(status_file);

    if (result) {
        r_debug("Saved status for slot %s", slot->name);
    }

    return result;
}

gboolean r_slot_is_mounted(RaucSlot *slot) {
    if (!slot || !slot->device) {
        return FALSE;
    }

    FILE *mounts = setmntent("/proc/mounts", "r");
    if (!mounts) {
        return FALSE;
    }

    struct mntent *entry;
    gboolean mounted = FALSE;

    while ((entry = getmntent(mounts)) != NULL) {
        if (g_strcmp0(entry->mnt_fsname, slot->device) == 0) {
            mounted = TRUE;
            break;
        }
    }

    endmntent(mounts);
    return mounted;
}

gboolean r_slot_mount(RaucSlot *slot, GError **error) {
    ensure_dlt_init();

    if (!slot || !slot->device) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_INVALID_NAME,
                   "Invalid slot or no device");
        return FALSE;
    }

    if (!r_slot_is_mountable(slot)) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_NOT_MOUNTABLE,
                   "Slot %s is not mountable", slot->name);
        return FALSE;
    }

    if (r_slot_is_mounted(slot)) {
        r_debug("Slot %s is already mounted", slot->name);
        return TRUE;
    }

    // 마운트 포인트 생성
    if (!slot->mount_point) {
        const gchar *mount_prefix = r_context_get_mount_prefix();
        slot->mount_point = g_build_filename(mount_prefix, slot->name, NULL);
    }

    if (!r_mkdir_parents(slot->mount_point, error)) {
        return FALSE;
    }

    // 마운트 수행
    gchar *mount_args[] = {"mount", slot->device, slot->mount_point, NULL};
    gint exit_status;

    if (!r_subprocess_run(mount_args, NULL, NULL, &exit_status, error)) {
        return FALSE;
    }

    if (exit_status != 0) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_MOUNT_FAILED,
                   "Mount failed with exit status %d", exit_status);
        return FALSE;
    }

    r_info("Mounted slot %s at %s", slot->name, slot->mount_point);
    return TRUE;
}

gboolean r_slot_unmount(RaucSlot *slot, GError **error) {
    ensure_dlt_init();

    if (!slot || !slot->mount_point) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_INVALID_NAME,
                   "Invalid slot or no mount point");
        return FALSE;
    }

    if (!r_slot_is_mounted(slot)) {
        r_debug("Slot %s is not mounted", slot->name);
        return TRUE;
    }

    // 언마운트 수행
    gchar *umount_args[] = {"umount", slot->mount_point, NULL};
    gint exit_status;

    if (!r_subprocess_run(umount_args, NULL, NULL, &exit_status, error)) {
        return FALSE;
    }

    if (exit_status != 0) {
        g_set_error(error, R_SLOT_ERROR, R_SLOT_ERROR_UNMOUNT_FAILED,
                   "Unmount failed with exit status %d", exit_status);
        return FALSE;
    }

    r_info("Unmounted slot %s from %s", slot->name, slot->mount_point);
    return TRUE;
}

gchar *r_slot_to_string(const RaucSlot *slot) {
    if (!slot) {
        return g_strdup("(null)");
    }

    GString *str = g_string_new("");

    g_string_append_printf(str, "Slot '%s':\n", slot->name);
    g_string_append_printf(str, "  Class: %s\n", slot->sclass ? slot->sclass : "(none)");
    g_string_append_printf(str, "  Device: %s\n", slot->device ? slot->device : "(none)");
    g_string_append_printf(str, "  Type: %s\n", slot->type ? slot->type : "(none)");
    g_string_append_printf(str, "  Bootname: %s\n", slot->bootname ? slot->bootname : "(none)");
    g_string_append_printf(str, "  State: %s\n", r_slot_slotstate_to_str(slot->state));
    g_string_append_printf(str, "  Boot Good: %s\n", slot->boot_good ? "yes" : "no");
    g_string_append_printf(str, "  Allow Mounted: %s\n", slot->allow_mounted ? "yes" : "no");
    g_string_append_printf(str, "  Read Only: %s\n", slot->readonly ? "yes" : "no");
    g_string_append_printf(str, "  Mount Point: %s\n", slot->mount_point ? slot->mount_point : "(none)");

    if (slot->status) {
        g_string_append_printf(str, "  Bundle Compatible: %s\n",
                              slot->status->bundle_compatible ? slot->status->bundle_compatible : "(none)");
        g_string_append_printf(str, "  Bundle Version: %s\n",
                              slot->status->bundle_version ? slot->status->bundle_version : "(none)");
        g_string_append_printf(str, "  Installed Count: %u\n", slot->status->installed_count);
    }

    return g_string_free(str, FALSE);
}
