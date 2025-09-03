#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>

#include "../../include/legacy/manifest.h"
#include "../../include/legacy/checksum.h"
#include "../../include/legacy/utils.h"
#include "../../include/legacy/context.h"

RaucManifest* r_manifest_new(void)
{
    RaucManifest *manifest = g_new0(RaucManifest, 1);
    manifest->images = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)r_manifest_free_image);
    return manifest;
}

void r_manifest_free(RaucManifest *manifest)
{
    if (manifest == NULL)
        return;

    g_free(manifest->compatible);
    g_free(manifest->version);
    g_free(manifest->description);
    g_free(manifest->build);

    if (manifest->images) {
        g_hash_table_unref(manifest->images);
    }

    g_free(manifest);
}

RaucImage* r_manifest_image_new(const gchar *slotclass, const gchar *filename)
{
    RaucImage *image = g_new0(RaucImage, 1);
    image->slotclass = g_strdup(slotclass);
    image->filename = g_strdup(filename);
    return image;
}

void r_manifest_free_image(RaucImage *image)
{
    if (image == NULL)
        return;

    g_free(image->filename);
    g_free(image->slotclass);
    g_free(image->variant);
    g_free(image->checksum.digest);
    g_free(image->hooks);

    g_free(image);
}

static gboolean parse_manifest_update_section(GKeyFile *key_file, RaucManifest *manifest, GError **error)
{
    GError *ierror = NULL;
    gchar *compatible = NULL;
    gchar *version = NULL;
    gchar *description = NULL;
    gchar *build = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(key_file != NULL, FALSE);
    g_return_val_if_fail(manifest != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!g_key_file_has_group(key_file, "update")) {
        g_set_error(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND,
                   "Missing required [update] section in manifest");
        goto out;
    }

    compatible = g_key_file_get_string(key_file, "update", "compatible", &ierror);
    if (compatible == NULL) {
        g_propagate_prefixed_error(error, ierror, "Missing compatible string in [update] section: ");
        goto out;
    }

    version = g_key_file_get_string(key_file, "update", "version", NULL);
    description = g_key_file_get_string(key_file, "update", "description", NULL);
    build = g_key_file_get_string(key_file, "update", "build", NULL);

    manifest->compatible = g_steal_pointer(&compatible);
    manifest->version = g_steal_pointer(&version);
    manifest->description = g_steal_pointer(&description);
    manifest->build = g_steal_pointer(&build);

    res = TRUE;

out:
    g_free(compatible);
    g_free(version);
    g_free(description);
    g_free(build);
    return res;
}

static RaucChecksumType parse_checksum_type(const gchar *type_str)
{
    if (g_strcmp0(type_str, "md5") == 0)
        return R_CHECKSUM_MD5;
    else if (g_strcmp0(type_str, "sha1") == 0)
        return R_CHECKSUM_SHA1;
    else if (g_strcmp0(type_str, "sha256") == 0)
        return R_CHECKSUM_SHA256;
    else if (g_strcmp0(type_str, "sha512") == 0)
        return R_CHECKSUM_SHA512;
    else
        return R_CHECKSUM_SHA256;
}

