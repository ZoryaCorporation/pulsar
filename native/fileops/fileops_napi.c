/**
 * @file fileops_napi.c
 * @brief Node.js NAPI bindings for Zorya FileOps
 *
 * @author Anthony Taliento
 * @date 2025-12-22
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 */

#define NAPI_VERSION 8

#include <node_api.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "zorya_fileops.h"

/* ============================================================
 * Utility Macros
 * ============================================================ */

#define FILEOPS_NAPI_VERSION "1.0.0"

#define NAPI_CALL(call)                                        \
    do {                                                       \
        napi_status status = (call);                           \
        if (status != napi_ok) {                               \
            napi_throw_error(env, NULL, "NAPI call failed");   \
            return NULL;                                       \
        }                                                      \
    } while (0)

/* ============================================================
 * Helper Functions
 * ============================================================ */

static napi_value create_stat_object(napi_env env, const zfo_stat_t* st) {
    napi_value obj;
    napi_create_object(env, &obj);

    napi_value val;

    napi_create_int64(env, (int64_t)st->size, &val);
    napi_set_named_property(env, obj, "size", val);

    napi_create_int64(env, (int64_t)st->atime, &val);
    napi_set_named_property(env, obj, "atime", val);

    napi_create_int64(env, (int64_t)st->mtime, &val);
    napi_set_named_property(env, obj, "mtime", val);

    napi_create_int64(env, (int64_t)st->ctime, &val);
    napi_set_named_property(env, obj, "ctime", val);

    napi_create_uint32(env, st->mode, &val);
    napi_set_named_property(env, obj, "mode", val);

    napi_create_uint32(env, st->uid, &val);
    napi_set_named_property(env, obj, "uid", val);

    napi_create_uint32(env, st->gid, &val);
    napi_set_named_property(env, obj, "gid", val);

    napi_create_int64(env, (int64_t)st->inode, &val);
    napi_set_named_property(env, obj, "ino", val);

    napi_create_uint32(env, st->nlink, &val);
    napi_set_named_property(env, obj, "nlink", val);

    napi_create_int32(env, st->type, &val);
    napi_set_named_property(env, obj, "type", val);

    /* Boolean helpers */
    napi_get_boolean(env, st->type == ZFO_TYPE_FILE, &val);
    napi_set_named_property(env, obj, "isFile", val);

    napi_get_boolean(env, st->type == ZFO_TYPE_DIR, &val);
    napi_set_named_property(env, obj, "isDirectory", val);

    napi_get_boolean(env, st->type == ZFO_TYPE_SYMLINK, &val);
    napi_set_named_property(env, obj, "isSymlink", val);

    return obj;
}

static const char* get_error_string(void) {
    return strerror(errno);
}

/* ============================================================
 * Version
 * ============================================================ */

static napi_value get_version(napi_env env, napi_callback_info info) {
    (void)info;
    napi_value result;
    napi_create_string_utf8(env, FILEOPS_NAPI_VERSION, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* ============================================================
 * File Operations
 * ============================================================ */

/* readFile(path: string): Buffer */
static napi_value read_file(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    uint8_t* data = NULL;
    size_t size = 0;
    int result = zfo_read_file(path, &data, &size);
    if (result != ZFO_OK || !data) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value buffer;
    void* buffer_data;
    NAPI_CALL(napi_create_buffer_copy(env, size, data, &buffer_data, &buffer));

    free(data);
    return buffer;
}

/* writeFile(path: string, data: Buffer): void */
static napi_value write_file(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Path and data required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    void* data;
    size_t data_len;
    NAPI_CALL(napi_get_buffer_info(env, argv[1], &data, &data_len));

    int result = zfo_write_file(path, data, data_len);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* appendFile(path: string, data: Buffer): void */
static napi_value append_file(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Path and data required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    void* data;
    size_t data_len;
    NAPI_CALL(napi_get_buffer_info(env, argv[1], &data, &data_len));

    int result = zfo_append_file(path, data, data_len);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* copyFile(src: string, dst: string): void */
static napi_value copy_file(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Source and destination required");
        return NULL;
    }

    char src[4096], dst[4096];
    size_t len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], src, sizeof(src), &len));
    NAPI_CALL(napi_get_value_string_utf8(env, argv[1], dst, sizeof(dst), &len));

    int result = zfo_copy(src, dst, NULL);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* moveFile(src: string, dst: string): void */
static napi_value move_file(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Source and destination required");
        return NULL;
    }

    char src[4096], dst[4096];
    size_t len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], src, sizeof(src), &len));
    NAPI_CALL(napi_get_value_string_utf8(env, argv[1], dst, sizeof(dst), &len));

    int result = zfo_move(src, dst, NULL);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* remove(path: string): void */
