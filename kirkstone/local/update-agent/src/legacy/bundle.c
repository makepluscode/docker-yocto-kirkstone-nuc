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
#include <gio/gio.h>
#include <openssl/cms.h>
#include <openssl/x509.h>

#include "../../include/legacy/context.h"
#include "../../include/legacy/bundle.h"
#include "../../include/legacy/utils.h"
#include "../../include/legacy/checksum.h"
#include "../../include/legacy/signature.h"

static gchar *bundle_mount_point = NULL;

// Forward declarations
static gboolean r_bundle_mount(const gchar *bundlename, gchar **mountpoint, GError **error);
static gboolean r_bundle_unmount(const gchar *mountpoint, GError **error);

#define MAX_BUNDLE_SIGNATURE_SIZE (64 * 1024)  // 64KB max signature size

static gboolean open_local_bundle(RaucBundle *bundle, GError **error)
{
    gboolean res = FALSE;
    GError *ierror = NULL;
    g_autoptr(GFile) bundlefile = NULL;
    g_autoptr(GFileInfo) bundleinfo = NULL;
    guint64 sigsize;
    goffset offset;
    gchar *mountpoint = NULL;

    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    g_assert_null(bundle->stream);

    // Mount bundle for manifest access
    if (!r_bundle_mount(bundle->path, &mountpoint, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    bundle->mount_point = g_strdup(mountpoint);

    bundlefile = g_file_new_for_path(bundle->path);
    bundle->stream = G_INPUT_STREAM(g_file_read(bundlefile, NULL, &ierror));
    if (bundle->stream == NULL) {
        g_propagate_prefixed_error(
                error,
                ierror,
                "Failed to open bundle for reading: ");
        res = FALSE;
        goto out;
    }

    bundleinfo = g_file_input_stream_query_info(
            G_FILE_INPUT_STREAM(bundle->stream),
            G_FILE_ATTRIBUTE_STANDARD_TYPE,
            NULL, &ierror);
    if (bundleinfo == NULL) {
        g_propagate_prefixed_error(
                error,
                ierror,
                "Failed to query bundle file info: ");
        res = FALSE;
        goto out;
    }

    if (g_file_info_get_file_type(bundleinfo) != G_FILE_TYPE_REGULAR) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                "Bundle is not a regular file");
        res = FALSE;
        goto out;
    }

    // Read signature size from end of file
    offset = sizeof(sigsize);
    res = g_seekable_seek(G_SEEKABLE(bundle->stream),
            -offset, G_SEEK_END, NULL, &ierror);
    if (!res) {
        g_propagate_prefixed_error(
                error,
                ierror,
                "Failed to seek to end of bundle: ");
        goto out;
    }
    offset = g_seekable_tell(G_SEEKABLE(bundle->stream));

    // Read signature size (8 bytes at end of file)
    guchar sigsize_buffer[8];
    gsize bytes_read = 0;
    res = g_input_stream_read_all(bundle->stream, sigsize_buffer, 8, &bytes_read, NULL, &ierror);
    if (!res || bytes_read != 8) {
        g_propagate_prefixed_error(
                error,
                ierror,
                "Failed to read signature size from bundle: ");
        goto out;
    }

    // Convert bytes to uint64 (RAUC uses network byte order - big-endian)
    // Use the same method as original RAUC: GUINT64_FROM_BE
    sigsize = ((guint64)sigsize_buffer[0] << 56) |
              ((guint64)sigsize_buffer[1] << 48) |
              ((guint64)sigsize_buffer[2] << 40) |
              ((guint64)sigsize_buffer[3] << 32) |
              ((guint64)sigsize_buffer[4] << 24) |
              ((guint64)sigsize_buffer[5] << 16) |
              ((guint64)sigsize_buffer[6] << 8)  |
              ((guint64)sigsize_buffer[7]);

    if (sigsize == 0) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                "Signature size is 0");
        res = FALSE;
        goto out;
    }

    // Sanity check: signature should be smaller than bundle size
    if (sigsize > (guint64)offset) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                "Signature size (%"G_GUINT64_FORMAT ") exceeds bundle size", sigsize);
        res = FALSE;
        goto out;
    }

    // Sanity check: signature should be smaller than 64KiB
    if (sigsize > MAX_BUNDLE_SIGNATURE_SIZE) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                "Signature size (%"G_GUINT64_FORMAT ") exceeds 64KiB", sigsize);
        res = FALSE;
        goto out;
    }

    // Calculate bundle content size (excluding signature)
    offset -= sigsize;
    bundle->size = offset;

    // Seek to start of signature
    res = g_seekable_seek(G_SEEKABLE(bundle->stream),
            offset, G_SEEK_SET, NULL, &ierror);
    if (!res) {
        g_propagate_prefixed_error(
                error,
                ierror,
                "Failed to seek to start of bundle signature: ");
        goto out;
    }

    // Read signature data
    guchar *sigdata_buffer = g_malloc(sigsize);
    gsize sig_bytes_read = 0;
    res = g_input_stream_read_all(bundle->stream, sigdata_buffer, sigsize, &sig_bytes_read, NULL, &ierror);
    if (!res || sig_bytes_read != sigsize) {
        g_free(sigdata_buffer);
        g_propagate_prefixed_error(
                error,
                ierror,
                "Failed to read signature from bundle: ");
        goto out;
    }

    bundle->sigdata = g_bytes_new_take(sigdata_buffer, sigsize);
    g_info("Bundle signature data loaded successfully, size: %zu bytes", sigsize);

    res = TRUE;

