#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>

#include "../../include/rauc/context.h"
#include "../../include/rauc/install.h"
#include "../../include/rauc/bundle.h"
#include "../../include/rauc/manifest.h"
#include "../../include/rauc/slot.h"
#include "../../include/rauc/utils.h"
#include "../../include/rauc/checksum.h"
#include "../../include/rauc/bootchooser.h"

#ifdef WITH_DLT
#include <dlt/dlt.h>
DLT_DECLARE_CONTEXT(dlt_context);
static gboolean dlt_initialized = FALSE;

// Initialize DLT context
static void init_dlt_context(void) {
    if (!dlt_initialized) {
        DLT_REGISTER_CONTEXT(dlt_context, "RINST", "RAUC Install");
        dlt_initialized = TRUE;
    }
}
#endif

G_DEFINE_QUARK(r-install-error-quark, r_install_error)

typedef struct {
    RaucSlot *slot;
    RaucImage *image;
    gchar *image_path;
    gboolean completed;
    GError *error;
} InstallTask;

static void install_task_free(InstallTask *task)
{
    if (task == NULL)
        return;

    g_free(task->image_path);
    if (task->error) {
        g_error_free(task->error);
    }
    g_free(task);
}

static InstallTask* install_task_new(RaucSlot *slot, RaucImage *image, const gchar *image_path)
{
    InstallTask *task = g_new0(InstallTask, 1);
    task->slot = slot;
    task->image = image;
    task->image_path = g_strdup(image_path);
    task->completed = FALSE;
    task->error = NULL;
    return task;
}

static gboolean verify_slot_compatible(RaucSlot *slot, RaucImage *image, GError **error)
{
    g_return_val_if_fail(slot != NULL, FALSE);
    g_return_val_if_fail(image != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (g_strcmp0(slot->sclass, image->slotclass) != 0) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Slot class mismatch: slot '%s' has class '%s', image requires '%s'",
                   slot->name, slot->sclass, image->slotclass);
        return FALSE;
    }

    return TRUE;
}

static gboolean copy_image_to_slot(const gchar *image_path, RaucSlot *slot,
                                 RaucProgressCallback progress_callback, gpointer user_data,
                                 GError **error)
{
    GError *ierror = NULL;
    gint image_fd = -1;
    gint slot_fd = -1;
    struct stat st;
    gchar buffer[64 * 1024];
    gssize bytes_read, bytes_written, total_written = 0;
    gboolean res = FALSE;

    g_return_val_if_fail(image_path != NULL, FALSE);
    g_return_val_if_fail(slot != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    g_info("Mounting slot '%s' for image copy", slot->name ? slot->name : "unknown");
    if (!r_slot_mount(slot, &ierror)) {
        g_critical("Failed to mount slot '%s': %s", slot->name ? slot->name : "unknown",
                   ierror ? ierror->message : "unknown error");
        g_propagate_prefixed_error(error, ierror, "Failed to mount slot '%s': ", slot->name);
        goto out;
    }
    g_info("Slot '%s' mounted successfully", slot->name ? slot->name : "unknown");

    g_info("Opening image file '%s' for reading", image_path);
    image_fd = open(image_path, O_RDONLY);
    if (image_fd < 0) {
        g_critical("Failed to open image file '%s': %s", image_path, g_strerror(errno));
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to open image file '%s': %s", image_path, g_strerror(errno));
        goto out;
    }
    g_info("Image file '%s' opened successfully", image_path);

    if (fstat(image_fd, &st) < 0) {
        g_critical("Failed to stat image file '%s': %s", image_path, g_strerror(errno));
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to stat image file '%s': %s", image_path, g_strerror(errno));
        goto out;
    }
    g_info("Image file size: %lld bytes", (long long)st.st_size);

    g_info("Opening slot device '%s' for writing", slot->device ? slot->device : "unknown");
    slot_fd = open(slot->device, O_WRONLY);
    if (slot_fd < 0) {
        g_critical("Failed to open slot device '%s': %s", slot->device ? slot->device : "unknown", g_strerror(errno));
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to open slot device '%s': %s", slot->device, g_strerror(errno));
        goto out;
    }
    g_info("Slot device '%s' opened successfully", slot->device ? slot->device : "unknown");

    g_info("Starting image copy from '%s' to slot device '%s'", image_path, slot->device ? slot->device : "unknown");
    while ((bytes_read = read(image_fd, buffer, sizeof(buffer))) > 0) {
        gchar *write_ptr = buffer;
        gssize remaining = bytes_read;

        while (remaining > 0) {
            bytes_written = write(slot_fd, write_ptr, remaining);
            if (bytes_written < 0) {
                if (errno == EINTR)
                    continue;
                g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                           "Failed to write to slot device '%s': %s", slot->device, g_strerror(errno));
                goto out;
            }

            write_ptr += bytes_written;
            remaining -= bytes_written;
            total_written += bytes_written;
        }

        if (progress_callback) {
            int percentage = (int)((total_written * 100) / st.st_size);
            static int last_reported = -1;

            // Report progress every 10% or at 100%
            if ((percentage / 10) != (last_reported / 10) || (percentage == 100 && last_reported != 100)) {
                gchar *message = g_strdup_printf("Installing to slot '%s': %d%%", slot->name, percentage);
                progress_callback(percentage, message, 1, user_data);
                g_free(message);
                last_reported = percentage;
            }
        }
    }

    if (bytes_read < 0) {
        g_critical("Failed to read from image file '%s': %s", image_path, g_strerror(errno));
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to read from image file '%s': %s", image_path, g_strerror(errno));
        goto out;
    }

    g_info("Image copy completed, syncing slot device '%s'", slot->device ? slot->device : "unknown");
    if (fsync(slot_fd) < 0) {
        g_critical("Failed to sync slot device '%s': %s", slot->device ? slot->device : "unknown", g_strerror(errno));
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to sync slot device '%s': %s", slot->device, g_strerror(errno));
        goto out;
    }
    g_info("Slot device '%s' synced successfully", slot->device ? slot->device : "unknown");

    res = TRUE;

out:
    if (image_fd >= 0)
        close(image_fd);
    if (slot_fd >= 0)
        close(slot_fd);

    if (slot->mount_point) {
        r_slot_unmount(slot, NULL);
    }

    return res;
}

