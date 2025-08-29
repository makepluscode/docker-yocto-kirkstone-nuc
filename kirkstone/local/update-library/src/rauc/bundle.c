#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <errno.h>
#include <glib.h>

#include "../../include/rauc/context.h"
#include "../../include/rauc/bundle.h"
#include "../../include/rauc/utils.h"
#include "../../include/rauc/checksum.h"

static gchar *bundle_mount_point = NULL;

gboolean r_bundle_mount(const gchar *bundlename, gchar **mountpoint, GError **error)
{
    GError *ierror = NULL;
    gchar *tmpdir = NULL;
    gchar *mount_cmd = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(bundlename != NULL, FALSE);
    g_return_val_if_fail(mountpoint != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    tmpdir = g_dir_make_tmp("rauc-bundle-XXXXXX", &ierror);
    if (tmpdir == NULL) {
        g_propagate_prefixed_error(error, ierror, "Failed to create temporary directory: ");
        goto out;
    }

    mount_cmd = g_strdup_printf("mount -o loop,ro '%s' '%s'", bundlename, tmpdir);
    if (!r_subprocess_new(mount_cmd, NULL, &ierror)) {
        g_propagate_prefixed_error(error, ierror, "Failed to mount bundle: ");
        goto out;
    }

    *mountpoint = g_strdup(tmpdir);
    bundle_mount_point = g_strdup(tmpdir);
    res = TRUE;

out:
    if (!res && tmpdir) {
        g_rmdir(tmpdir);
        g_free(tmpdir);
    }
    g_free(mount_cmd);
    return res;
}

gboolean r_bundle_unmount(const gchar *mountpoint, GError **error)
{
    GError *ierror = NULL;
    gchar *umount_cmd = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(mountpoint != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    umount_cmd = g_strdup_printf("umount '%s'", mountpoint);
    if (!r_subprocess_new(umount_cmd, NULL, &ierror)) {
        g_propagate_prefixed_error(error, ierror, "Failed to unmount bundle: ");
        goto out;
    }

    if (g_rmdir(mountpoint) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to remove mount directory '%s': %s", mountpoint, g_strerror(errno));
        goto out;
    }

    if (bundle_mount_point && g_strcmp0(bundle_mount_point, mountpoint) == 0) {
        g_free(bundle_mount_point);
        bundle_mount_point = NULL;
    }

    res = TRUE;

out:
    g_free(umount_cmd);
    return res;
}

static gboolean check_bundle_structure(const gchar *mountpoint, GError **error)
{
    gchar *manifest_path = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(mountpoint != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    manifest_path = g_build_filename(mountpoint, "manifest.raucm", NULL);

    if (!g_file_test(manifest_path, G_FILE_TEST_EXISTS)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                   "Bundle manifest not found at '%s'", manifest_path);
        goto out;
    }

    if (!g_file_test(manifest_path, G_FILE_TEST_IS_REGULAR)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Bundle manifest is not a regular file: '%s'", manifest_path);
        goto out;
    }

    res = TRUE;

out:
    g_free(manifest_path);
    return res;
}

gboolean r_bundle_open(const gchar *bundlename, RaucBundle **bundle, GError **error)
{
    GError *ierror = NULL;
    RaucBundle *ibundle = NULL;
    gchar *mountpoint = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(bundlename != NULL, FALSE);
    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!g_file_test(bundlename, G_FILE_TEST_EXISTS)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                   "Bundle file not found: '%s'", bundlename);
        goto out;
    }

    if (!r_bundle_mount(bundlename, &mountpoint, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    if (!check_bundle_structure(mountpoint, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    ibundle = g_new0(RaucBundle, 1);
    ibundle->path = g_strdup(bundlename);
    ibundle->mount_point = g_strdup(mountpoint);
    ibundle->manifest = NULL;

    *bundle = ibundle;
    res = TRUE;

out:
    if (!res) {
        if (mountpoint) {
            r_bundle_unmount(mountpoint, NULL);
        }
        if (ibundle) {
            g_free(ibundle->path);
            g_free(ibundle->mount_point);
            g_free(ibundle);
        }
    }
    g_free(mountpoint);
    return res;
}

void r_bundle_free(RaucBundle *bundle)
{
    if (bundle == NULL)
        return;

    if (bundle->mount_point) {
        r_bundle_unmount(bundle->mount_point, NULL);
        g_free(bundle->mount_point);
    }

    if (bundle->manifest) {
        r_manifest_free(bundle->manifest);
    }

    g_free(bundle->path);
    g_free(bundle);
}

gboolean r_bundle_verify_signature(RaucBundle *bundle, GError **error)
{
    GError *ierror = NULL;
    gchar *manifest_path = NULL;
    gchar *signature_path = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    manifest_path = g_build_filename(bundle->mount_point, "manifest.raucm", NULL);
    signature_path = g_build_filename(bundle->mount_point, "manifest.raucm.sig", NULL);

    if (!g_file_test(signature_path, G_FILE_TEST_EXISTS)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                   "Bundle signature not found: '%s'", signature_path);
        goto out;
    }

    if (!r_verify_signature(manifest_path, signature_path, &ierror)) {
        g_propagate_prefixed_error(error, ierror, "Bundle signature verification failed: ");
        goto out;
    }

    res = TRUE;

out:
    g_free(manifest_path);
    g_free(signature_path);
    return res;
}

gboolean r_bundle_check_compatible(RaucBundle *bundle, GError **error)
{
    RaucContext *context = r_context_get();
    gboolean res = FALSE;

    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(bundle->manifest != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!bundle->manifest->compatible) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Bundle manifest does not specify compatible string");
        goto out;
    }

    if (!context->config->system_compatible) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "System compatible string not configured");
        goto out;
    }

    if (g_strcmp0(bundle->manifest->compatible, context->config->system_compatible) != 0) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Bundle compatible '%s' does not match system compatible '%s'",
                   bundle->manifest->compatible, context->config->system_compatible);
        goto out;
    }

    res = TRUE;

