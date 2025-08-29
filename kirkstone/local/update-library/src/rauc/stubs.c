#include <glib.h>
#include <string.h>
#include "../../include/rauc/context.h"
#include "../../include/rauc/bundle.h"
#include "../../include/rauc/manifest.h"
#include "../../include/rauc/utils.h"
#include "../../include/rauc/slot.h"
#include "../../include/rauc/install.h"

// 누락된 함수들의 간단한 stub 구현

gboolean r_subprocess_new(const gchar *command, gchar **out, GError **error)
{
    g_return_val_if_fail(command != NULL, FALSE);

    // 실제로는 system() 호출을 통해 구현하거나 g_spawn_sync 사용
    gint exit_status;
    gchar *stdout_str = NULL;
    gchar *stderr_str = NULL;

    gboolean success = g_spawn_command_line_sync(
        command,
        &stdout_str,
        &stderr_str,
        &exit_status,
        error
    );

    if (success && exit_status == 0) {
        if (out) {
            *out = stdout_str;
        } else {
            g_free(stdout_str);
        }
        g_free(stderr_str);
        return TRUE;
    }

    if (!*error && exit_status != 0) {
        g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
                   "Command failed with exit status %d: %s",
                   exit_status, stderr_str ? stderr_str : "");
    }

    g_free(stdout_str);
    g_free(stderr_str);
    return FALSE;
}

gboolean r_verify_signature(const gchar *manifest_path, const gchar *signature_path, GError **error)
{
    g_return_val_if_fail(manifest_path != NULL, FALSE);
    g_return_val_if_fail(signature_path != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    // Stub implementation - 실제로는 OpenSSL로 서명 검증
    if (!g_file_test(manifest_path, G_FILE_TEST_EXISTS)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                   "Manifest file not found: %s", manifest_path);
        return FALSE;
    }

    if (!g_file_test(signature_path, G_FILE_TEST_EXISTS)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                   "Signature file not found: %s", signature_path);
        return FALSE;
    }

    // 간단히 파일 존재만 확인하고 통과
    return TRUE;
}

RaucContext* r_context_get(void)
{
    // Stub implementation - 글로벌 컨텍스트 반환
    static RaucContext global_context = {0};
    static RaucConfig global_config = {0};
    static gboolean initialized = FALSE;

    if (!initialized) {
        global_config.system_compatible = g_strdup("intel-i7-x64-nuc-rauc");
        global_config.slots = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)r_slot_free);

        global_context.compatible = g_strdup("intel-i7-x64-nuc-rauc");
        global_context.data_directory = g_strdup("/data");
        global_context.config = &global_config;

        // Add actual slots
        RaucSlot *slot0 = g_new0(RaucSlot, 1);
        slot0->name = g_strdup("rootfs.0");
        slot0->sclass = g_strdup("rootfs");
        slot0->slot_class = g_strdup("rootfs");
        slot0->device = g_strdup("/dev/sda2");
        slot0->type = g_strdup("ext4");
        slot0->bootname = g_strdup("A");
        slot0->state = ST_BOOTED;
        slot0->rauc_state = R_SLOT_STATE_BOOTED;
        slot0->status = g_new0(RaucSlotStatus, 1);
        slot0->status->status = g_strdup("good");
        // Store slots by name for lookup
        g_hash_table_insert(global_config.slots, g_strdup("rootfs.0"), slot0);

        RaucSlot *slot1 = g_new0(RaucSlot, 1);
        slot1->name = g_strdup("rootfs.1");
        slot1->sclass = g_strdup("rootfs");
        slot1->slot_class = g_strdup("rootfs");
        slot1->device = g_strdup("/dev/sda3");
        slot1->type = g_strdup("ext4");
        slot1->bootname = g_strdup("B");
        slot1->state = ST_INACTIVE;
        slot1->rauc_state = R_SLOT_STATE_INACTIVE;
        slot1->status = g_new0(RaucSlotStatus, 1);
        slot1->status->status = g_strdup("inactive");
        g_hash_table_insert(global_config.slots, g_strdup("rootfs.1"), slot1);

        global_context.config = &global_config;
        initialized = TRUE;
    }

    return &global_context;
}

// 추가 누락된 함수들

RaucSlot* r_context_find_slot_by_class(RaucContext *context, const gchar *slotclass)
{
    // Return inactive slot (B slot) for installation
    if (context && context->config && context->config->slots) {
        // Look for inactive slot (rootfs.1) first
        RaucSlot *slot = g_hash_table_lookup(context->config->slots, "rootfs.1");
        if (slot && slot->rauc_state == R_SLOT_STATE_INACTIVE) {
            return slot;
        }

        // If no inactive slot found, return rootfs.0
        return g_hash_table_lookup(context->config->slots, "rootfs.0");
    }
    return NULL;
}

// 위의 함수들은 실제 구현 파일들(bundle.c, manifest.c, install.c)에서 제공되므로
// stubs.c에서는 제거함
