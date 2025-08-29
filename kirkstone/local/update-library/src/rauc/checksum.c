#include "../../include/rauc/checksum.h"
#include "../../include/rauc/utils.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef WITH_DLT
#include <dlt/dlt.h>
DLT_DECLARE_CONTEXT(dlt_context_checksum);
static gboolean dlt_initialized = FALSE;
#endif

#define R_CHECKSUM_ERROR r_checksum_error_quark()

GQuark r_checksum_error_quark(void) {
    return g_quark_from_static_string("r-checksum-error-quark");
}

static void ensure_dlt_init(void) {
#ifdef WITH_DLT
    if (!dlt_initialized) {
        DLT_REGISTER_CONTEXT(dlt_context_checksum, "RSUM", "RAUC Checksum");
        dlt_initialized = TRUE;
    }
#endif
}

const gchar *r_checksum_type_to_string(RaucChecksumType type) {
    switch (type) {
        case R_CHECKSUM_MD5:
            return "md5";
        case R_CHECKSUM_SHA1:
            return "sha1";
        case R_CHECKSUM_SHA256:
            return "sha256";
        case R_CHECKSUM_SHA512:
            return "sha512";
        case R_CHECKSUM_NONE:
        default:
            return "none";
    }
}

RaucChecksumType r_checksum_type_from_string(const gchar *type_str) {
    if (!type_str) {
        return R_CHECKSUM_NONE;
    }

    if (g_strcmp0(type_str, "md5") == 0) {
        return R_CHECKSUM_MD5;
    } else if (g_strcmp0(type_str, "sha1") == 0) {
        return R_CHECKSUM_SHA1;
    } else if (g_strcmp0(type_str, "sha256") == 0) {
        return R_CHECKSUM_SHA256;
    } else if (g_strcmp0(type_str, "sha512") == 0) {
        return R_CHECKSUM_SHA512;
    }

    return R_CHECKSUM_NONE;
}

static const EVP_MD *r_checksum_type_to_evp_md(RaucChecksumType type) {
    switch (type) {
        case R_CHECKSUM_MD5:
            return EVP_md5();
        case R_CHECKSUM_SHA1:
            return EVP_sha1();
        case R_CHECKSUM_SHA256:
            return EVP_sha256();
        case R_CHECKSUM_SHA512:
            return EVP_sha512();
        case R_CHECKSUM_NONE:
        default:
            return NULL;
    }
}

RaucChecksum *r_checksum_new(RaucChecksumType type) {
    RaucChecksum *checksum = g_new0(RaucChecksum, 1);
    checksum->type = type;
    checksum->digest = NULL;
    checksum->size = 0;

    return checksum;
}

void r_checksum_free(RaucChecksum *checksum) {
    if (!checksum) {
        return;
    }

    g_clear_pointer(&checksum->digest, g_free);
    g_free(checksum);
}

RaucChecksum *r_checksum_copy(const RaucChecksum *checksum) {
    if (!checksum) {
        return NULL;
    }

    RaucChecksum *copy = r_checksum_new(checksum->type);
    copy->size = checksum->size;

    if (checksum->digest) {
        copy->digest = g_strdup(checksum->digest);
    }

    return copy;
}

void r_checksum_clear(RaucChecksum *checksum) {
    if (!checksum) {
        return;
    }

    checksum->type = R_CHECKSUM_NONE;
    g_clear_pointer(&checksum->digest, g_free);
    checksum->size = 0;
}

gboolean r_checksum_is_set(const RaucChecksum *checksum) {
    return checksum &&
           checksum->type != R_CHECKSUM_NONE &&
           checksum->digest != NULL;
}

gboolean r_checksum_equal(const RaucChecksum *a, const RaucChecksum *b) {
    if (!a && !b) {
        return TRUE;
    }

    if (!a || !b) {
        return FALSE;
    }

    if (a->type != b->type) {
        return FALSE;
    }

    if (a->size != b->size) {
        return FALSE;
    }

    return g_strcmp0(a->digest, b->digest) == 0;
}

