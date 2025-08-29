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
        global_config.system_compatible = g_strdup("test-system");
        global_config.slots = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
        global_context.config = &global_config;
        initialized = TRUE;
    }

    return &global_context;
}

// 추가 누락된 함수들

RaucSlot* r_context_find_slot_by_class(RaucContext *context, const gchar *slotclass)
{
    // Stub implementation
    static RaucSlot dummy_slot;
    static gboolean initialized = FALSE;

    if (!initialized) {
        memset(&dummy_slot, 0, sizeof(RaucSlot));
        dummy_slot.name = g_intern_string("test-slot");
        dummy_slot.sclass = g_intern_string(slotclass);
        dummy_slot.slot_class = g_intern_string(slotclass);
        dummy_slot.device = g_strdup("/dev/null");
        dummy_slot.state = ST_INACTIVE;
        dummy_slot.rauc_state = R_SLOT_STATE_INACTIVE;
        initialized = TRUE;
    }
    return &dummy_slot;
}

// 위의 함수들은 실제 구현 파일들(bundle.c, manifest.c, install.c)에서 제공되므로
// stubs.c에서는 제거함
