#include "bootchooser.h"
#include "event_log.h"
#include "context.h"
#include "install.h"
#include "mark.h"
#include "slot.h"
#include "status_file.h"

static RaucSlot* get_slot_by_identifier(const gchar *identifier, GError **error)
{
	GHashTableIter iter;
	RaucSlot *slot = NULL, *booted = NULL;

	g_return_val_if_fail(error == NULL || *error == NULL, NULL);

	g_hash_table_iter_init(&iter, r_context()->config->slots);
	while (g_hash_table_iter_next(&iter, NULL, (gpointer*) &booted)) {
		if (booted->state == ST_BOOTED)
			break;
		booted = NULL;
	}

	if (g_strcmp0(identifier, "booted") == 0) {
		if (booted)
			slot = booted;
		else
			g_set_error(
					error,
					R_SLOT_ERROR,
					R_SLOT_ERROR_NO_SLOT_WITH_STATE_BOOTED,
					"Did not find booted slot");
	} else if (g_strcmp0(identifier, "other") == 0) {
		if (booted) {
			g_hash_table_iter_init(&iter, r_context()->config->slots);
			while (g_hash_table_iter_next(&iter, NULL, (gpointer*) &slot)) {
				if (slot->sclass == booted->sclass && !slot->parent && slot->bootname && slot != booted)
					break;
				slot = NULL;
			}
			if (!slot)
				g_set_error(error,
						R_SLOT_ERROR,
						R_SLOT_ERROR_FAILED,
						"No other bootable slot of the same class found");
		} else {
			g_set_error(
					error,
					R_SLOT_ERROR,
					R_SLOT_ERROR_NO_SLOT_WITH_STATE_BOOTED,
					"Did not find booted slot needed to find another bootable slot of the same class");
		}
	} else {
		g_auto(GStrv) groupsplit = g_strsplit(identifier, ".", -1);

		if (g_strv_length(groupsplit) == 2) {
			slot = (RaucSlot*) g_hash_table_lookup(r_context()->config->slots, identifier);
			if (!slot) {
				g_set_error(error,
						R_SLOT_ERROR,
						R_SLOT_ERROR_FAILED,
						"No slot with class %s and name %s found",
						groupsplit[0],
						identifier);
				return NULL;
			}
			if (!slot->bootname) {
				g_set_error(error,
						R_SLOT_ERROR,
						R_SLOT_ERROR_FAILED,
						"Slot %s has no bootname set",
						slot->name);
				return NULL;
			}
		} else {
			g_set_error(error,
					R_SLOT_ERROR,
					R_SLOT_ERROR_FAILED,
					"Invalid slot name format: '%s'", identifier);
		}
	}

	return slot;
}

#define MESSAGE_ID_MARKED_ACTIVE "8b5e7435e1054d86858278e7544fe6da"
#define MESSAGE_ID_MARKED_GOOD   "3304e15a7a9a447885eb208ba7ae3a05"
#define MESSAGE_ID_MARKED_BAD    "ccb0e584a47043d7a5316994bce77ae5"

static void r_event_log_mark_active(RaucSlot *slot)
{
	g_return_if_fail(slot);

	g_log_structured(R_EVENT_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE,
			"RAUC_EVENT_TYPE", "mark",
			"MESSAGE_ID", MESSAGE_ID_MARKED_ACTIVE,
			"SLOT_NAME", slot->name,
			"SLOT_BOOTNAME", slot->bootname ?: "",
			"BUNDLE_HASH", slot->status->bundle_hash ?: "",
			"MESSAGE", "Marked slot %s as active", slot->name
			);
}

static void r_event_log_mark_good(RaucSlot *slot)
{
	g_return_if_fail(slot);

	g_log_structured(R_EVENT_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE,
			"RAUC_EVENT_TYPE", "mark",
			"MESSAGE_ID", MESSAGE_ID_MARKED_GOOD,
			"SLOT_NAME", slot->name,
			"SLOT_BOOTNAME", slot->bootname ?: "",
			"MESSAGE", "Marked slot %s as good", slot->name
			);
}

