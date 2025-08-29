#include "rauc/utils.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib/gstdio.h>

#ifdef WITH_DLT
#include <dlt/dlt.h>
DLT_DECLARE_CONTEXT(dlt_context_utils);
static gboolean dlt_initialized = FALSE;
#endif

static void ensure_dlt_init(void) {
#ifdef WITH_DLT
    if (!dlt_initialized) {
        DLT_REGISTER_CONTEXT(dlt_context_utils, "RUTIL", "RAUC Utils");
        dlt_initialized = TRUE;
    }
#endif
}

gboolean r_file_exists(const gchar *filename) {
    if (!filename) {
        return FALSE;
    }

    return g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);
}

gboolean r_directory_exists(const gchar *dirname) {
    if (!dirname) {
        return FALSE;
    }

    return g_file_test(dirname, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR);
}

gchar *read_file_str(const gchar *filename, GError **error) {
    ensure_dlt_init();

    if (!filename) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                   "Invalid filename");
        return NULL;
    }

    gchar *contents = NULL;
    gsize length = 0;

    if (!g_file_get_contents(filename, &contents, &length, error)) {
        r_debug("Failed to read file: %s", filename);
        return NULL;
    }

    r_debug("Read %zu bytes from file: %s", length, filename);
    return contents;
}

gboolean write_file_str(const gchar *filename, const gchar *content, GError **error) {
    ensure_dlt_init();

    if (!filename || !content) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                   "Invalid filename or content");
        return FALSE;
    }

    gboolean result = g_file_set_contents(filename, content, -1, error);

    if (result) {
        r_debug("Wrote %zu bytes to file: %s", strlen(content), filename);
    } else {
        r_error("Failed to write file: %s", filename);
    }

    return result;
}

gboolean r_copy_file(const gchar *src_path, const gchar *dest_path, GError **error) {
    ensure_dlt_init();

    if (!src_path || !dest_path) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                   "Invalid source or destination path");
        return FALSE;
    }

    GFile *src = g_file_new_for_path(src_path);
    GFile *dest = g_file_new_for_path(dest_path);

    gboolean result = g_file_copy(src, dest, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, error);

    if (result) {
        r_debug("Copied file from %s to %s", src_path, dest_path);
    } else {
        r_error("Failed to copy file from %s to %s", src_path, dest_path);
    }

    g_object_unref(src);
    g_object_unref(dest);

    return result;
}

gboolean r_mkdir_parents(const gchar *dirname, GError **error) {
    ensure_dlt_init();

    if (!dirname) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                   "Invalid directory name");
        return FALSE;
    }

    if (g_mkdir_with_parents(dirname, 0755) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to create directory '%s': %s", dirname, g_strerror(errno));
        r_error("Failed to create directory: %s", dirname);
        return FALSE;
    }

    r_debug("Created directory: %s", dirname);
    return TRUE;
}

gboolean r_chmod(const gchar *filename, mode_t mode, GError **error) {
    ensure_dlt_init();

    if (!filename) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                   "Invalid filename");
        return FALSE;
    }

    if (g_chmod(filename, mode) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to change permissions of '%s': %s", filename, g_strerror(errno));
        r_error("Failed to chmod %s", filename);
        return FALSE;
    }

    r_debug("Changed permissions of %s to %04o", filename, mode);
    return TRUE;
}

gchar *r_create_temp_dir(const gchar *template_name, GError **error) {
    ensure_dlt_init();

    if (!template_name) {
        template_name = "rauc-XXXXXX";
    }

    gchar *temp_dir = g_dir_make_tmp(template_name, error);

    if (temp_dir) {
        r_debug("Created temporary directory: %s", temp_dir);
    } else {
        r_error("Failed to create temporary directory");
    }

    return temp_dir;
}

gboolean r_remove_tree(const gchar *dirname, GError **error) {
    ensure_dlt_init();

    if (!dirname) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                   "Invalid directory name");
        return FALSE;
    }

    if (!r_directory_exists(dirname)) {
        return TRUE;  // 이미 없으면 성공으로 간주
    }

    GDir *dir = g_dir_open(dirname, 0, error);
    if (!dir) {
        r_error("Failed to open directory: %s", dirname);
        return FALSE;
    }

    const gchar *entry;
    gboolean success = TRUE;

    while ((entry = g_dir_read_name(dir)) != NULL) {
        gchar *entry_path = g_build_filename(dirname, entry, NULL);

        if (g_file_test(entry_path, G_FILE_TEST_IS_DIR)) {
            if (!r_remove_tree(entry_path, error)) {
                success = FALSE;
                g_free(entry_path);
                break;
            }
        } else {
            if (g_unlink(entry_path) != 0) {
                g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                           "Failed to remove file '%s': %s", entry_path, g_strerror(errno));
                success = FALSE;
                g_free(entry_path);
                break;
            }
        }

        g_free(entry_path);
    }

    g_dir_close(dir);

    if (success) {
        if (g_rmdir(dirname) != 0) {
            g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                       "Failed to remove directory '%s': %s", dirname, g_strerror(errno));
            success = FALSE;
        } else {
            r_debug("Removed directory tree: %s", dirname);
        }
    }

    return success;
}