gboolean r_checksum_file(const gchar *filename, RaucChecksum *checksum, GError **error) {
    ensure_dlt_init();

    if (!filename || !checksum) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_FORMAT,
                   "Invalid filename or checksum");
        return FALSE;
    }

    if (checksum->type == R_CHECKSUM_NONE) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_TYPE,
                   "Invalid checksum type");
        return FALSE;
    }

    GFile *file = g_file_new_for_path(filename);
    GFileInputStream *stream = g_file_read(file, NULL, error);
    g_object_unref(file);

    if (!stream) {
        r_error("Failed to open file for checksum: %s", filename);
        return FALSE;
    }

    gboolean result = r_checksum_stream(G_INPUT_STREAM(stream), checksum, error);
    g_object_unref(stream);

    if (result) {
        r_debug("Calculated %s checksum for file: %s",
               r_checksum_type_to_string(checksum->type), filename);
    }

    return result;
}

gboolean r_checksum_memory(const guchar *data, gsize length, RaucChecksum *checksum, GError **error) {
    ensure_dlt_init();

    if (!data || !checksum) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_FORMAT,
                   "Invalid data or checksum");
        return FALSE;
    }

    const EVP_MD *md = r_checksum_type_to_evp_md(checksum->type);
    if (!md) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_TYPE,
                   "Unsupported checksum type: %d", checksum->type);
        return FALSE;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to create digest context");
        return FALSE;
    }

    if (!EVP_DigestInit_ex(ctx, md, NULL)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to initialize digest");
        EVP_MD_CTX_free(ctx);
        return FALSE;
    }

    if (!EVP_DigestUpdate(ctx, data, length)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to update digest");
        EVP_MD_CTX_free(ctx);
        return FALSE;
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    if (!EVP_DigestFinal_ex(ctx, digest, &digest_len)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to finalize digest");
        EVP_MD_CTX_free(ctx);
        return FALSE;
    }

    EVP_MD_CTX_free(ctx);

    // Convert to hex string
    GString *hex_string = g_string_sized_new(digest_len * 2);
    for (unsigned int i = 0; i < digest_len; i++) {
        g_string_append_printf(hex_string, "%02x", digest[i]);
    }

    g_free(checksum->digest);
    checksum->digest = g_string_free(hex_string, FALSE);
    checksum->size = length;

    r_debug("Calculated %s checksum for %zu bytes: %s",
           r_checksum_type_to_string(checksum->type), length, checksum->digest);

    return TRUE;
}

gboolean r_checksum_fd(int fd, RaucChecksum *checksum, GError **error) {
    ensure_dlt_init();

    if (fd < 0 || !checksum) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_FORMAT,
                   "Invalid file descriptor or checksum");
        return FALSE;
    }

    const EVP_MD *md = r_checksum_type_to_evp_md(checksum->type);
    if (!md) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_TYPE,
                   "Unsupported checksum type: %d", checksum->type);
        return FALSE;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to create digest context");
        return FALSE;
    }

    if (!EVP_DigestInit_ex(ctx, md, NULL)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to initialize digest");
        EVP_MD_CTX_free(ctx);
        return FALSE;
    }

    guchar buffer[8192];
    gssize bytes_read;
    gsize total_bytes = 0;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        if (!EVP_DigestUpdate(ctx, buffer, bytes_read)) {
            g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                       "Failed to update digest");
            EVP_MD_CTX_free(ctx);
            return FALSE;
        }
        total_bytes += bytes_read;
    }

    if (bytes_read < 0) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_FILE_ACCESS,
                   "Failed to read from file descriptor");
        EVP_MD_CTX_free(ctx);
        return FALSE;
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    if (!EVP_DigestFinal_ex(ctx, digest, &digest_len)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to finalize digest");
        EVP_MD_CTX_free(ctx);
        return FALSE;
    }

    EVP_MD_CTX_free(ctx);

    // Convert to hex string
    GString *hex_string = g_string_sized_new(digest_len * 2);
    for (unsigned int i = 0; i < digest_len; i++) {
        g_string_append_printf(hex_string, "%02x", digest[i]);
    }

    g_free(checksum->digest);
    checksum->digest = g_string_free(hex_string, FALSE);
    checksum->size = total_bytes;

    r_debug("Calculated %s checksum for fd %d (%zu bytes): %s",
           r_checksum_type_to_string(checksum->type), fd, total_bytes, checksum->digest);

    return TRUE;
}