static void r_event_log_mark_bad(RaucSlot *slot)
{
	g_return_if_fail(slot);

	g_log_structured(R_EVENT_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE,
			"RAUC_EVENT_TYPE", "mark",
			"MESSAGE_ID", MESSAGE_ID_MARKED_BAD,
			"SLOT_NAME", slot->name,
			"SLOT_BOOTNAME", slot->bootname ?: "",
			"MESSAGE", "Marked slot %s as bad", slot->name
			);
}

gboolean r_mark_active(RaucSlot *slot, GError **error)
{
	RaucSlotStatus *slot_state;
	GError *ierror = NULL;
	g_autoptr(GDateTime) now = NULL;

	g_return_val_if_fail(slot, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	r_slot_status_load(slot);
	slot_state = slot->status;

	if (!r_boot_set_primary(slot, &ierror)) {
		g_set_error(error, R_INSTALL_ERROR, R_INSTALL_ERROR_MARK_BOOTABLE,
				"failed to activate slot %s: %s", slot->name, ierror->message);
		g_error_free(ierror);
		return FALSE;
	}

	r_event_log_mark_active(slot);

	g_free(slot_state->activated_timestamp);
	now = g_date_time_new_now_utc();
	slot_state->activated_timestamp = g_date_time_format(now, "%Y-%m-%dT%H:%M:%SZ");
	slot_state->activated_count++;

	if (!r_slot_status_save(slot, &ierror)) {
		g_message("Error while writing status file: %s", ierror->message);
		g_error_free(ierror);
	}

	return TRUE;
}

gboolean r_mark_good(RaucSlot *slot, GError **error)
{
	GError *ierror = NULL;

	g_return_val_if_fail(slot, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	if (!r_boot_set_state(slot, TRUE, &ierror)) {
		g_set_error(error, R_INSTALL_ERROR, R_INSTALL_ERROR_MARK_BOOTABLE,
				"Failed marking slot %s as good:  %s", slot->name, ierror->message);
		g_error_free(ierror);
		return FALSE;
	}

	r_event_log_mark_good(slot);

	return TRUE;
}

gboolean r_mark_bad(RaucSlot *slot, GError **error)
{
	GError *ierror = NULL;

	g_return_val_if_fail(slot, FALSE);
	g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

	if (!r_boot_set_state(slot, FALSE, &ierror)) {
		g_set_error(error, R_INSTALL_ERROR, R_INSTALL_ERROR_MARK_BOOTABLE,
				"Failed marking slot %s as bad:  %s", slot->name, ierror->message);
		g_error_free(ierror);
		return FALSE;
	}

	r_event_log_mark_bad(slot);

	return TRUE;
}

gboolean mark_run(const gchar *state,
		const gchar *slot_identifier,
		gchar **slot_name,
		gchar **message)
{
	RaucSlot *slot = NULL;
	g_autoptr(GError) ierror = NULL;

	g_assert(slot_name == NULL || *slot_name == NULL);
	g_assert(message != NULL && *message == NULL);

	slot = get_slot_by_identifier(slot_identifier, &ierror);
	if (ierror) {
		*message = g_strdup(ierror->message);
		return FALSE;
	}

	if (g_strcmp0(state, "good") == 0) {
		if (!r_mark_good(slot, &ierror)) {
			*message = g_strdup(ierror->message);
			return FALSE;
		}
		*message = g_strdup_printf("marked slot %s as good", slot->name);
	} else if (g_strcmp0(state, "bad") == 0) {
		if (!r_mark_bad(slot, &ierror)) {
			*message = g_strdup(ierror->message);
			return FALSE;
		}
		*message = g_strdup_printf("marked slot %s as bad", slot->name);
	} else if (g_strcmp0(state, "active") == 0) {
		if (!r_mark_active(slot, &ierror)) {
			*message = g_strdup(ierror->message);
			return FALSE;
		}
		*message = g_strdup_printf("activated slot %s", slot->name);
	} else {
		*message = g_strdup_printf("unknown subcommand %s", state);
		return FALSE;
	}

	if (slot_name)
		*slot_name = g_strdup(slot->name);

	return TRUE;
}