gboolean r_symlink(const gchar *target, const gchar *linkpath, GError **error) {
    ensure_dlt_init();

    if (!target || !linkpath) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                   "Invalid target or link path");
        return FALSE;
    }

    if (symlink(target, linkpath) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to create symlink '%s' -> '%s': %s",
                   linkpath, target, g_strerror(errno));
        r_error("Failed to create symlink %s -> %s", linkpath, target);
        return FALSE;
    }

    r_debug("Created symlink: %s -> %s", linkpath, target);
    return TRUE;
}

gboolean r_is_block_device(const gchar *path) {
    if (!path) {
        return FALSE;
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        return FALSE;
    }

    return S_ISBLK(st.st_mode);
}

gboolean r_fsync(const gchar *path, GError **error) {
    ensure_dlt_init();

    if (!path) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_INVAL,
                   "Invalid path");
        return FALSE;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to open '%s': %s", path, g_strerror(errno));
        return FALSE;
    }

    if (fsync(fd) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to sync '%s': %s", path, g_strerror(errno));
        close(fd);
        return FALSE;
    }

    close(fd);
    r_debug("Synced file: %s", path);
    return TRUE;
}

gchar *r_format_size(guint64 bytes) {
    return g_format_size(bytes);
}

gboolean r_str_has_prefix(const gchar *str, const gchar *prefix) {
    if (!str || !prefix) {
        return FALSE;
    }

    return g_str_has_prefix(str, prefix);
}

gboolean r_str_has_suffix(const gchar *str, const gchar *suffix) {
    if (!str || !suffix) {
        return FALSE;
    }

    return g_str_has_suffix(str, suffix);
}

gchar *r_path_get_basename(const gchar *path) {
    if (!path) {
        return NULL;
    }

    return g_path_get_basename(path);
}

gchar *r_path_get_dirname(const gchar *path) {
    if (!path) {
        return NULL;
    }

    return g_path_get_dirname(path);
}

gchar *r_build_path(const gchar *first_element, ...) {
    if (!first_element) {
        return NULL;
    }

    va_list args;
    va_start(args, first_element);

    GPtrArray *elements = g_ptr_array_new();
    g_ptr_array_add(elements, (gpointer)first_element);

    const gchar *element;
    while ((element = va_arg(args, const gchar *)) != NULL) {
        g_ptr_array_add(elements, (gpointer)element);
    }

    va_end(args);

    g_ptr_array_add(elements, NULL);

    gchar *result = g_build_filenamev((gchar **)elements->pdata);

    g_ptr_array_free(elements, TRUE);

    return result;
}

gchar *r_realpath(const gchar *path) {
    if (!path) {
        return NULL;
    }

    return g_canonicalize_filename(path, NULL);
}

gchar *r_get_current_time_iso8601(void) {
    GDateTime *now = g_date_time_new_now_utc();
    gchar *iso_string = g_date_time_format_iso8601(now);
    g_date_time_unref(now);

    return iso_string;
}

gboolean r_subprocess_run(gchar * const *argv,
                         gchar **stdout_output,
                         gchar **stderr_output,
                         gint *exit_status,
                         GError **error) {
    ensure_dlt_init();

    if (!argv || !argv[0]) {
        g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_INVAL,
                   "Invalid command");
        return FALSE;
    }

    gchar *stdout_str = NULL;
    gchar *stderr_str = NULL;
    gint status = 0;

    gboolean success = g_spawn_sync(NULL,  // working_directory
                                   argv,
                                   NULL,   // envp
                                   G_SPAWN_SEARCH_PATH,
                                   NULL,   // child_setup
                                   NULL,   // user_data
                                   stdout_output ? &stdout_str : NULL,
                                   stderr_output ? &stderr_str : NULL,
                                   &status,
                                   error);

    if (success) {
        if (stdout_output) {
            *stdout_output = stdout_str;
        } else {
            g_free(stdout_str);
        }

        if (stderr_output) {
            *stderr_output = stderr_str;
        } else {
            g_free(stderr_str);
        }

        if (exit_status) {
            *exit_status = status;
        }

        r_debug("Command executed: %s (exit status: %d)", argv[0], status);
    } else {
        g_free(stdout_str);
        g_free(stderr_str);
        r_error("Failed to execute command: %s", argv[0]);
    }

    return success;
}
