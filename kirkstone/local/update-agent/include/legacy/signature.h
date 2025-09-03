#pragma once

#include <openssl/cms.h>
#include <openssl/x509.h>
#include <glib.h>

G_DEFINE_AUTOPTR_CLEANUP_FUNC(CMS_ContentInfo, CMS_ContentInfo_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(X509_STORE, X509_STORE_free)

#define R_SIGNATURE_ERROR r_signature_error_quark()
GQuark r_signature_error_quark(void);

typedef enum {
    R_SIGNATURE_ERROR_UNKNOWN,
    R_SIGNATURE_ERROR_PARSE,
    R_SIGNATURE_ERROR_INVALID,
} RSignatureError;

/**
 * Check whether the CMS signature is detached or not
 *
 * @param sig signature to be checked
 * @param detached return location for the detached/inline result
 * @param error return location for a GError, or NULL
 *
 * @return TRUE if succeeded, FALSE if failed
 */
gboolean cms_is_detached(GBytes *sig, gboolean *detached, GError **error)
G_GNUC_WARN_UNUSED_RESULT;

/**
 * Prepare an OpenSSL X509_STORE for signature verification.
 *
 * @param capath optional ca file path
 * @param cadir optional ca directory path
 * @param error return location for a GError, or NULL
 *
 * @return X509_STORE, NULL if failed
 */
X509_STORE* setup_x509_store(const gchar *capath, const gchar *cadir, GError **error)
G_GNUC_WARN_UNUSED_RESULT;

/**
 * Verify detached signature for given file.
 *
 * @param fd file descriptor to verify against signature
 * @param sig signature used to verify
 * @param limit size of content to use, 0 if all should be included
 * @param store X509 store to use for verification
 * @param cms Return location for the CMS_ContentInfo used for verification
 * @param error return location for a GError, or NULL
 *
 * @return TRUE if succeeded, FALSE if failed
 */
gboolean cms_verify_fd(gint fd, GBytes *sig, goffset limit, X509_STORE *store, CMS_ContentInfo **cms, GError **error)
G_GNUC_WARN_UNUSED_RESULT;

/**
 * Verify detached and inline signatures.
 *
 * @param content content to verify against signature, or NULL (for inline signature)
 * @param sig signature used to verify
 * @param store X509 store to use for verification
 * @param cms Return location for the CMS_ContentInfo used for verification
 * @param manifest return location for included manifest, or NULL (for detached signature)
 * @param error return location for a GError, or NULL
 *
 * @return TRUE if succeeded, FALSE if failed
 */
gboolean cms_verify_bytes(GBytes *content, GBytes *sig, X509_STORE *store, CMS_ContentInfo **cms, GBytes **manifest, GError **error)
G_GNUC_WARN_UNUSED_RESULT;

/**
 * Verify inline signature and return included manifest.
 *
 * @param sig signature to verify
 * @param store X509 store to use for verification
 * @param cms Return location for the CMS_ContentInfo used for verification
 * @param manifest return location for included manifest
 * @param error return location for a GError, or NULL
 *
 * @return TRUE if succeeded, FALSE if failed
 */
gboolean cms_verify_sig(GBytes *sig, X509_STORE *store, CMS_ContentInfo **cms, GBytes **manifest, GError **error)
G_GNUC_WARN_UNUSED_RESULT;

/**
 * Get infos about signer and verification chain.
 *
 * Must be called *after* cms_verify()
 *
 * @param cms CMS_ContentInfo used in cms_verify()
 * @param store Store used in cms_verify()
 * @param[out] verified_chain Return location for the verification chain, or NULL
 *                            [transfer full]
 * @param[out] error return location for a GError, or NULL
 *
 * @return TRUE if succeeded, FALSE if failed
 */
gboolean cms_get_cert_chain(CMS_ContentInfo *cms, X509_STORE *store, STACK_OF(X509) **verified_chain, GError **error)
G_GNUC_WARN_UNUSED_RESULT;

/**
 * Get signer information from CMS signature.
 *
 * @param cms CMS_ContentInfo used in cms_verify()
 * @param[out] error return location for a GError, or NULL
 *
 * @return allocated string containing signer information, or NULL if failed
 */
gchar* cms_get_signers(CMS_ContentInfo *cms, GError **error)
G_GNUC_WARN_UNUSED_RESULT;