static gboolean parse_image_section(GKeyFile *key_file, const gchar *group_name,
                                  RaucManifest *manifest, GError **error)
{
    GError *ierror = NULL;
    RaucImage *image = NULL;
    gchar *filename = NULL;
    gchar *slotclass = NULL;
    gchar *variant = NULL;
    gchar *checksum_str = NULL;
    gchar *checksum_type_str = NULL;
    gchar *hooks = NULL;
    gsize size = 0;
    gboolean res = FALSE;

    g_return_val_if_fail(key_file != NULL, FALSE);
    g_return_val_if_fail(group_name != NULL, FALSE);
    g_return_val_if_fail(manifest != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    filename = g_key_file_get_string(key_file, group_name, "filename", &ierror);
    if (filename == NULL) {
        g_propagate_prefixed_error(error, ierror, "Missing filename in image section '%s': ", group_name);
        goto out;
    }

    slotclass = g_key_file_get_string(key_file, group_name, "slotclass", NULL);
    if (slotclass == NULL) {
        g_autofree gchar *basename = g_path_get_basename(group_name);
        if (g_str_has_prefix(basename, "image.")) {
            slotclass = g_strdup(basename + 6);
        } else {
            slotclass = g_strdup("rootfs");
        }
    }

    variant = g_key_file_get_string(key_file, group_name, "variant", NULL);
    checksum_str = g_key_file_get_string(key_file, group_name, "sha256", NULL);
    checksum_type_str = g_strdup("sha256");

    if (checksum_str == NULL) {
        checksum_str = g_key_file_get_string(key_file, group_name, "sha1", NULL);
        g_free(checksum_type_str);
        checksum_type_str = g_strdup("sha1");
    }

    if (checksum_str == NULL) {
        checksum_str = g_key_file_get_string(key_file, group_name, "md5", NULL);
        g_free(checksum_type_str);
        checksum_type_str = g_strdup("md5");
    }

    hooks = g_key_file_get_string(key_file, group_name, "hooks", NULL);

    if (g_key_file_has_key(key_file, group_name, "size", NULL)) {
        size = g_key_file_get_uint64(key_file, group_name, "size", NULL);
    }

    image = g_new0(RaucImage, 1);
    image->filename = g_steal_pointer(&filename);
    image->slotclass = g_steal_pointer(&slotclass);
    image->variant = g_steal_pointer(&variant);
    image->size = size;
    image->hooks = g_steal_pointer(&hooks);

    if (checksum_str) {
        image->checksum.digest = g_steal_pointer(&checksum_str);
        image->checksum.type = parse_checksum_type(checksum_type_str);
    }

    g_hash_table_insert(manifest->images, g_strdup(image->slotclass), image);
    res = TRUE;

out:
    if (!res && image) {
        r_manifest_free_image(image);
    }
    g_free(filename);
    g_free(slotclass);
    g_free(variant);
    g_free(checksum_str);
    g_free(checksum_type_str);
    g_free(hooks);
    return res;
}

static gboolean parse_all_image_sections(GKeyFile *key_file, RaucManifest *manifest, GError **error)
{
    GError *ierror = NULL;
    gchar **groups = NULL;
    gsize groups_nb = 0;
    gboolean res = FALSE;

    g_return_val_if_fail(key_file != NULL, FALSE);
    g_return_val_if_fail(manifest != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    groups = g_key_file_get_groups(key_file, &groups_nb);

    for (gsize i = 0; i < groups_nb; i++) {
        if (g_str_has_prefix(groups[i], "image.")) {
            if (!parse_image_section(key_file, groups[i], manifest, &ierror)) {
                g_propagate_error(error, ierror);
                goto out;
            }
        }
    }

    if (g_hash_table_size(manifest->images) == 0) {
        g_set_error(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND,
                   "No image sections found in manifest");
        goto out;
    }

    res = TRUE;

out:
    g_strfreev(groups);
    return res;
}

gboolean r_manifest_read_file(const gchar *filename, RaucManifest **manifest, GError **error)
{
    GError *ierror = NULL;
    GKeyFile *key_file = NULL;
    RaucManifest *imanifest = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(filename != NULL, FALSE);
    g_return_val_if_fail(manifest != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                   "Manifest file not found: '%s'", filename);
        goto out;
    }

    key_file = g_key_file_new();
    if (!g_key_file_load_from_file(key_file, filename, G_KEY_FILE_NONE, &ierror)) {
        g_propagate_prefixed_error(error, ierror, "Failed to load manifest file '%s': ", filename);
        goto out;
    }

    imanifest = r_manifest_new();

    if (!parse_manifest_update_section(key_file, imanifest, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    if (!parse_all_image_sections(key_file, imanifest, &ierror)) {
        g_propagate_error(error, ierror);
        goto out;
    }

    *manifest = imanifest;
    imanifest = NULL;
    res = TRUE;

out:
    if (key_file) {
        g_key_file_free(key_file);
    }
    if (imanifest) {
        r_manifest_free(imanifest);
    }
    return res;
}

gboolean r_manifest_write_file(RaucManifest *manifest, const gchar *filename, GError **error)
{
    GError *ierror = NULL;
    GKeyFile *key_file = NULL;
    GHashTableIter iter;
    gpointer key, value;
    gchar *data = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail(manifest != NULL, FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    key_file = g_key_file_new();

    g_key_file_set_string(key_file, "update", "compatible", manifest->compatible);

    if (manifest->version) {
        g_key_file_set_string(key_file, "update", "version", manifest->version);
    }

    if (manifest->description) {
        g_key_file_set_string(key_file, "update", "description", manifest->description);
    }

    if (manifest->build) {
        g_key_file_set_string(key_file, "update", "build", manifest->build);
    }

    if (manifest->images) {
        g_hash_table_iter_init(&iter, manifest->images);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            RaucImage *image = (RaucImage*)value;
            gchar *section_name = g_strdup_printf("image.%s", image->slotclass);

            g_key_file_set_string(key_file, section_name, "filename", image->filename);
            g_key_file_set_string(key_file, section_name, "slotclass", image->slotclass);

            if (image->variant) {
                g_key_file_set_string(key_file, section_name, "variant", image->variant);
            }

            if (image->size > 0) {
                g_key_file_set_uint64(key_file, section_name, "size", image->size);
            }

            if (image->checksum.digest) {
                const gchar *checksum_key = NULL;
                switch (image->checksum.type) {
                    case R_CHECKSUM_MD5:
                        checksum_key = "md5";
                        break;
                    case R_CHECKSUM_SHA1:
                        checksum_key = "sha1";
                        break;
                    case R_CHECKSUM_SHA256:
                        checksum_key = "sha256";
                        break;
                    case R_CHECKSUM_SHA512:
                        checksum_key = "sha512";
                        break;
                }

                if (checksum_key) {
                    g_key_file_set_string(key_file, section_name, checksum_key, image->checksum.digest);
                }
            }

            if (image->hooks) {
                g_key_file_set_string(key_file, section_name, "hooks", image->hooks);
            }

            g_free(section_name);
        }
    }

    data = g_key_file_to_data(key_file, NULL, &ierror);
    if (data == NULL) {
        g_propagate_prefixed_error(error, ierror, "Failed to serialize manifest: ");
        goto out;
    }

    if (!g_file_set_contents(filename, data, -1, &ierror)) {
        g_propagate_prefixed_error(error, ierror, "Failed to write manifest file '%s': ", filename);
        goto out;
    }

    res = TRUE;

out:
    if (key_file) {
        g_key_file_free(key_file);
    }
    g_free(data);
    return res;
}

gboolean r_manifest_validate(RaucManifest *manifest, GError **error)
{
    GHashTableIter iter;
    gpointer key, value;
    gboolean res = FALSE;

    g_return_val_if_fail(manifest != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!manifest->compatible) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Manifest missing compatible string");
        goto out;
    }

    if (!manifest->images || g_hash_table_size(manifest->images) == 0) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Manifest contains no images");
        goto out;
    }

    g_hash_table_iter_init(&iter, manifest->images);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        RaucImage *image = (RaucImage*)value;

        if (!image->filename) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "Image missing filename");
            goto out;
        }

        if (!image->slotclass) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "Image missing slotclass");
            goto out;
        }

        if (!image->checksum.digest) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "Image '%s' missing checksum", image->filename);
            goto out;
        }
    }

    res = TRUE;