static napi_value remove_path(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    int result = zfo_remove(path);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* removeRecursive(path: string): void */
static napi_value remove_recursive(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    int result = zfo_remove_all(path);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* ============================================================
 * Stat Operations
 * ============================================================ */

/* stat(path: string): StatResult */
static napi_value stat_path(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    zfo_stat_t st;
    int result = zfo_stat(path, &st);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    return create_stat_object(env, &st);
}

/* lstat(path: string): StatResult */
static napi_value lstat_path(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    zfo_stat_t st;
    int result = zfo_lstat(path, &st);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    return create_stat_object(env, &st);
}

/* exists(path: string): boolean */
static napi_value exists_path(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    napi_value result;
    napi_get_boolean(env, zfo_exists(path), &result);
    return result;
}

/* isFile(path: string): boolean */
static napi_value is_file(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    napi_value result;
    napi_get_boolean(env, zfo_is_file(path), &result);
    return result;
}

/* isDirectory(path: string): boolean */
static napi_value is_directory(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    napi_value result;
    napi_get_boolean(env, zfo_is_dir(path), &result);
    return result;
}

/* isSymlink(path: string): boolean */
static napi_value is_symlink(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    napi_value result;
    napi_get_boolean(env, zfo_is_symlink(path), &result);
    return result;
}

/* fileSize(path: string): number */
static napi_value file_size(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    zfo_off_t size = zfo_size(path);

    napi_value result;
    napi_create_int64(env, (int64_t)size, &result);
    return result;
}

/* ============================================================
 * Directory Operations
 * ============================================================ */

/* mkdir(path: string, recursive?: boolean): void */
static napi_value mkdir_path(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    bool recursive = false;
    if (argc >= 2) {
        napi_get_value_bool(env, argv[1], &recursive);
    }

    int result;
    if (recursive) {
        result = zfo_mkdir_p(path, 0755);
    } else {
        result = zfo_mkdir(path, 0755);
    }

    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* readdir(path: string): string[] */
static napi_value readdir_path(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    zfo_dirent_t* entries = NULL;
    size_t count = 0;
    int result = zfo_listdir(path, &entries, &count);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value arr;
    napi_create_array_with_length(env, count, &arr);

    for (size_t i = 0; i < count; i++) {
        napi_value name;
        napi_create_string_utf8(env, entries[i].name, NAPI_AUTO_LENGTH, &name);
        napi_set_element(env, arr, (uint32_t)i, name);
    }

    free(entries);
    return arr;
}

/* readdirWithTypes(path: string): DirEntry[] */
static napi_value readdir_with_types(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    zfo_dirent_t* entries = NULL;
    size_t count = 0;
    int result = zfo_listdir(path, &entries, &count);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value arr;
    napi_create_array_with_length(env, count, &arr);

    for (size_t i = 0; i < count; i++) {
        napi_value obj;
        napi_create_object(env, &obj);

        napi_value name, type;
        napi_create_string_utf8(env, entries[i].name, NAPI_AUTO_LENGTH, &name);
        napi_create_int32(env, entries[i].type, &type);

        napi_set_named_property(env, obj, "name", name);
        napi_set_named_property(env, obj, "type", type);

        napi_value isFile, isDir, isSymlink;
        napi_get_boolean(env, entries[i].type == ZFO_TYPE_FILE, &isFile);
        napi_get_boolean(env, entries[i].type == ZFO_TYPE_DIR, &isDir);
        napi_get_boolean(env, entries[i].type == ZFO_TYPE_SYMLINK, &isSymlink);

        napi_set_named_property(env, obj, "isFile", isFile);
        napi_set_named_property(env, obj, "isDirectory", isDir);
        napi_set_named_property(env, obj, "isSymlink", isSymlink);

        napi_set_element(env, arr, (uint32_t)i, obj);
    }

    free(entries);
    return arr;
}

/* ============================================================
 * Path Operations
 * ============================================================ */

/* basename(path: string): string */
static napi_value path_basename(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    char result_buf[4096];
    zfo_basename(path, result_buf, sizeof(result_buf));

    napi_value result;
    napi_create_string_utf8(env, result_buf, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* dirname(path: string): string */
static napi_value path_dirname(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    char result_buf[4096];
    zfo_dirname(path, result_buf, sizeof(result_buf));

    napi_value result;
    napi_create_string_utf8(env, result_buf, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* extname(path: string): string */
static napi_value path_extname(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    char result_buf[256];
    zfo_extname(path, result_buf, sizeof(result_buf));

    napi_value result;
    napi_create_string_utf8(env, result_buf, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* join(base: string, path: string): string */
static napi_value path_join(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "At least two paths required");
        return NULL;
    }

    char base[4096], path[4096];
    size_t len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], base, sizeof(base), &len));
    NAPI_CALL(napi_get_value_string_utf8(env, argv[1], path, sizeof(path), &len));

    char result_buf[4096];
    zfo_join(result_buf, sizeof(result_buf), base, path);

    napi_value result;
    napi_create_string_utf8(env, result_buf, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* normalize(path: string): string */
static napi_value path_normalize(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    char result_buf[4096];
    zfo_normalize(path, result_buf, sizeof(result_buf));

    napi_value result;
    napi_create_string_utf8(env, result_buf, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* resolve(path: string): string */
static napi_value path_resolve(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    char result_buf[4096];
    int rc = zfo_realpath(path, result_buf, sizeof(result_buf));
    if (rc != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value result;
    napi_create_string_utf8(env, result_buf, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* isAbsolute(path: string): boolean */
static napi_value path_is_absolute(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    /* Simple check: absolute path starts with / on Unix */
    napi_value result;
    napi_get_boolean(env, path[0] == '/', &result);
    return result;
}

/* ============================================================
 * Symlink Operations
 * ============================================================ */

/* symlink(target: string, linkpath: string): void */
static napi_value create_symlink(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Target and linkpath required");
        return NULL;
    }

    char target[4096], linkpath[4096];
    size_t len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], target, sizeof(target), &len));
    NAPI_CALL(napi_get_value_string_utf8(env, argv[1], linkpath, sizeof(linkpath), &len));

    int result = zfo_symlink(target, linkpath);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* readlink(path: string): string */
static napi_value read_symlink(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    char result_buf[4096];
    int rc = zfo_readlink(path, result_buf, sizeof(result_buf));
    if (rc != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value result;
    napi_create_string_utf8(env, result_buf, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* ============================================================
 * System Paths
 * ============================================================ */

/* tmpdir(): string */
static napi_value get_tmpdir(napi_env env, napi_callback_info info) {
    (void)info;

    char result_buf[4096];
    zfo_tmpdir(result_buf, sizeof(result_buf));

    napi_value result;
    napi_create_string_utf8(env, result_buf, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* homedir(): string */
static napi_value get_homedir(napi_env env, napi_callback_info info) {
    (void)info;

    char result_buf[4096];
    const char* home = getenv("HOME");
    if (home) {
        size_t len = strlen(home);
        if (len >= sizeof(result_buf)) len = sizeof(result_buf) - 1;
        memcpy(result_buf, home, len);
        result_buf[len] = '\0';
    } else {
        result_buf[0] = '\0';
    }

    napi_value result;
    napi_create_string_utf8(env, result_buf, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* cwd(): string */
static napi_value get_cwd(napi_env env, napi_callback_info info) {
    (void)info;

    char result_buf[4096];
    zfo_getcwd(result_buf, sizeof(result_buf));

    napi_value result;
    napi_create_string_utf8(env, result_buf, NAPI_AUTO_LENGTH, &result);
    return result;
}

/* chdir(path: string): void */
static napi_value change_dir(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    int result = zfo_chdir(path);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* ============================================================
 * Permissions
 * ============================================================ */

/* chmod(path: string, mode: number): void */
static napi_value chmod_path(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Path and mode required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    uint32_t mode;
    NAPI_CALL(napi_get_value_uint32(env, argv[1], &mode));

    int result = zfo_chmod(path, mode);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* chown(path: string, uid: number, gid: number): void */
static napi_value chown_path(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 3) {
        napi_throw_type_error(env, NULL, "Path, uid, and gid required");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    uint32_t uid, gid;
    NAPI_CALL(napi_get_value_uint32(env, argv[1], &uid));
    NAPI_CALL(napi_get_value_uint32(env, argv[2], &gid));

    int result = zfo_chown(path, uid, gid);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* ============================================================
 * Glob
 * ============================================================ */

/* glob(pattern: string): string[] */
static napi_value glob_paths(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Pattern required");
        return NULL;
    }

    char pattern[4096];
    size_t pattern_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], pattern, sizeof(pattern), &pattern_len));

    char** paths = NULL;
    size_t count = 0;
    int result = zfo_glob(pattern, &paths, &count);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value arr;
    napi_create_array_with_length(env, count, &arr);

    for (size_t i = 0; i < count; i++) {
        napi_value path;
        napi_create_string_utf8(env, paths[i], NAPI_AUTO_LENGTH, &path);
        napi_set_element(env, arr, (uint32_t)i, path);
        free(paths[i]);
    }
    if (paths) free(paths);

    return arr;
}

/* ============================================================
 * Watch (inotify wrapper)
 * ============================================================ */

/* Internal watch state */
static zfo_watch_t* g_watcher = NULL;

/* Callback data storage for polling */
typedef struct {
    napi_env env;
    napi_value arr;
    uint32_t count;
} watch_poll_data_t;

static void watch_poll_callback(const zfo_watch_data_t* data, void* userdata) {
    watch_poll_data_t* poll_data = (watch_poll_data_t*)userdata;
    napi_env env = poll_data->env;

    napi_value obj;
    napi_create_object(env, &obj);

    napi_value event_val, path_val, old_path_val, is_dir_val, cookie_val;
    napi_create_int32(env, (int32_t)data->event, &event_val);
    napi_create_string_utf8(env, data->path, NAPI_AUTO_LENGTH, &path_val);
    napi_create_string_utf8(env, data->old_path, NAPI_AUTO_LENGTH, &old_path_val);
    napi_get_boolean(env, data->is_dir, &is_dir_val);
    napi_create_uint32(env, data->cookie, &cookie_val);

    napi_set_named_property(env, obj, "event", event_val);
    napi_set_named_property(env, obj, "path", path_val);
    napi_set_named_property(env, obj, "oldPath", old_path_val);
    napi_set_named_property(env, obj, "isDir", is_dir_val);
    napi_set_named_property(env, obj, "cookie", cookie_val);

    /* Event type helpers */
    napi_value isCreate, isDelete, isModify, isMove;
    napi_get_boolean(env, data->event == ZFO_EVENT_CREATE, &isCreate);
    napi_get_boolean(env, data->event == ZFO_EVENT_DELETE, &isDelete);
    napi_get_boolean(env, data->event == ZFO_EVENT_MODIFY, &isModify);
    napi_get_boolean(env, data->event == ZFO_EVENT_MOVE_FROM || data->event == ZFO_EVENT_MOVE_TO, &isMove);

    napi_set_named_property(env, obj, "isCreate", isCreate);
    napi_set_named_property(env, obj, "isDelete", isDelete);
    napi_set_named_property(env, obj, "isModify", isModify);
    napi_set_named_property(env, obj, "isMove", isMove);

    napi_set_element(env, poll_data->arr, poll_data->count++, obj);
}

/* watchInit(): void */
static napi_value watch_init(napi_env env, napi_callback_info info) {
    (void)info;

    if (g_watcher) {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        return undefined;
    }

    g_watcher = zfo_watch_create();
    if (!g_watcher) {
        napi_throw_error(env, NULL, "Failed to initialize watcher");
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* watchAdd(path: string): number */
static napi_value watch_add(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Path required");
        return NULL;
    }

    if (!g_watcher) {
        napi_throw_error(env, NULL, "Watcher not initialized");
        return NULL;
    }

    char path[4096];
    size_t path_len;
    NAPI_CALL(napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &path_len));

    int wd = zfo_watch_add(g_watcher, path, ZFO_EVENT_ALL, false);
    if (wd < 0) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value result;
    napi_create_int32(env, wd, &result);
    return result;
}

/* watchRemove(wd: number): void */
static napi_value watch_remove(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Watch descriptor required");
        return NULL;
    }

    if (!g_watcher) {
        napi_throw_error(env, NULL, "Watcher not initialized");
        return NULL;
    }

    int32_t wd;
    NAPI_CALL(napi_get_value_int32(env, argv[0], &wd));

    int result = zfo_watch_remove(g_watcher, wd);
    if (result != ZFO_OK) {
        napi_throw_error(env, NULL, get_error_string());
        return NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* watchPoll(timeout: number): WatchEvent[] */
static napi_value watch_poll(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (!g_watcher) {
        napi_throw_error(env, NULL, "Watcher not initialized");
        return NULL;
    }

    int32_t timeout = 0;
    if (argc >= 1) {
        NAPI_CALL(napi_get_value_int32(env, argv[0], &timeout));
    }

    napi_value arr;
    napi_create_array(env, &arr);

    watch_poll_data_t poll_data = { env, arr, 0 };

    if (timeout > 0) {
        zfo_watch_wait(g_watcher, watch_poll_callback, &poll_data, timeout);
    } else {
        zfo_watch_poll(g_watcher, watch_poll_callback, &poll_data);
    }

    return arr;
}

/* watchClose(): void */
static napi_value watch_close(napi_env env, napi_callback_info info) {
    (void)info;

    if (g_watcher) {
        zfo_watch_destroy(g_watcher);
        g_watcher = NULL;
    }

    napi_value undefined;
    napi_get_undefined(env, &undefined);
    return undefined;
}

/* ============================================================
 * Module Registration
 * ============================================================ */

#define EXPORT_FUNCTION(name, func)                            \
    do {                                                       \
        napi_value fn;                                         \
        napi_create_function(env, name, NAPI_AUTO_LENGTH,      \
                             func, NULL, &fn);                 \
        napi_set_named_property(env, exports, name, fn);       \
    } while (0)

static napi_value Init(napi_env env, napi_value exports) {
    /* Version */
    EXPORT_FUNCTION("version", get_version);

    /* File Operations */
    EXPORT_FUNCTION("readFile", read_file);
    EXPORT_FUNCTION("writeFile", write_file);
    EXPORT_FUNCTION("appendFile", append_file);
    EXPORT_FUNCTION("copyFile", copy_file);
    EXPORT_FUNCTION("moveFile", move_file);
    EXPORT_FUNCTION("remove", remove_path);
    EXPORT_FUNCTION("removeRecursive", remove_recursive);

    /* Stat Operations */
    EXPORT_FUNCTION("stat", stat_path);
    EXPORT_FUNCTION("lstat", lstat_path);
    EXPORT_FUNCTION("exists", exists_path);
    EXPORT_FUNCTION("isFile", is_file);
    EXPORT_FUNCTION("isDirectory", is_directory);
    EXPORT_FUNCTION("isSymlink", is_symlink);
    EXPORT_FUNCTION("fileSize", file_size);

    /* Directory Operations */
    EXPORT_FUNCTION("mkdir", mkdir_path);
    EXPORT_FUNCTION("readdir", readdir_path);
    EXPORT_FUNCTION("readdirWithTypes", readdir_with_types);

    /* Path Operations */
    EXPORT_FUNCTION("basename", path_basename);
    EXPORT_FUNCTION("dirname", path_dirname);
    EXPORT_FUNCTION("extname", path_extname);
    EXPORT_FUNCTION("join", path_join);
    EXPORT_FUNCTION("normalize", path_normalize);
    EXPORT_FUNCTION("resolve", path_resolve);
    EXPORT_FUNCTION("isAbsolute", path_is_absolute);

    /* Symlinks */
    EXPORT_FUNCTION("symlink", create_symlink);
    EXPORT_FUNCTION("readlink", read_symlink);

    /* System Paths */
    EXPORT_FUNCTION("tmpdir", get_tmpdir);
    EXPORT_FUNCTION("homedir", get_homedir);
    EXPORT_FUNCTION("cwd", get_cwd);
    EXPORT_FUNCTION("chdir", change_dir);

    /* Permissions */
    EXPORT_FUNCTION("chmod", chmod_path);
    EXPORT_FUNCTION("chown", chown_path);

    /* Glob */
    EXPORT_FUNCTION("glob", glob_paths);

    /* Watch */
    EXPORT_FUNCTION("watchInit", watch_init);
    EXPORT_FUNCTION("watchAdd", watch_add);
    EXPORT_FUNCTION("watchRemove", watch_remove);
    EXPORT_FUNCTION("watchPoll", watch_poll);
    EXPORT_FUNCTION("watchClose", watch_close);

    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
