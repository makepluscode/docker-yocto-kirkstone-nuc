#pragma once

#include <glib.h>

#include "config_file.h"

#define R_BOOTCHOOSER_ERROR r_bootchooser_error_quark()
GQuark r_bootchooser_error_quark(void);

#define R_BOOTCHOOSER_ERROR_FAILED		0
#define R_BOOTCHOOSER_ERROR_NOT_SUPPORTED	10
#define R_BOOTCHOOSER_ERROR_PARSE_FAILED	20

/**
 * Check if bootloader (name) is supported
 *
 * @param typename Name of bootloader as represented in config
 *
 * @return TRUE if it is supported, otherwise FALSE
 */
gboolean r_boot_is_supported_bootloader(const gchar *bootloader)
G_GNUC_WARN_UNUSED_RESULT;

/**
 * Mark slot as good or bad.
 *
 * @param slot Slot to mark
 * @param good Whether to mark it as good (instead of bad)
 * @param error return location for a GError, or NULL
 *
 * @return TRUE if successful, FALSE if failed
 */
gboolean r_boot_set_state(RaucSlot *slot, gboolean good, GError **error)
G_GNUC_WARN_UNUSED_RESULT;

/**
 * Mark slot as primary boot option of its slot class.
 *
 * @param slot Slot to mark
 * @param error return location for a GError, or NULL
 *
 * @return TRUE if successful, FALSE if failed
 */
gboolean r_boot_set_primary(RaucSlot *slot, GError **error)
G_GNUC_WARN_UNUSED_RESULT;

/**
 * Get primary boot slot.
 *
 * @param error return location for a GError, or NULL
 *
 * @return Primary slot, NULL if detection failed
 */
RaucSlot* r_boot_get_primary(GError **error)
G_GNUC_WARN_UNUSED_RESULT;

/**
 * Get bootloader state.
 *
 * @param slot Slot to get boot state from
 * @param good return location for slot status.
 *             TRUE means 'good', FALSE means 'bad')
 * @param error return location for a GError, or NULL
 *
 * @return TRUE if successful, FALSE if failed
 */
gboolean r_boot_get_state(RaucSlot* slot, gboolean *good, GError **error)
G_GNUC_WARN_UNUSED_RESULT;