out:
    return res;
}

static gboolean verify_bundle_checksum(const gchar *bundle_path, const gchar *expected_checksum,
                                     RaucChecksumType checksum_type, GError **error)
{
    GError *ierror = NULL;
    RaucChecksum checksum = {0};
    gboolean res = FALSE;

    g_return_val_if_fail(bundle_path != NULL, FALSE);
    g_return_val_if_fail(expected_checksum != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    checksum.type = checksum_type;

    if (!r_checksum_file(bundle_path, &checksum, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    if (g_strcmp0(checksum.digest, expected_checksum) != 0) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Bundle checksum mismatch. Expected: %s, Calculated: %s",
                   expected_checksum, checksum.digest);
        goto out;
    }

    res = TRUE;

out:
    g_free(checksum.digest);
    return res;
}

gboolean r_bundle_verify_content(RaucBundle *bundle, GError **error)
{
    GError *ierror = NULL;
    GHashTableIter iter;
    gpointer key, value;
    gboolean res = FALSE;

    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(bundle->manifest != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!bundle->manifest->images) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Bundle manifest contains no images");
        goto out;
    }

    g_hash_table_iter_init(&iter, bundle->manifest->images);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucImage *image = (RaucImage*)value;
        gchar *image_path = NULL;

        image_path = g_build_filename(bundle->mount_point, image->filename, NULL);

        if (!g_file_test(image_path, G_FILE_TEST_EXISTS)) {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                       "Bundle image not found: '%s'", image_path);
            g_free(image_path);
            goto out;
        }

        if (image->checksum.digest &&
            !verify_bundle_checksum(image_path, image->checksum.digest,
                                   image->checksum.type, &ierror)) {
            g_propagate_prefixed_error(error, ierror, "Image checksum verification failed for '%s': ", image->filename);
            g_free(image_path);
            goto out;
        }

        g_free(image_path);
    }

    res = TRUE;

out:
    return res;
}

gboolean r_bundle_load_manifest(RaucBundle *bundle, GError **error)
{
    GError *ierror = NULL;
    gchar *manifest_path = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (bundle->manifest) {
        res = TRUE;
        goto out;
    }

    manifest_path = g_build_filename(bundle->mount_point, "manifest.raucm", NULL);

    if (!r_manifest_read_file(manifest_path, &bundle->manifest, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    res = TRUE;

out:
    g_free(manifest_path);
    return res;
}

gchar* r_bundle_get_image_path(RaucBundle *bundle, const gchar *slotclass, GError **error)
{
    RaucImage *image = NULL;
    gchar *image_path = NULL;

    g_return_val_if_fail(bundle != NULL, NULL);
    g_return_val_if_fail(bundle->manifest != NULL, NULL);
    g_return_val_if_fail(slotclass != NULL, NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    if (!bundle->manifest->images) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Bundle contains no images");
        return NULL;
    }

    image = g_hash_table_lookup(bundle->manifest->images, slotclass);
    if (!image) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "No image found for slot class '%s'", slotclass);
        return NULL;
    }

    image_path = g_build_filename(bundle->mount_point, image->filename, NULL);
    return image_path;
}