gboolean r_checksum_stream(GInputStream *stream, RaucChecksum *checksum, GError **error) {
    ensure_dlt_init();

    if (!stream || !checksum) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_FORMAT,
                   "Invalid stream or checksum");
        return FALSE;
    }

    const EVP_MD *md = r_checksum_type_to_evp_md(checksum->type);
    if (!md) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_TYPE,
                   "Unsupported checksum type: %d", checksum->type);
        return FALSE;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to create digest context");
        return FALSE;
    }

    if (!EVP_DigestInit_ex(ctx, md, NULL)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to initialize digest");
        EVP_MD_CTX_free(ctx);
        return FALSE;
    }

    guchar buffer[8192];
    gssize bytes_read;
    gsize total_bytes = 0;

    while ((bytes_read = g_input_stream_read(stream, buffer, sizeof(buffer), NULL, error)) > 0) {
        if (!EVP_DigestUpdate(ctx, buffer, bytes_read)) {
            g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                       "Failed to update digest");
            EVP_MD_CTX_free(ctx);
            return FALSE;
        }
        total_bytes += bytes_read;
    }

    if (bytes_read < 0) {
        EVP_MD_CTX_free(ctx);
        return FALSE;  // error already set by g_input_stream_read
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    if (!EVP_DigestFinal_ex(ctx, digest, &digest_len)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to finalize digest");
        EVP_MD_CTX_free(ctx);
        return FALSE;
    }

    EVP_MD_CTX_free(ctx);

    // Convert to hex string
    GString *hex_string = g_string_sized_new(digest_len * 2);
    for (unsigned int i = 0; i < digest_len; i++) {
        g_string_append_printf(hex_string, "%02x", digest[i]);
    }

    g_free(checksum->digest);
    checksum->digest = g_string_free(hex_string, FALSE);
    checksum->size = total_bytes;

    r_debug("Calculated %s checksum for stream (%zu bytes): %s",
           r_checksum_type_to_string(checksum->type), total_bytes, checksum->digest);

    return TRUE;
}

void r_checksum_set_digest(RaucChecksum *checksum, const gchar *digest) {
    if (!checksum || !digest) {
        return;
    }

    g_free(checksum->digest);
    checksum->digest = g_strdup(digest);
}

const gchar *r_checksum_get_digest(const RaucChecksum *checksum) {
    if (!checksum) {
        return NULL;
    }

    return checksum->digest;
}

gchar *r_checksum_to_string(const RaucChecksum *checksum) {
    if (!checksum || !r_checksum_is_set(checksum)) {
        return g_strdup("none");
    }

    return g_strdup_printf("%s:%s",
                          r_checksum_type_to_string(checksum->type),
                          checksum->digest);
}

gboolean r_checksum_from_string(const gchar *checksum_str, RaucChecksum *checksum, GError **error) {
    if (!checksum_str || !checksum) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_FORMAT,
                   "Invalid checksum string or checksum");
        return FALSE;
    }

    gchar **parts = g_strsplit(checksum_str, ":", 2);
    if (!parts || !parts[0] || !parts[1]) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_FORMAT,
                   "Invalid checksum format: %s", checksum_str);
        g_strfreev(parts);
        return FALSE;
    }

    RaucChecksumType type = r_checksum_type_from_string(parts[0]);
    if (type == R_CHECKSUM_NONE) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_TYPE,
                   "Unknown checksum type: %s", parts[0]);
        g_strfreev(parts);
        return FALSE;
    }

    checksum->type = type;
    g_free(checksum->digest);
    checksum->digest = g_strdup(parts[1]);

    g_strfreev(parts);
    return TRUE;
}

