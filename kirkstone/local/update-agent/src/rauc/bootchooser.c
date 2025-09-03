#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include "../../include/rauc/bootchooser.h"
#include "../../include/rauc/context.h"
#include "../../include/rauc/utils.h"

#define GRUB_EDITENV "grub-editenv"

G_DEFINE_QUARK(r-bootchooser-error-quark, r_bootchooser_error)

/**
 * Execute grub-editenv command with given arguments
 *
 * @param args NULL-terminated array of arguments
 * @param error Error pointer for error reporting
 * @return TRUE on success, FALSE on error
 */
static gboolean execute_grub_editenv(gchar **args, GError **error)
{
    g_return_val_if_fail(args, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    gint exit_status;
    gchar *stderr_output = NULL;
    GError *spawn_error = NULL;

    if (!g_spawn_sync(NULL, args, NULL,
                      G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL,
                      NULL, NULL, NULL, &stderr_output, &exit_status, &spawn_error)) {
        g_set_error(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                   "Failed to execute %s: %s", GRUB_EDITENV, spawn_error->message);
        g_error_free(spawn_error);
        g_free(stderr_output);
        return FALSE;
    }

    if (exit_status != 0) {
        g_set_error(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                   "%s failed with exit code %d: %s", GRUB_EDITENV, exit_status,
                   stderr_output ? stderr_output : "Unknown error");
        g_free(stderr_output);
        return FALSE;
    }

    g_free(stderr_output);
    return TRUE;
}

/**
 * Get GRUB environment variable
 *
 * @param key Environment variable key
 * @param value Output string (must be freed by caller)
 * @param error Error pointer for error reporting
 * @return TRUE on success, FALSE on error
 */
gboolean grub_env_get(const gchar *key, GString **value, GError **error)
{
    g_return_val_if_fail(key, FALSE);
    g_return_val_if_fail(value && *value == NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    RaucContext *context = r_context_get();
    if (!context || !context->grubenv_path) {
        g_set_error(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                   "GRUB environment path not configured");
        return FALSE;
    }

    gchar *args[] = {
        GRUB_EDITENV,
        context->grubenv_path,
        "list",
        NULL
    };

    gchar *stdout_output = NULL;
    gchar *stderr_output = NULL;
    gint exit_status;
    GError *spawn_error = NULL;

    if (!g_spawn_sync(NULL, args, NULL,
                      G_SPAWN_SEARCH_PATH,
                      NULL, NULL, &stdout_output, &stderr_output, &exit_status, &spawn_error)) {
        g_set_error(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                   "Failed to execute %s: %s", GRUB_EDITENV, spawn_error->message);
        g_error_free(spawn_error);
        g_free(stdout_output);
        g_free(stderr_output);
        return FALSE;
    }

    if (exit_status != 0) {
        g_set_error(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                   "%s failed with exit code %d: %s", GRUB_EDITENV, exit_status,
                   stderr_output ? stderr_output : "Unknown error");
        g_free(stdout_output);
        g_free(stderr_output);
        return FALSE;
    }

    g_free(stderr_output);

    /* Parse output to find the requested key */
    *value = g_string_new("");
    if (stdout_output) {
        gchar **lines = g_strsplit(stdout_output, "\n", -1);
        for (gchar **line = lines; *line; line++) {
            if (g_str_has_prefix(*line, key)) {
                gchar *equal_sign = strchr(*line, '=');
                if (equal_sign) {
                    g_string_assign(*value, equal_sign + 1);
                    break;
                }
            }
        }
        g_strfreev(lines);
    }

    g_free(stdout_output);
    return TRUE;
}

/**
 * Set GRUB environment variables
 *
 * @param pairs Array of "key=value" strings
 * @param error Error pointer for error reporting
 * @return TRUE on success, FALSE on error
 */
gboolean grub_env_set(GPtrArray *pairs, GError **error)
{
    g_return_val_if_fail(pairs, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    RaucContext *context = r_context_get();
    if (!context || !context->grubenv_path) {
        g_set_error(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                   "GRUB environment path not configured");
        return FALSE;
    }

    /* Build arguments array */
    GPtrArray *args = g_ptr_array_new();
    g_ptr_array_add(args, g_strdup(GRUB_EDITENV));
    g_ptr_array_add(args, g_strdup(context->grubenv_path));
    g_ptr_array_add(args, g_strdup("set"));

    for (guint i = 0; i < pairs->len; i++) {
        g_ptr_array_add(args, g_strdup(g_ptr_array_index(pairs, i)));
    }
    g_ptr_array_add(args, NULL);

    gboolean result = execute_grub_editenv((gchar **)args->pdata, error);

    g_ptr_array_free(args, TRUE);
    return result;
}

/**
 * Generate new boot order with given slot as primary
 *
 * @param primary_slot Slot to be set as primary
 * @return New boot order string (must be freed by caller)
 */
static gchar* bootchooser_order_primary(RaucSlot *primary_slot)
{
    g_return_val_if_fail(primary_slot, NULL);
    g_return_val_if_fail(primary_slot->bootname, NULL);

    /* For simplicity, assume A/B slot system */
    if (g_strcmp0(primary_slot->bootname, "A") == 0) {
        return g_strdup("A B");
    } else if (g_strcmp0(primary_slot->bootname, "B") == 0) {
        return g_strdup("B A");
    }

    /* Fallback: just return the slot name */
    return g_strdup(primary_slot->bootname);
}

/**
 * Set slot as primary boot target
 */
gboolean grub_set_primary(RaucSlot *slot, GError **error)
{
    g_return_val_if_fail(slot, FALSE);
    g_return_val_if_fail(slot->bootname, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    g_autofree gchar *order = bootchooser_order_primary(slot);
    GPtrArray *pairs = g_ptr_array_new_with_free_func(g_free);

    g_ptr_array_add(pairs, g_strdup_printf("%s_OK=1", slot->bootname));
    g_ptr_array_add(pairs, g_strdup_printf("%s_TRY=0", slot->bootname));
    g_ptr_array_add(pairs, g_strdup_printf("ORDER=%s", order));

    gboolean result = grub_env_set(pairs, error);
    g_ptr_array_free(pairs, TRUE);

    return result;
}

/**
 * Set slot state (good/bad)
 */
gboolean grub_set_state(RaucSlot *slot, gboolean good, GError **error)
{
    g_return_val_if_fail(slot, FALSE);
    g_return_val_if_fail(slot->bootname, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    GPtrArray *pairs = g_ptr_array_new_with_free_func(g_free);

    if (good) {
        g_ptr_array_add(pairs, g_strdup_printf("%s_OK=1", slot->bootname));
        g_ptr_array_add(pairs, g_strdup_printf("%s_TRY=0", slot->bootname));
    } else {
        g_ptr_array_add(pairs, g_strdup_printf("%s_OK=0", slot->bootname));
        g_ptr_array_add(pairs, g_strdup_printf("%s_TRY=0", slot->bootname));
    }

    gboolean result = grub_env_set(pairs, error);
    g_ptr_array_free(pairs, TRUE);

    return result;
}

/**
 * Get slot state
 */
gboolean grub_get_state(RaucSlot *slot, gboolean *good, GError **error)
{
    g_return_val_if_fail(slot, FALSE);
    g_return_val_if_fail(slot->bootname, FALSE);
    g_return_val_if_fail(good, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    g_autoptr(GString) order = NULL;
    g_autoptr(GString) slot_ok = NULL;
    g_autoptr(GString) slot_try = NULL;
    g_auto(GStrv) bootnames = NULL;
    g_autofree gchar *key_ok = NULL;
    g_autofree gchar *key_try = NULL;
    gboolean found = FALSE;
    GError *ierror = NULL;

    /* Check if slot is in boot order */
    if (!grub_env_get("ORDER", &order, &ierror)) {
        g_propagate_error(error, ierror);
        return FALSE;
    }

    if (order->len > 0) {
        bootnames = g_strsplit(order->str, " ", -1);
        for (gchar **bootname = bootnames; *bootname; bootname++) {
            if (g_strcmp0(*bootname, slot->bootname) == 0) {
                found = TRUE;
                break;
            }
        }
    }

    if (!found) {
        *good = FALSE;
        return TRUE;
    }

    /* Check slot state variables */
    key_ok = g_strdup_printf("%s_OK", slot->bootname);
    if (!grub_env_get(key_ok, &slot_ok, &ierror)) {
        g_propagate_error(error, ierror);
        return FALSE;
    }

    key_try = g_strdup_printf("%s_TRY", slot->bootname);
    if (!grub_env_get(key_try, &slot_try, &ierror)) {
        g_propagate_error(error, ierror);
        return FALSE;
    }

    /* Slot is good if OK=1 and TRY=0 */
    *good = (g_ascii_strtoull(slot_ok->str, NULL, 0) == 1) &&
            (g_ascii_strtoull(slot_try->str, NULL, 0) == 0);

    return TRUE;
}

/**
 * Get current primary slot
 */
RaucSlot* grub_get_primary(GError **error)
{
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    RaucContext *context = r_context_get();
    if (!context || !context->config_slots) {
        g_set_error_literal(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                           "No RAUC context or slots configured");
        return NULL;
    }

    g_autoptr(GString) order = NULL;
    GError *ierror = NULL;

    if (!grub_env_get("ORDER", &order, &ierror)) {
        g_propagate_error(error, ierror);
        return NULL;
    }

    if (!order->len) {
        g_set_error_literal(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_PARSE_FAILED,
                           "Empty boot order");
        return NULL;
    }

    /* Parse order and find first good slot */
    g_auto(GStrv) bootnames = g_strsplit(order->str, " ", -1);
    for (gchar **bootname = bootnames; *bootname; bootname++) {
        GHashTableIter iter;
        RaucSlot *slot;
        g_hash_table_iter_init(&iter, context->config_slots);
        while (g_hash_table_iter_next(&iter, NULL, (gpointer*) &slot)) {
            if (slot->bootname && g_strcmp0(slot->bootname, *bootname) == 0) {
                gboolean good = FALSE;
                if (grub_get_state(slot, &good, NULL) && good) {
                    return slot;
                }
                break;
            }
        }
    }

    g_set_error_literal(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_PARSE_FAILED,
                       "No good primary slot found");
    return NULL;
}

/* Public API implementations */

gboolean r_boot_set_primary(RaucSlot *slot, GError **error)
{
    g_return_val_if_fail(slot, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    RaucContext *context = r_context_get();
    if (!context) {
        g_set_error_literal(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                           "RAUC context not initialized");
        return FALSE;
    }

    /* Check bootloader type */
    if (g_strcmp0(context->bootloader, "grub") == 0) {
        return grub_set_primary(slot, error);
    } else {
        g_set_error(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                   "Unsupported bootloader: %s",
                   context->bootloader ? context->bootloader : "none");
        return FALSE;
    }
}

gboolean r_boot_set_state(RaucSlot *slot, gboolean good, GError **error)
{
    g_return_val_if_fail(slot, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    RaucContext *context = r_context_get();
    if (!context) {
        g_set_error_literal(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                           "RAUC context not initialized");
        return FALSE;
    }

    if (g_strcmp0(context->bootloader, "grub") == 0) {
        return grub_set_state(slot, good, error);
    } else {
        g_set_error(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                   "Unsupported bootloader: %s",
                   context->bootloader ? context->bootloader : "none");
        return FALSE;
    }
}

gboolean r_boot_get_state(RaucSlot *slot, gboolean *good, GError **error)
{
    g_return_val_if_fail(slot, FALSE);
    g_return_val_if_fail(good, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    RaucContext *context = r_context_get();
    if (!context) {
        g_set_error_literal(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                           "RAUC context not initialized");
        return FALSE;
    }

    if (g_strcmp0(context->bootloader, "grub") == 0) {
        return grub_get_state(slot, good, error);
    } else {
        g_set_error(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                   "Unsupported bootloader: %s",
                   context->bootloader ? context->bootloader : "none");
        return FALSE;
    }
}

RaucSlot* r_boot_get_primary(GError **error)
{
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    RaucContext *context = r_context_get();
    if (!context) {
        g_set_error_literal(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                           "RAUC context not initialized");
        return NULL;
    }

    if (g_strcmp0(context->bootloader, "grub") == 0) {
        return grub_get_primary(error);
    } else {
        g_set_error(error, R_BOOTCHOOSER_ERROR, R_BOOTCHOOSER_ERROR_FAILED,
                   "Unsupported bootloader: %s",
                   context->bootloader ? context->bootloader : "none");
        return NULL;
    }
}

gboolean r_boot_mark_active(RaucSlot *slot, GError **error)
{
    g_return_val_if_fail(slot, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    g_info("Marking slot '%s' as active in bootloader", slot->name ? slot->name : "unknown");
    g_info("Slot bootname: %s", slot->bootname ? slot->bootname : "NULL");

    /* Mark slot as primary and set it as good */
    g_info("Setting slot as primary boot target");
    if (!r_boot_set_primary(slot, error)) {
        g_critical("Failed to set slot as primary: %s",
                   error && *error ? (*error)->message : "unknown error");
        return FALSE;
    }
    g_info("Successfully set slot as primary boot target");

    g_info("Setting slot state as good");
    if (!r_boot_set_state(slot, TRUE, error)) {
        g_critical("Failed to set slot state as good: %s",
                   error && *error ? (*error)->message : "unknown error");
        return FALSE;
    }
    g_info("Successfully set slot state as good");

    g_info("Slot '%s' successfully marked as active in bootloader", slot->name ? slot->name : "unknown");
    return TRUE;
}