out:
    if (!res && mountpoint) {
        r_bundle_unmount(mountpoint, NULL);
        g_free(mountpoint);
    }
    return res;
}

gboolean r_bundle_load(const gchar *bundlename, RaucBundle **bundle, GError **error)
{
    gboolean res = FALSE;
    RaucBundle *ibundle = NULL;

    g_return_val_if_fail(bundlename != NULL, FALSE);
    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    g_print("[Bundle Step 1/6] Starting bundle load and verification process\n");
    g_print("DEBUG: Bundle file: %s\n", bundlename);

    // Create bundle structure
    ibundle = g_new0(RaucBundle, 1);
    ibundle->path = g_strdup(bundlename);

    g_info("Opening bundle and extracting signature data");
    // Open bundle and extract signature data
    if (!open_local_bundle(ibundle, error)) {
        g_critical("Failed to open bundle file: %s",
                   error && *error ? (*error)->message : "unknown error");
        g_free(ibundle->path);
        g_free(ibundle);
        goto out;
    }
    g_info("Bundle opened successfully, signature data extracted");

    g_print("[Bundle Step 3/6] Loading manifest for compatibility checks\n");
    // Load manifest for compatibility checks
    if (!r_bundle_load_manifest(ibundle, error)) {
        g_print("ERROR: Failed to load bundle manifest\n");
        g_free(ibundle->path);
        g_free(ibundle);
        goto out;
    }

    g_print("[Bundle Step 4/6] Bundle structure loaded successfully\n");
    *bundle = ibundle;
    res = TRUE;

out:
    return res;
}