gboolean r_checksum_verify_file(const gchar *filename, const RaucChecksum *expected_checksum, GError **error) {
    if (!filename || !expected_checksum || !r_checksum_is_set(expected_checksum)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_FORMAT,
                   "Invalid filename or expected checksum");
        return FALSE;
    }

    RaucChecksum *actual_checksum = r_checksum_new(expected_checksum->type);
    gboolean calc_result = r_checksum_file(filename, actual_checksum, error);

    if (!calc_result) {
        r_checksum_free(actual_checksum);
        return FALSE;
    }

    gboolean match = r_checksum_equal(actual_checksum, expected_checksum);

    if (!match) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_MISMATCH,
                   "Checksum mismatch for file %s: expected %s, got %s",
                   filename, expected_checksum->digest, actual_checksum->digest);
    }

    r_checksum_free(actual_checksum);
    return match;
}

gboolean r_checksum_verify_memory(const guchar *data, gsize length, const RaucChecksum *expected_checksum, GError **error) {
    if (!data || !expected_checksum || !r_checksum_is_set(expected_checksum)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_FORMAT,
                   "Invalid data or expected checksum");
        return FALSE;
    }

    RaucChecksum *actual_checksum = r_checksum_new(expected_checksum->type);
    gboolean calc_result = r_checksum_memory(data, length, actual_checksum, error);

    if (!calc_result) {
        r_checksum_free(actual_checksum);
        return FALSE;
    }

    gboolean match = r_checksum_equal(actual_checksum, expected_checksum);

    if (!match) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_MISMATCH,
                   "Checksum mismatch for memory data: expected %s, got %s",
                   expected_checksum->digest, actual_checksum->digest);
    }

    r_checksum_free(actual_checksum);
    return match;
}

// 점진적 체크섬 계산 함수들

RaucChecksumContext *r_checksum_context_new(RaucChecksumType type, GError **error) {
    const EVP_MD *md = r_checksum_type_to_evp_md(type);
    if (!md) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_TYPE,
                   "Unsupported checksum type: %d", type);
        return NULL;
    }

    RaucChecksumContext *ctx = g_new0(RaucChecksumContext, 1);
    ctx->md_ctx = EVP_MD_CTX_new();
    ctx->type = type;
    ctx->bytes_processed = 0;

    if (!ctx->md_ctx || !EVP_DigestInit_ex(ctx->md_ctx, md, NULL)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to initialize digest context");
        r_checksum_context_free(ctx);
        return NULL;
    }

    return ctx;
}

gboolean r_checksum_context_update(RaucChecksumContext *ctx, const guchar *data, gsize length, GError **error) {
    if (!ctx || !data) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_FORMAT,
                   "Invalid context or data");
        return FALSE;
    }

    if (!EVP_DigestUpdate(ctx->md_ctx, data, length)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to update digest");
        return FALSE;
    }

    ctx->bytes_processed += length;
    return TRUE;
}

gboolean r_checksum_context_finalize(RaucChecksumContext *ctx, RaucChecksum *checksum, GError **error) {
    if (!ctx || !checksum) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_INVALID_FORMAT,
                   "Invalid context or checksum");
        return FALSE;
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    if (!EVP_DigestFinal_ex(ctx->md_ctx, digest, &digest_len)) {
        g_set_error(error, R_CHECKSUM_ERROR, R_CHECKSUM_ERROR_CALCULATION_FAILED,
                   "Failed to finalize digest");
        return FALSE;
    }

    // Convert to hex string
    GString *hex_string = g_string_sized_new(digest_len * 2);
    for (unsigned int i = 0; i < digest_len; i++) {
        g_string_append_printf(hex_string, "%02x", digest[i]);
    }

    checksum->type = ctx->type;
    g_free(checksum->digest);
    checksum->digest = g_string_free(hex_string, FALSE);
    checksum->size = ctx->bytes_processed;

    return TRUE;
}

void r_checksum_context_free(RaucChecksumContext *ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->md_ctx) {
        EVP_MD_CTX_free(ctx->md_ctx);
    }

    g_free(ctx);
}