static gboolean verify_installed_image(RaucSlot *slot, RaucImage *image,
                                     RaucProgressCallback progress_callback, gpointer user_data,
                                     GError **error)
{
    GError *ierror = NULL;
    gboolean res = FALSE;
    RaucChecksum computed = {};

    g_return_val_if_fail(slot != NULL, FALSE);
    g_return_val_if_fail(image != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!image->checksum.digest) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Image has no checksum for verification");
        goto out;
    }

    if (progress_callback) {
        gchar *message = g_strdup_printf("Verifying slot '%s'", slot->name);
        progress_callback(0, message, 1, user_data);
        g_free(message);
    }

    /* Use exactly the same approach as original RAUC: verify_checksum */
    computed.type = image->checksum.type;

    if (!r_checksum_file(slot->device, &computed, &ierror)) {
        g_propagate_prefixed_error(error, ierror, "Failed to calculate checksum of installed image: ");
        goto out;
    }

    /* Check size match first */
    res = (image->checksum.size == computed.size);
    if (!res) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Size verification failed for slot '%s'. Expected: %lld, Calculated: %lld",
                   slot->name, (long long)image->checksum.size, (long long)computed.size);
        goto out;
    }

    /* Check digest match */
    res = g_str_equal(image->checksum.digest, computed.digest);
    if (!res) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Checksum verification failed for slot '%s'. Expected: %s, Calculated: %s",
                   slot->name, image->checksum.digest, computed.digest);
        goto out;
    }

    if (progress_callback) {
        gchar *message = g_strdup_printf("Verification completed for slot '%s'", slot->name);
        progress_callback(100, message, 1, user_data);
        g_free(message);
    }

    res = TRUE;

out:
    if (computed.digest) {
        g_free(computed.digest);
    }
    return res;
}