gboolean r_bundle_mount(const gchar *bundlename, gchar **mountpoint, GError **error)
{
    GError *ierror = NULL;
    gchar *tmpdir = NULL;
    gchar *mount_cmd = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(bundlename != NULL, FALSE);
    g_return_val_if_fail(mountpoint != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    g_print("[Bundle Step 5/6] Creating temporary mount point for bundle\n");
    tmpdir = g_dir_make_tmp("rauc-bundle-XXXXXX", &ierror);
    if (tmpdir == NULL) {
        g_print("ERROR: Failed to create temporary directory\n");
        g_propagate_prefixed_error(error, ierror, "Failed to create temporary directory: ");
        goto out;
    }
    g_print("DEBUG: Created temporary mount point: %s\n", tmpdir);

    g_print("[Bundle Step 6/6] Mounting bundle as read-only loop device\n");
    mount_cmd = g_strdup_printf("mount -o loop,ro '%s' '%s'", bundlename, tmpdir);
    printf("DEBUG: Executing mount command: %s\n", mount_cmd);

    if (!r_subprocess_new(mount_cmd, NULL, &ierror)) {
        g_print("ERROR: Failed to mount bundle\n");
        g_propagate_prefixed_error(error, ierror, "Failed to mount bundle: ");
        goto out;
    }

    g_print("✓ Bundle mounted successfully at: %s\n", tmpdir);
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
    gboolean res = FALSE;
    gboolean detached = FALSE;
    g_autoptr(CMS_ContentInfo) cms = NULL;
    g_autoptr(X509_STORE) store = NULL;

    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    g_print("[Verification Step 1/4] Starting bundle signature verification\n");
    printf("DEBUG: Starting signature verification...\n");

    // Check if we have signature data from the bundle
    if (!bundle->sigdata) {
        g_critical("Bundle signature data not found - bundle may not be properly loaded");
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                   "Bundle signature data not found");
        goto out;
    }

    g_info("Signature data found, size: %zu bytes", g_bytes_get_size(bundle->sigdata));

    g_print("[Verification Step 2/4] Analyzing signature format and type\n");
    // Determine if this is a detached signature
    if (!cms_is_detached(bundle->sigdata, &detached, &ierror)) {
        g_print("ERROR: Failed to determine signature type\n");
        g_propagate_prefixed_error(error, ierror, "Failed to determine signature type: ");
        goto out;
    }

    printf("DEBUG: Signature type: %s\n", detached ? "detached" : "inline");

    g_print("[Verification Step 3/4] Loading CA certificates and setting up X509 store\n");
    // Set up X509 store for verification (try multiple paths)
    const char* ca_paths[] = {
        "/etc/rauc/ca.cert.pem",
        "/home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/ca.cert.pem",
        "/etc/ssl/certs/ca-certificates.crt",
        NULL
    };

    store = NULL;
    for (int i = 0; ca_paths[i] != NULL; i++) {
        printf("DEBUG: Trying CA path: %s\n", ca_paths[i]);
        store = setup_x509_store(ca_paths[i], NULL, &ierror);
        if (store) {
            printf("DEBUG: Successfully loaded CA from: %s\n", ca_paths[i]);
            break;
        } else {
            printf("DEBUG: Failed to load CA from: %s - %s\n", ca_paths[i], ierror ? ierror->message : "unknown error");
            if (ierror) {
                g_error_free(ierror);
                ierror = NULL;
            }
        }
    }

    if (!store) {
        g_print("ERROR: Failed to load CA certificate from any path\n");
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                   "Failed to load CA certificate from any path");
        goto out;
    }

    printf("DEBUG: X509 store setup complete\n");
    g_print("[Verification Step 4/4] Performing CMS signature verification\n");
    g_message("Verifying bundle signature... ");

    if (detached) {
        printf("DEBUG: Processing detached signature\n");
        // For detached signatures, verify against the bundle file itself
        int fd = open(bundle->path, O_RDONLY);

        if (fd < 0) {
            g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       "Failed to open bundle file for verification: %s", g_strerror(errno));
            goto out;
        }

        printf("DEBUG: Bundle file opened, fd: %d, size: %" G_GUINT64_FORMAT "\n", fd, bundle->size);

        // Verify the detached signature against the bundle content
        res = cms_verify_fd(fd, bundle->sigdata, bundle->size, store, &cms, &ierror);
        close(fd);  // Close file descriptor

        printf("DEBUG: cms_verify_fd result: %s\n", res ? "SUCCESS" : "FAILED");

        if (!res) {
            g_propagate_prefixed_error(error, ierror, "Bundle signature verification failed: ");
            goto out;
        }
    } else {
        printf("DEBUG: Processing inline signature\n");
        // For inline signatures, verify the signature and extract manifest
        g_autoptr(GBytes) manifest_bytes = NULL;

        res = cms_verify_sig(bundle->sigdata, store, &cms, &manifest_bytes, &ierror);
        if (!res) {
            g_propagate_prefixed_error(error, ierror, "Bundle signature verification failed: ");
            goto out;
        }
    }

    printf("DEBUG: Basic signature verification completed\n");

    // Get certificate chain for verification
    STACK_OF(X509) *verified_chain = NULL;
    res = cms_get_cert_chain(cms, store, &verified_chain, &ierror);
    if (!res) {
        g_propagate_prefixed_error(error, ierror, "Failed to get certificate chain: ");
        goto out;
    }

    printf("DEBUG: Certificate chain verification completed\n");

    // Log verification success
    g_autofree gchar *signers = cms_get_signers(cms, &ierror);
    if (signers) {
        g_message("Verified %s signature by %s", detached ? "detached" : "inline", signers);
    }

    printf("DEBUG: Signature verification completed successfully\n");
    res = TRUE;

out:
    return res;
}

gboolean r_bundle_check_compatible(RaucBundle *bundle, GError **error)
{
    RaucContext *context = r_context_get();
    gboolean res = FALSE;

    g_return_val_if_fail(bundle != NULL, FALSE);
    g_return_val_if_fail(bundle->manifest != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    printf("DEBUG: r_bundle_check_compatible called\n");

    if (!context) {
        printf("DEBUG: Context is NULL!\n");
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "RAUC context not initialized");
        goto out;
    }

    if (!context->config) {
        printf("DEBUG: Context config is NULL!\n");
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "RAUC context config not initialized");
        goto out;
    }

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
