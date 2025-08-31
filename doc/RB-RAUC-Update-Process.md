# RAUC Update Process Documentation

## Overview

This document describes the detailed step-by-step process of RAUC (Robust Auto-Update Client) bundle installation, including bundle verification, signature validation, and image deployment to target slots.

## Process Flow

The RAUC update process consists of three main phases:
1. **Bundle Loading and Verification** (6 steps)
2. **Signature Verification** (4 steps)  
3. **Installation Process** (5 steps)

## Phase 1: Bundle Loading and Verification

### [Bundle Step 1/6] Starting bundle load and verification process
- Initialize bundle structure
- Set up bundle path and basic validation
- **Debug Output**: Bundle file path

### [Bundle Step 2/6] Opening bundle and extracting signature data
- Open bundle file and read signature data
- Extract embedded or detached signature information
- **Error Handling**: Bundle file access validation

### [Bundle Step 3/6] Loading manifest for compatibility checks
- Parse bundle manifest file
- Extract compatibility information and version data
- **Error Handling**: Manifest parsing validation

### [Bundle Step 4/6] Bundle structure loaded successfully
- Complete bundle data structure initialization
- Prepare for signature verification phase

### [Bundle Step 5/6] Creating temporary mount point for bundle
- Create temporary directory for bundle mounting
- **Debug Output**: Temporary mount point path

### [Bundle Step 6/6] Mounting bundle as read-only loop device
- Mount bundle file as loop device
- **Debug Output**: Mount command execution
- **Success Output**: Mount point confirmation

## Phase 2: Signature Verification

### [Verification Step 1/4] Starting bundle signature verification
- Initialize signature verification process
- Validate presence of signature data
- **Debug Output**: Signature data size

### [Verification Step 2/4] Analyzing signature format and type
- Determine signature type (detached vs. inline)
- Parse CMS signature structure
- **Debug Output**: Signature type identification

### [Verification Step 3/4] Loading CA certificates and setting up X509 store
- Attempt to load CA certificates from multiple paths:
  - `/etc/rauc/ca.cert.pem`
  - `/home/makepluscode/docker-yocto-kirkstone-nuc/kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/ca.cert.pem`
  - `/etc/ssl/certs/ca-certificates.crt`
- **Debug Output**: CA loading attempts and results

### [Verification Step 4/4] Performing CMS signature verification
- Execute CMS_verify operation
- Validate certificate chain
- **Success**: Signature verification completed

## Phase 3: Installation Process

### [Step 1/5] Starting installation of 'image' to slot 'name'
- Initialize installation task
- **Debug Output**: Image filename and target slot

### [Step 2/5] Verifying slot compatibility
- Check slot class compatibility with image requirements
- **Progress**: 5%

### [Step 3/5] Updating slot status to inactive
- Mark target slot as inactive before installation
- **Progress**: 10%

### [Step 4/5] Copying image data to slot
- Begin image data transfer to target slot
- **Progress**: 15% (start) to 90%+ (completion)
- **Optimized Output**: Progress reported every 10% (10%, 20%, 30%, ..., 90%, 100%)

### [Step 5/5] Finalizing installation and updating slot status
- Complete installation process
- Update slot status to "good"
- **Progress**: 98%
- **Final Success**: 100% completion message

## Debug Information

### Signature Verification Details
```
DEBUG: Starting signature verification...
DEBUG: Signature data found, size: XXXX bytes
DEBUG: Signature type: detached/inline
DEBUG: Trying CA path: /path/to/ca.cert.pem
DEBUG: Successfully loaded CA from: /path/to/ca.cert.pem
DEBUG: X509 store setup complete
```

### CMS Verification Process
```
DEBUG: Processing detached signature
DEBUG: Bundle file opened, fd: X, size: XXXXXX
DEBUG: cms_verify_fd: fd=X, limit=XXXXXX, sig_size=XXXX
DEBUG: Starting CMS_verify...
DEBUG: CMS_verify result: SUCCESS
```

### Installation Progress
```
DEBUG: Report progress every 10% or at 100%
Installing to slot 'rootfs.1': 0%
Installing to slot 'rootfs.1': 10%
Installing to slot 'rootfs.1': 20%
...
Installing to slot 'rootfs.1': 90%
Installing to slot 'rootfs.1': 100%
```

## Error Handling

### Common Error Cases
1. **Bundle file not found**: File path validation failure
2. **Signature verification failed**: Invalid signature or CA certificate issues
3. **Slot compatibility mismatch**: Image slot class doesn't match target slot
4. **Mount failure**: Bundle mounting issues
5. **Installation failure**: Image copy or slot update errors

### Error Messages Format
```
ERROR: Failed to open bundle file
ERROR: Bundle signature data not found
ERROR: Failed to determine signature type
ERROR: Failed to load CA certificate from any path
ERROR: Failed to mount bundle
```

## Configuration

### CA Certificate Paths
The system attempts to load CA certificates from these locations in order:
1. `/etc/rauc/ca.cert.pem` (primary)
2. Project-specific CA: `kirkstone/meta-nuc/recipes-core/rauc/files/ca-fixed/ca.cert.pem`
3. System CA bundle: `/etc/ssl/certs/ca-certificates.crt`