out:
    return res;
}

RaucImage* r_manifest_get_image(RaucManifest *manifest, const gchar *slotclass)
{
    g_return_val_if_fail(manifest != NULL, NULL);
    g_return_val_if_fail(slotclass != NULL, NULL);

    if (!manifest->images) {
        return NULL;
    }

    return (RaucImage*)g_hash_table_lookup(manifest->images, slotclass);
}

GList* r_manifest_get_slot_classes(RaucManifest *manifest)
{
    GList *slot_classes = NULL;
    GHashTableIter iter;
    gpointer key, value;

    g_return_val_if_fail(manifest != NULL, NULL);

    if (!manifest->images) {
        return NULL;
    }

    g_hash_table_iter_init(&iter, manifest->images);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        slot_classes = g_list_append(slot_classes, g_strdup((const gchar*)key));
    }

    return slot_classes;
}

gchar* r_manifest_get_info_string(RaucManifest *manifest)
{
    GString *info = NULL;
    GHashTableIter iter;
    gpointer key, value;
    gchar *result = NULL;

    g_return_val_if_fail(manifest != NULL, NULL);

    info = g_string_new("");

    g_string_append_printf(info, "Compatible: %s\n",
                          manifest->compatible ? manifest->compatible : "unknown");

    if (manifest->version) {
        g_string_append_printf(info, "Version: %s\n", manifest->version);
    }

    if (manifest->description) {
        g_string_append_printf(info, "Description: %s\n", manifest->description);
    }

    if (manifest->build) {
        g_string_append_printf(info, "Build: %s\n", manifest->build);
    }

    g_string_append(info, "\nImages:\n");

    if (manifest->images) {
        g_hash_table_iter_init(&iter, manifest->images);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            RaucImage *image = (RaucImage*)value;
            g_string_append_printf(info, "  %s: %s", image->slotclass, image->filename);

            if (image->checksum.digest) {
                const gchar *checksum_type = "unknown";
                switch (image->checksum.type) {
                    case R_CHECKSUM_MD5:
                        checksum_type = "md5";
                        break;
                    case R_CHECKSUM_SHA1:
                        checksum_type = "sha1";
                        break;
                    case R_CHECKSUM_SHA256:
                        checksum_type = "sha256";
                        break;
                    case R_CHECKSUM_SHA512:
                        checksum_type = "sha512";
                        break;
                }
                g_string_append_printf(info, " (%s: %s)", checksum_type, image->checksum.digest);
            }

            if (image->size > 0) {
                g_string_append_printf(info, " [%lu bytes]", image->size);
            }

            g_string_append(info, "\n");
        }
    } else {
        g_string_append(info, "  (none)\n");
    }

    result = g_string_free(info, FALSE);
    return result;
}