static gboolean update_slot_status(RaucSlot *slot, RaucSlotState state, GError **error)
{
    GError *ierror = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(slot != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    slot->rauc_state = state;

    if (!r_slot_save_status(slot, &ierror)) {
        g_propagate_prefixed_error(error, ierror, "Failed to save slot status: ");
        goto out;
    }

    res = TRUE;

out:
    return res;
}

static gboolean install_image_to_slot(InstallTask *task,
                                    RaucProgressCallback progress_callback, gpointer user_data,
                                    GError **error)
{
    GError *ierror = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(task != NULL, FALSE);
    g_return_val_if_fail(task->slot != NULL, FALSE);
    g_return_val_if_fail(task->image != NULL, FALSE);
    g_return_val_if_fail(task->image_path != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    g_info("Starting installation of image '%s' to slot '%s'",
           task->image->filename ? task->image->filename : "unknown",
           task->slot->name ? task->slot->name : "unknown");

    if (progress_callback) {
        gchar *message = g_strdup_printf("[Step 1/5] Starting installation of '%s' to slot '%s'",
                                       task->image->filename, task->slot->name);
        progress_callback(0, message, 0, user_data);
        g_free(message);
    }

    g_info("Verifying slot compatibility for slot '%s' with image class '%s'",
           task->slot->name ? task->slot->name : "unknown",
           task->image->slotclass ? task->image->slotclass : "unknown");

    if (progress_callback) {
        gchar *message = g_strdup_printf("[Step 2/5] Verifying slot compatibility");
        progress_callback(5, message, 0, user_data);
        g_free(message);
    }

    if (!verify_slot_compatible(task->slot, task->image, &ierror)) {
        g_critical("Slot compatibility verification failed: %s", ierror ? ierror->message : "unknown error");
        g_propagate_error(error, ierror);
        goto out;
    }
    g_info("Slot compatibility verification passed");

    g_info("Updating slot '%s' status to inactive", task->slot->name ? task->slot->name : "unknown");

    if (progress_callback) {
        gchar *message = g_strdup_printf("[Step 3/5] Updating slot status to inactive");
        progress_callback(10, message, 0, user_data);
        g_free(message);
    }

    if (!update_slot_status(task->slot, R_SLOT_STATE_INACTIVE, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    if (progress_callback) {
        gchar *message = g_strdup_printf("[Step 4/5] Copying image data to slot");
        progress_callback(15, message, 0, user_data);
        g_free(message);
    }

    if (!copy_image_to_slot(task->image_path, task->slot, progress_callback, user_data, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    if (progress_callback) {
        gchar *message = g_strdup_printf("[Step 5/5] Finalizing installation and updating slot status");
        progress_callback(98, message, 0, user_data);
        g_free(message);
    }

    if (!update_slot_status(task->slot, R_SLOT_STATE_GOOD, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    /* Follow original RAUC approach: no post-installation verification */
    if (progress_callback) {
        gchar *message = g_strdup_printf("Installation to slot '%s' completed successfully", task->slot->name);
        progress_callback(100, message, 1, user_data);
        g_free(message);
    }

    /* Mark slot as active in bootloader */
    g_info("Attempting to mark slot as active in bootloader");
    g_info("Slot name: %s, bootname: %s",
           task->slot && task->slot->name ? task->slot->name : "NULL",
           task->slot && task->slot->bootname ? task->slot->bootname : "NULL");

    if (task->slot && task->slot->name && task->slot->bootname) {
        g_info("Calling r_boot_mark_active for slot '%s' with bootname '%s'",
               task->slot->name, task->slot->bootname);

        if (!r_boot_mark_active(task->slot, &ierror)) {
            g_critical("Failed to mark slot %s as active in bootloader: %s",
                      task->slot->name, ierror ? ierror->message : "unknown error");
            g_clear_error(&ierror);
        } else {
            g_info("Successfully marked slot %s as active in bootloader", task->slot->name);

            /* RAUC install과 동일하게 bootchooser 적용 후 재부팅 수행 */
            if (r_install_auto_reboot) {
                GError *reboot_error = NULL;
                g_info("Auto-reboot enabled, initiating system reboot...");

                if (!r_install_reboot_system(&reboot_error)) {
                    g_critical("Failed to reboot system: %s",
                             reboot_error ? reboot_error->message : "unknown error");
                    g_clear_error(&reboot_error);
                } else {
                    g_info("System reboot initiated successfully");
                }
            }
        }
    } else {
        g_critical("Cannot mark slot as active: slot data is incomplete");
        g_critical("Slot: %p, name: %s, bootname: %s",
                   task->slot,
                   task->slot && task->slot->name ? task->slot->name : "NULL",
                   task->slot && task->slot->bootname ? task->slot->bootname : "NULL");
    }

    if (progress_callback) {
        const gchar *image_name = (task->image && task->image->filename) ? task->image->filename : "unknown";
        const gchar *slot_name = (task->slot && task->slot->name) ? task->slot->name : "unknown";
        gchar *message = g_strdup_printf("Successfully installed image '%s' to slot '%s'",
                                       image_name, slot_name);
        progress_callback(100, message, 0, user_data);
        g_free(message);
    }

    task->completed = TRUE;
    res = TRUE;

out:
    if (!res && task->slot) {
        update_slot_status(task->slot, R_SLOT_STATE_BAD, NULL);
    }
    return res;
}

static GList* create_install_tasks(RaucBundle *bundle, GError **error)
{
    RaucContext *context = r_context_get();
    GList *tasks = NULL;
    GHashTableIter iter;
    gpointer key, value;
    GError *ierror = NULL;

    g_return_val_if_fail(bundle != NULL, NULL);
    g_return_val_if_fail(bundle->manifest != NULL, NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    if (!bundle->manifest->images) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Bundle contains no images");
        return NULL;
    }

    g_hash_table_iter_init(&iter, bundle->manifest->images);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucImage *image = (RaucImage*)value;
        RaucSlot *target_slot = NULL;
        gchar *image_path = NULL;
        InstallTask *task = NULL;

        target_slot = r_context_find_slot_by_class(context, image->slotclass);
        if (!target_slot) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "No target slot found for slot class '%s'", image->slotclass);
            goto error;
        }

        image_path = r_bundle_get_image_path(bundle, image->slotclass, &ierror);
        if (!image_path) {
            g_propagate_error(error, ierror);
            goto error;
        }

        task = install_task_new(target_slot, image, image_path);
        tasks = g_list_append(tasks, task);

        g_free(image_path);
    }

    return tasks;

error:
    g_list_free_full(tasks, (GDestroyNotify)install_task_free);
    return NULL;
}

gboolean r_install_bundle(RaucBundle *bundle,
                         RaucProgressCallback progress_callback,
                         RaucCompletionCallback completed_callback,
                         gpointer user_data,
                         GError **error)
{
    GError *ierror = NULL;
    GList *tasks = NULL;
    GList *current = NULL;
    gint total_tasks = 0;
    gint completed_tasks = 0;
    gboolean res = FALSE;

    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (progress_callback) {
        progress_callback(0, "Starting bundle installation", 0, user_data);
    }

    // Skip signature verification if bundle already has sigdata (already verified)
    if (!bundle->sigdata) {
        g_info("Starting bundle signature verification...");
        if (!r_bundle_verify_signature(bundle, &ierror)) {
            g_critical("Bundle signature verification failed: %s", ierror ? ierror->message : "unknown error");
            g_propagate_prefixed_error(error, ierror, "Bundle signature verification failed: ");
            goto out;
        }
        g_info("Bundle signature verification passed");
    } else {
        g_info("Skipping signature verification (already verified)");
    }

    g_info("Starting compatibility check...");
    if (!r_bundle_check_compatible(bundle, &ierror)) {
        g_critical("Bundle compatibility check failed: %s", ierror ? ierror->message : "unknown error");
        g_propagate_prefixed_error(error, ierror, "Bundle compatibility check failed: ");
        goto out;
    }
    g_info("Compatibility check passed");

    g_info("Starting content verification...");
    if (!r_bundle_verify_content(bundle, &ierror)) {
        g_critical("Bundle content verification failed: %s", ierror ? ierror->message : "unknown error");
        g_propagate_prefixed_error(error, ierror, "Bundle content verification failed: ");
        goto out;
    }
    g_info("Content verification passed");

    g_info("Creating install tasks...");
    tasks = create_install_tasks(bundle, &ierror);
    if (!tasks) {
        g_critical("Failed to create install tasks: %s", ierror ? ierror->message : "unknown error");
        g_propagate_error(error, ierror);
        goto out;
    }
    g_info("Install tasks created successfully");

    total_tasks = g_list_length(tasks);

    if (progress_callback) {
        gchar *message = g_strdup_printf("Installing %d images", total_tasks);
        progress_callback(10, message, 0, user_data);
        g_free(message);
    }

    for (current = tasks; current != NULL; current = current->next) {
        InstallTask *task = (InstallTask*)current->data;

        if (!install_image_to_slot(task, progress_callback, user_data, &ierror)) {
            task->error = g_error_copy(ierror);
            g_propagate_error(error, ierror);
            goto out;
        }

        completed_tasks++;

        if (progress_callback) {
            int overall_progress = 10 + (completed_tasks * 80) / total_tasks;
            gchar *message = g_strdup_printf("Installed %d of %d images", completed_tasks, total_tasks);
            progress_callback(overall_progress, message, 0, user_data);
            g_free(message);
        }
    }

    if (progress_callback) {
        progress_callback(100, "Installation completed successfully", 0, user_data);
    }

    if (completed_callback) {
        completed_callback(R_INSTALL_RESULT_SUCCESS, "Installation completed successfully", user_data);
    }

    res = TRUE;

out:
    if (!res) {
        if (completed_callback) {
            gchar *error_msg = error && *error ? (*error)->message : "Unknown error";
            completed_callback(R_INSTALL_RESULT_FAILURE, error_msg, user_data);
        }
    }

    g_list_free_full(tasks, (GDestroyNotify)install_task_free);
    return res;
}

gboolean r_install_bundle_from_file(const gchar *bundle_path,
                                   RaucProgressCallback progress_callback,
                                   RaucCompletionCallback completed_callback,
                                   gpointer user_data,
                                   GError **error)
{
    GError *ierror = NULL;
    RaucBundle *bundle = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(bundle_path != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (progress_callback) {
        gchar *message = g_strdup_printf("Opening bundle '%s'", bundle_path);
        progress_callback(0, message, 0, user_data);
        g_free(message);
    }

    // update-test-app과 동일하게 r_bundle_load 사용 (서명 데이터 포함)
    if (!r_bundle_load(bundle_path, &bundle, &ierror)) {
        g_critical("Failed to load bundle '%s': %s", bundle_path,
                   ierror ? ierror->message : "unknown error");
        g_propagate_error(error, ierror);
        goto out;
    }
    g_info("Bundle '%s' loaded successfully with signature data", bundle_path);

    if (!r_install_bundle(bundle, progress_callback, completed_callback, user_data, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    res = TRUE;

out:
    if (bundle) {
        r_bundle_free(bundle);
    }
    return res;
}

gboolean r_install_get_progress(gint *percentage, gchar **message, gint *nesting_depth)
{
    static gint current_percentage = 0;
    static gchar *current_message = NULL;

    if (percentage) {
        *percentage = current_percentage;
    }

    if (message) {
        *message = g_strdup(current_message ? current_message : "No operation in progress");
    }

    if (nesting_depth) {
        *nesting_depth = 0;  // 기본값
    }

    return current_percentage < 100;
}

void r_install_cancel(void)
{

}

gchar* r_install_get_status_info(void)
{
    RaucContext *context = r_context_get();
    GString *info = NULL;
    GHashTableIter iter;
    gpointer key, value;
    gchar *result = NULL;

    info = g_string_new("Installation Status:\n\n");

    if (!context || !context->config || !context->config->slots) {
        g_string_append(info, "No slot information available\n");
        goto out;
    }

    g_hash_table_iter_init(&iter, context->config->slots);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucSlot *slot = (RaucSlot*)value;
        const gchar *state_str = "unknown";

        switch (slot->rauc_state) {
            case R_SLOT_STATE_INACTIVE:
                state_str = "inactive";
                break;
            case R_SLOT_STATE_BOOTED:
                state_str = "booted";
                break;
            case R_SLOT_STATE_ACTIVE:
                state_str = "active";
                break;
            case R_SLOT_STATE_GOOD:
                state_str = "good";
                break;
            case R_SLOT_STATE_BAD:
                state_str = "bad";
                break;
        }

        g_string_append_printf(info, "Slot '%s' (%s): %s\n",
                              slot->name, slot->sclass, state_str);

        if (slot->device) {
            g_string_append_printf(info, "  Device: %s\n", slot->device);
        }

        if (slot->mount_point) {
            g_string_append_printf(info, "  Mounted at: %s\n", slot->mount_point);
        }

        g_string_append(info, "\n");
    }

out:
    result = g_string_free(info, FALSE);
    return result;
}

/* 자동 재부팅 옵션 전역 변수 */
gboolean r_install_auto_reboot = FALSE;

gboolean r_install_reboot_system(GError **error)
{
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    gchar *reboot_command = NULL;
    gchar *stdout_output = NULL;
    gchar *stderr_output = NULL;
    gint exit_status;
    GError *spawn_error = NULL;
    gboolean success = FALSE;

#ifdef WITH_DLT
    init_dlt_context();
#endif
    r_info("Initiating system reboot...");

    /* RAUC와 동일하게 systemctl reboot 명령 사용 */
    reboot_command = g_strdup("systemctl reboot");

    /* 명령 실행 (RAUC와 동일한 방식) */
    if (!g_spawn_command_line_sync(reboot_command,
                                   &stdout_output,
                                   &stderr_output,
                                   &exit_status,
                                   &spawn_error)) {
        g_set_error(error, R_INSTALL_ERROR, R_INSTALL_ERROR_FAILED,
                   "Failed to execute reboot command: %s",
                   spawn_error->message);
        g_error_free(spawn_error);
        goto out;
    }

    if (exit_status != 0) {
        g_set_error(error, R_INSTALL_ERROR, R_INSTALL_ERROR_FAILED,
                   "Reboot command failed with exit code %d: %s",
                   exit_status, stderr_output ? stderr_output : "Unknown error");
        goto out;
    }

#ifdef WITH_DLT
    init_dlt_context();
#endif
    r_info("System reboot command executed successfully");
    success = TRUE;

out:
    g_free(reboot_command);
    g_free(stdout_output);
    g_free(stderr_output);

    return success;
}