### Bundle Compatibility
- System compatible: `intel-i7-x64-nuc-rauc`
- Bundle must match this compatibility string exactly

## Usage

### Command Line Interface
```bash
# Using update-test-app
/usr/local/bin/update-test-app /path/to/bundle.raucb

# Using standard RAUC
rauc install /path/to/bundle.raucb
```

### Expected Output Flow
1. Bundle loading steps (1-6/6)
2. Verification steps (1-4/4)  
3. Installation steps (1-5/5)
4. Progress updates (0%, 10%, 20%, ..., 100%)
5. Success confirmation

## Implementation Details

### Key Components
- **Bundle Loader**: `r_bundle_load()` in `bundle.c`
- **Signature Verifier**: `r_bundle_verify_signature()` in `bundle.c`
- **CMS Verification**: `cms_verify_fd()` and `cms_verify_bytes()` in `signature.c`
- **Installation Engine**: `install_image_to_slot()` in `install.c`

### Code Snippets

#### Bundle Loading (`update-library/src/rauc/bundle.c`)
```c
gboolean r_bundle_load(const gchar *bundlename, RaucBundle **bundle, GError **error)
{
    gboolean res = FALSE;
    RaucBundle *ibundle = NULL;

    g_print("[Bundle Step 1/6] Starting bundle load and verification process\n");
    g_print("DEBUG: Bundle file: %s\n", bundlename);

    // Create bundle structure
    ibundle = g_new0(RaucBundle, 1);
    ibundle->path = g_strdup(bundlename);

    g_print("[Bundle Step 2/6] Opening bundle and extracting signature data\n");
    // Open bundle and extract signature data
    if (!open_local_bundle(ibundle, error)) {
        g_print("ERROR: Failed to open bundle file\n");
        g_free(ibundle->path);
        g_free(ibundle);
        goto out;
    }

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
```

#### Bundle Mounting (`update-library/src/rauc/bundle.c`)
```c
gboolean r_bundle_mount(const gchar *bundlename, gchar **mountpoint, GError **error)
{
    GError *ierror = NULL;
    gchar *tmpdir = NULL;
    gchar *mount_cmd = NULL;
    gboolean res = FALSE;

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
    // Cleanup code...
    return res;
}
```

#### Signature Verification (`update-library/src/rauc/bundle.c`)
```c
gboolean r_bundle_verify_signature(RaucBundle *bundle, GError **error)
{
    GError *ierror = NULL;
    gboolean res = FALSE;
    gboolean detached = FALSE;
    g_autoptr(CMS_ContentInfo) cms = NULL;
    g_autoptr(X509_STORE) store = NULL;

    g_print("[Verification Step 1/4] Starting bundle signature verification\n");
    printf("DEBUG: Starting signature verification...\n");

    // Check if we have signature data from the bundle
    if (!bundle->sigdata) {
        g_print("ERROR: Bundle signature data not found\n");
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                   "Bundle signature data not found");
        goto out;
    }

    printf("DEBUG: Signature data found, size: %zu bytes\n", g_bytes_get_size(bundle->sigdata));

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
            printf("DEBUG: Failed to load CA from: %s - %s\n", ca_paths[i], 
                   ierror ? ierror->message : "unknown error");
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

    // Perform actual CMS verification...
    // (verification logic continues)

out:
    return res;
}
```

#### CMS Verification (`update-library/src/rauc/signature.c`)
```c
gboolean cms_verify_fd(gint fd, GBytes *sig, goffset limit, X509_STORE *store, CMS_ContentInfo **cms, GError **error)
{
    g_autoptr(GMappedFile) file = NULL;
    g_autoptr(GBytes) content = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(fd >= 0, FALSE);
    g_return_val_if_fail(sig != NULL, FALSE);
    g_return_val_if_fail(store != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    printf("DEBUG: cms_verify_fd: fd=%d, limit=%" G_GUINT64_FORMAT ", sig_size=%zu\n",
           fd, limit, g_bytes_get_size(sig));

    file = g_mapped_file_new_from_fd(fd, FALSE, error);
    if (!file) {
        return FALSE;
    }

    printf("DEBUG: Mapped file size: %zu bytes\n", g_mapped_file_get_length(file));

    if (limit > 0 && limit < g_mapped_file_get_length(file)) {
        printf("DEBUG: Limiting content to %" G_GUINT64_FORMAT " bytes\n", limit);
        content = g_bytes_new(g_mapped_file_get_contents(file), limit);
    } else {
        content = g_bytes_new(g_mapped_file_get_contents(file), g_mapped_file_get_length(file));
    }

    printf("DEBUG: Calling cms_verify_bytes...\n");
    res = cms_verify_bytes(content, sig, store, cms, NULL, error);
    printf("DEBUG: cms_verify_bytes result: %s\n", res ? "SUCCESS" : "FAILED");

out:
    return res;
}
```

