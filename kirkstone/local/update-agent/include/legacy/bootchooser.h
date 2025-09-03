#pragma once

#include <glib.h>
#include <stdbool.h>
#include "slot.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * GRUB bootchooser interface for RAUC update-library
 *
 * This module provides GRUB environment variable management
 * for A/B slot switching without D-Bus dependency.
 */

/* Error definitions */
#define R_BOOTCHOOSER_ERROR r_bootchooser_error_quark()
GQuark r_bootchooser_error_quark(void);

typedef enum {
    R_BOOTCHOOSER_ERROR_FAILED,
    R_BOOTCHOOSER_ERROR_PARSE_FAILED,
    R_BOOTCHOOSER_ERROR_ENV_ACCESS
} RBootchooserError;

/**
 * Set slot as primary boot target
 *
 * This function:
 * 1. Sets SLOT_OK=1, SLOT_TRY=0 for the given slot
 * 2. Updates ORDER to make this slot first priority
 * 3. Executes grub-editenv command to apply changes
 *
 * @param slot Target slot to set as primary
 * @param error Error pointer for error reporting
 * @return TRUE on success, FALSE on error
 */
gboolean r_boot_set_primary(RaucSlot *slot, GError **error);

/**
 * Set slot state (good/bad)
 *
 * @param slot Target slot
 * @param good TRUE for good state, FALSE for bad state
 * @param error Error pointer for error reporting
 * @return TRUE on success, FALSE on error
 */
gboolean r_boot_set_state(RaucSlot *slot, gboolean good, GError **error);

/**
 * Get slot state
 *
 * @param slot Target slot
 * @param good Output parameter for slot state
 * @param error Error pointer for error reporting
 * @return TRUE on success, FALSE on error
 */
gboolean r_boot_get_state(RaucSlot *slot, gboolean *good, GError **error);

/**
 * Get current primary slot
 *
 * @param error Error pointer for error reporting
 * @return Primary slot or NULL on error
 */
RaucSlot* r_boot_get_primary(GError **error);

/**
 * Mark slot as active (combines set_primary and set_state)
 *
 * @param slot Target slot to mark as active
 * @param error Error pointer for error reporting
 * @return TRUE on success, FALSE on error
 */
gboolean r_boot_mark_active(RaucSlot *slot, GError **error);

/* Internal GRUB environment functions */
gboolean grub_env_get(const gchar *key, GString **value, GError **error);
gboolean grub_env_set(GPtrArray *pairs, GError **error);
gboolean grub_set_primary(RaucSlot *slot, GError **error);
gboolean grub_set_state(RaucSlot *slot, gboolean good, GError **error);
gboolean grub_get_state(RaucSlot *slot, gboolean *good, GError **error);
RaucSlot* grub_get_primary(GError **error);

#ifdef __cplusplus
}
#endif
