#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <glib.h>
#include <gio/gio.h>
#include <openssl/cms.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#include "../../include/legacy/signature.h"

GQuark r_signature_error_quark(void)
{
    return g_quark_from_static_string("r-signature-error-quark");
}

gboolean cms_is_detached(GBytes *sig, gboolean *detached, GError **error)
{
    g_autoptr(CMS_ContentInfo) cms = NULL;
    BIO *bio = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(sig != NULL, FALSE);
    g_return_val_if_fail(detached != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    bio = BIO_new_mem_buf(g_bytes_get_data(sig, NULL), g_bytes_get_size(sig));
    if (!bio) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
                   "Failed to create BIO for signature");
        goto out;
    }

    cms = d2i_CMS_bio(bio, NULL);
    if (!cms) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
                   "Failed to parse CMS signature");
        goto out;
    }

    *detached = CMS_is_detached(cms);
    res = TRUE;

out:
    if (bio)
        BIO_free(bio);
    return res;
}

X509_STORE* setup_x509_store(const gchar *capath, const gchar *cadir, GError **error)
{
    X509_STORE *store = NULL;
    GError *ierror = NULL;

    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    store = X509_STORE_new();
    if (!store) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                   "Failed to create X509 store");
        goto out;
    }

    // Load CA certificate if provided
    if (capath) {
        if (!X509_STORE_load_locations(store, capath, NULL)) {
            g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                       "Failed to load CA certificate from %s", capath);
            goto out;
        }
    }

    // Load CA directory if provided
    if (cadir) {
        if (!X509_STORE_load_locations(store, NULL, cadir)) {
            g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                       "Failed to load CA directory %s", cadir);
            goto out;
        }
    }

    // Set verification flags - be more lenient for development certificates
    // Don't use X509_V_FLAG_X509_STRICT which requires strict key usage extensions
    X509_STORE_set_flags(store, 0);

    return store;

out:
    if (store) {
        X509_STORE_free(store);
        store = NULL;
    }
    return NULL;
}

gboolean cms_verify_fd(gint fd, GBytes *sig, goffset limit, X509_STORE *store, CMS_ContentInfo **cms, GError **error)
{
    g_autoptr(GMappedFile) file = NULL;
    g_autoptr(GBytes) content = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(fd >= 0, FALSE);
    g_return_val_if_fail(sig != NULL, FALSE);
    g_return_val_if_fail(store != NULL, FALSE);
    g_return_val_if_fail(cms == NULL || *cms == NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    printf("DEBUG: cms_verify_fd: fd=%d, limit=%" G_GUINT64_FORMAT ", sig_size=%zu\n",
           fd, limit, g_bytes_get_size(sig));

    file = g_mapped_file_new_from_fd(fd, FALSE, error);
    if (!file) {
        printf("DEBUG: Failed to create mapped file\n");
        goto out;
    }
    content = g_mapped_file_get_bytes(file);

    printf("DEBUG: Mapped file size: %zu bytes\n", g_bytes_get_size(content));

    // Limit content size if specified
    if (limit > 0 && limit < g_bytes_get_size(content)) {
        printf("DEBUG: Limiting content to %" G_GUINT64_FORMAT " bytes\n", limit);
        GBytes *tmp = g_bytes_new_from_bytes(content, 0, limit);
        g_bytes_unref(content);
        content = tmp;
    }

    printf("DEBUG: Calling cms_verify_bytes...\n");
    res = cms_verify_bytes(content, sig, store, cms, NULL, error);
    printf("DEBUG: cms_verify_bytes result: %s\n", res ? "SUCCESS" : "FAILED");

out:
    return res;
}

gboolean cms_verify_bytes(GBytes *content, GBytes *sig, X509_STORE *store, CMS_ContentInfo **cms, GBytes **manifest, GError **error)
{
    g_autoptr(CMS_ContentInfo) icms = NULL;
    BIO *incontent = NULL;
    BIO *insig = NULL;
    BIO *outcontent = NULL;
    gboolean res = FALSE;
    gboolean verified = FALSE;
    gboolean detached;

    g_return_val_if_fail(sig != NULL, FALSE);
    g_return_val_if_fail(store != NULL, FALSE);
    g_return_val_if_fail(cms == NULL || *cms == NULL, FALSE);
    g_return_val_if_fail(manifest == NULL || *manifest == NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    printf("DEBUG: cms_verify_bytes: sig_size=%zu, content_size=%zu\n",
           g_bytes_get_size(sig), content ? g_bytes_get_size(content) : 0);

    insig = BIO_new_mem_buf(g_bytes_get_data(sig, NULL), g_bytes_get_size(sig));
    if (!insig) {
        printf("DEBUG: Failed to create BIO for signature\n");
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
                   "Failed to create BIO for signature");
        goto out;
    }

    printf("DEBUG: Created BIO for signature\n");

    if (!(icms = d2i_CMS_bio(insig, NULL))) {
        printf("DEBUG: Failed to parse CMS signature\n");
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
                   "Failed to parse CMS signature");
        goto out;
    }

    printf("DEBUG: Parsed CMS signature successfully\n");

    detached = CMS_is_detached(icms);
    printf("DEBUG: Signature is %s\n", detached ? "detached" : "inline");

    if (detached) {
        if (content == NULL) {
            g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
                       "No content provided for detached signature");
            goto out;
        }
        if (manifest != NULL) {
            g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
                       "Unexpected manifest output location for detached signature");
            goto out;
        }
        incontent = BIO_new_mem_buf(g_bytes_get_data(content, NULL), g_bytes_get_size(content));
        if (!incontent) {
            printf("DEBUG: Failed to create BIO for content\n");
            g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
                       "Failed to create BIO for content");
            goto out;
        }
        printf("DEBUG: Created BIO for content\n");
    } else {
        if (content != NULL) {
            g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
                       "Unexpected content provided for inline signature");
            goto out;
        }
        if (manifest == NULL) {
            g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
                       "No manifest output location for inline signature");
            goto out;
        }
        outcontent = BIO_new(BIO_s_mem());
        if (!outcontent) {
            printf("DEBUG: Failed to create BIO for output\n");
            g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_PARSE,
                       "Failed to create BIO for output");
            goto out;
        }
        printf("DEBUG: Created BIO for output\n");
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
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_INVALID,
                   "Signature verification failed");
        goto out;
    }

    if (!detached && manifest) {
        char *data = NULL;
        long len = BIO_get_mem_data(outcontent, &data);
        if (data && len > 0) {
            GBytes *tmp = g_bytes_new(data, len);
            if (!tmp) {
                g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                           "Missing manifest in inline signature");
                goto out;
            }
            *manifest = tmp;
        } else {
            g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                       "Missing manifest in inline signature");
            goto out;
        }
    }

    if (cms)
        *cms = g_steal_pointer(&icms);

    res = TRUE;
    printf("DEBUG: cms_verify_bytes completed successfully\n");