```c
gboolean cms_verify_bytes(GBytes *content, GBytes *sig, X509_STORE *store, CMS_ContentInfo **cms, GBytes **manifest, GError **error)
{
    g_autoptr(CMS_ContentInfo) icms = NULL;
    BIO *incontent = NULL;
    BIO *insig = NULL;
    BIO *outcontent = NULL;
    gboolean res = FALSE;
    gboolean detached = FALSE;
    int verified = 0;

    printf("DEBUG: cms_verify_bytes: sig_size=%zu, content_size=%zu\n",
           g_bytes_get_size(sig), content ? g_bytes_get_size(content) : 0);

    insig = BIO_new_mem_buf(g_bytes_get_data(sig, NULL), g_bytes_get_size(sig));
    if (!insig) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to create signature BIO");
        goto out;
    }

    printf("DEBUG: Created BIO for signature\n");

    icms = d2i_CMS_bio(insig, NULL);
    if (!icms) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to parse CMS signature");
        goto out;
    }

    printf("DEBUG: Parsed CMS signature successfully\n");

    detached = (CMS_is_detached(icms) == 1);
    printf("DEBUG: Signature is %s\n", detached ? "detached" : "embedded");

    if (content && detached) {
        incontent = BIO_new_mem_buf(g_bytes_get_data(content, NULL), g_bytes_get_size(content));
        if (!incontent) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to create content BIO");
            goto out;
        }
        printf("DEBUG: Created BIO for content\n");
    }

    if (!detached) {
        outcontent = BIO_new(BIO_s_mem());
        if (!outcontent) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to create output BIO");
            goto out;
        }
    }

    printf("DEBUG: Starting CMS_verify...\n");
    if (detached) {
        verified = CMS_verify(icms, NULL, store, incontent, NULL, CMS_DETACHED | CMS_BINARY);
    } else {
        verified = CMS_verify(icms, NULL, store, NULL, outcontent, CMS_BINARY);
    }

    printf("DEBUG: CMS_verify result: %s\n", verified ? "SUCCESS" : "FAILED");

    if (!verified) {
        printf("DEBUG: Signature verification failed\n");
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "CMS signature verification failed");
        goto out;
    }

    printf("DEBUG: cms_verify_bytes completed successfully\n");

    if (cms) {
        *cms = g_steal_pointer(&icms);
    }

    res = TRUE;

out:
    // Cleanup BIOs...
    return res;
}
```

#### Installation Process (`update-library/src/rauc/install.c`)
```c
static gboolean install_image_to_slot(InstallTask *task,
                                    RaucProgressCallback progress_callback, gpointer user_data,
                                    GError **error)
{
    GError *ierror = NULL;
    gboolean res = FALSE;

    if (progress_callback) {
        gchar *message = g_strdup_printf("[Step 1/5] Starting installation of '%s' to slot '%s'",
                                       task->image->filename, task->slot->name);
        progress_callback(0, message, 0, user_data);
        g_free(message);
    }

    if (progress_callback) {
        gchar *message = g_strdup_printf("[Step 2/5] Verifying slot compatibility");
        progress_callback(5, message, 0, user_data);
        g_free(message);
    }

    if (!verify_slot_compatible(task->slot, task->image, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

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

    res = TRUE;

out:
    return res;
}
```

#### Progress Reporting Optimization (`update-library/src/rauc/install.c`)
```c
static gboolean copy_image_to_slot(const gchar *image_path, RaucSlot *slot,
                                 RaucProgressCallback progress_callback, gpointer user_data,
                                 GError **error)
{
    // ... file opening and setup code ...

    while ((bytes_read = read(image_fd, buffer, sizeof(buffer))) > 0) {
        gchar *write_ptr = buffer;
        size_t remaining = bytes_read;

        while (remaining > 0) {
            ssize_t bytes_written = write(slot_fd, write_ptr, remaining);
            if (bytes_written < 0) {
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

    // ... cleanup code ...
}
```

### Progress Reporting Optimization
- **Original**: Every percentage change reported (1%, 2%, 3%, ...)
- **Optimized**: Only 10% increments + 100% completion (0%, 10%, 20%, ..., 90%, 100%)
- **Logic**: `(percentage / 10) != (last_reported / 10)` ensures reporting only when crossing 10% boundaries
- **Special Case**: `percentage == 100 && last_reported != 100` ensures 100% completion is always reported
- **Result**: Reduces verbose output while maintaining visibility

## Troubleshooting

### Debug Mode
Enable detailed debugging by checking console output for:
- File access permissions
- CA certificate loading status
- Signature verification details
- Mount point creation
- Installation progress

### Common Solutions
1. **CA Certificate Issues**: Ensure proper CA certificate is available
2. **Compatibility Problems**: Verify bundle compatibility string matches system
3. **Permission Errors**: Check file permissions and mount capabilities
4. **Network Issues**: For remote deployments, verify SSH connectivity

## Related Documentation
- RAUC Official Documentation: https://rauc.readthedocs.io/
- Bundle Creation Guide: `tools/bundler/README.md`
- Deployment Scripts: `kirkstone/local/update-library/deploy.sh`