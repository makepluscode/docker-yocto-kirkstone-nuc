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
    // Return the global context from context.c
    extern RaucContext *r_context;

    if (!r_context) {
        return NULL;
    }

    return r_context;
}

// 추가 누락된 함수들

RaucSlot* r_context_find_slot_by_class(RaucContext *context, const gchar *slotclass)
{
    if (!context || !slotclass) {
        return NULL;
    }

    /* Find the non-booted slot for installation (A/B switching) */
    if (g_strcmp0(slotclass, "rootfs") == 0) {
        RaucSlot *slot = g_new0(RaucSlot, 1);

        /* Check which slot is currently booted by reading /proc/cmdline */
        gchar *cmdline_content = NULL;
        gboolean is_booted_from_b = FALSE;

        if (g_file_get_contents("/proc/cmdline", &cmdline_content, NULL, NULL)) {
            /* Check if booted from sda3 (B slot) or sda2 (A slot) */
            if (g_strstr_len(cmdline_content, -1, "root=/dev/sda3") ||
                g_strstr_len(cmdline_content, -1, "PARTUUID=") &&
                g_strstr_len(cmdline_content, -1, "sda3")) {
                is_booted_from_b = TRUE;
            }
            g_free(cmdline_content);
        }

        /* Install to the non-booted slot */
        if (is_booted_from_b) {
            /* Booted from B, install to A */
            slot->name = g_strdup("rootfs.0");
            slot->device = g_strdup("/dev/sda2");
            slot->bootname = g_strdup("A");
            slot->data_directory = g_strdup("slots/rootfs.0");
        } else {
            /* Booted from A, install to B */
            slot->name = g_strdup("rootfs.1");
            slot->device = g_strdup("/dev/sda3");
            slot->bootname = g_strdup("B");
            slot->data_directory = g_strdup("slots/rootfs.1");
        }

        slot->sclass = g_strdup("rootfs");
        slot->type = g_strdup("ext4");
        slot->state = ST_INACTIVE;
        slot->rauc_state = R_SLOT_STATE_INACTIVE;

        // status 구조체 생성 및 초기화
        slot->status = g_new0(RaucSlotStatus, 1);
        slot->status->bundle_compatible = g_strdup("intel-i7-x64-nuc-rauc");
        slot->status->bundle_version = g_strdup("0.0.1");
        slot->status->bundle_description = g_strdup("NUC Qt5 Image");
        slot->status->status = g_strdup("ok");
        slot->status->installed_count = 0;
        slot->status->activated_count = 0;

        return slot;
    }

    return NULL;
}

// 위의 함수들은 실제 구현 파일들(bundle.c, manifest.c, install.c)에서 제공되므로
// stubs.c에서는 제거함