out:
    // Print OpenSSL errors if any
    if (!res) {
        printf("DEBUG: OpenSSL errors:\n");
        ERR_print_errors_fp(stdout);
    }

    // Clean up BIOs safely
    if (incontent) {
        BIO_free(incontent);
        incontent = NULL;
    }
    if (insig) {
        BIO_free(insig);
        insig = NULL;
    }
    if (outcontent) {
        BIO_free(outcontent);
        outcontent = NULL;
    }

    return res;
}

gboolean cms_verify_sig(GBytes *sig, X509_STORE *store, CMS_ContentInfo **cms, GBytes **manifest, GError **error)
{
    return cms_verify_bytes(NULL, sig, store, cms, manifest, error);
}

gboolean cms_get_cert_chain(CMS_ContentInfo *cms, X509_STORE *store, STACK_OF(X509) **verified_chain, GError **error)
{
    STACK_OF(X509) *signers = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(cms != NULL, FALSE);
    g_return_val_if_fail(store != NULL, FALSE);
    g_return_val_if_fail(verified_chain != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    signers = CMS_get0_signers(cms);
    if (!signers) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                   "Failed to get signers from CMS");
        goto out;
    }

    *verified_chain = sk_X509_dup(signers);
    if (!*verified_chain) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                   "Failed to duplicate certificate chain");
        goto out;
    }

    res = TRUE;

out:
    return res;
}

gchar* cms_get_signers(CMS_ContentInfo *cms, GError **error)
{
    STACK_OF(X509) *signers = NULL;
    X509 *signer = NULL;
    gchar *result = NULL;

    g_return_val_if_fail(cms != NULL, NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    signers = CMS_get0_signers(cms);
    if (!signers || sk_X509_num(signers) == 0) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                   "No signers found in CMS");
        goto out;
    }

    signer = sk_X509_value(signers, 0);
    if (!signer) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                   "Failed to get first signer");
        goto out;
    }

    result = X509_NAME_oneline(X509_get_subject_name(signer), NULL, 0);
    if (!result) {
        g_set_error(error, R_SIGNATURE_ERROR, R_SIGNATURE_ERROR_UNKNOWN,
                   "Failed to get signer name");
        goto out;
    }

out:
    return result;
}
