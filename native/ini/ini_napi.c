/**
 * @file zorya_ini_napi.c
 * @brief Node.js N-API bindings for ZORYA-INI parser
 *
 * @author Anthony Taliento
 * @date 2025-12-08
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * DESCRIPTION:
 *   Native Node.js addon exposing zorya_ini.c functionality.
 *   Provides O(1) config lookups for NEXUS IDE.
 *
 * USAGE (JavaScript):
 *   const ini = require('./zorya_ini.node');
 *   
 *   const config = ini.load('config.ini');
 *   const port = ini.getInt(config, 'server.port');
 *   const host = ini.get(config, 'server.host');
 *   const features = ini.getArray(config, 'app.features');
 *   ini.free(config);
 *
 * BUILD:
 *   node-gyp configure build
 */

#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zorya_ini.h"

/* ============================================================
 * HELPER MACROS
 * ============================================================ */

#define NAPI_CALL(env, call)                                          \
    do {                                                              \
        napi_status status = (call);                                  \
        if (status != napi_ok) {                                      \
            const napi_extended_error_info* error_info = NULL;        \
            napi_get_last_error_info((env), &error_info);             \
            const char* err_message = error_info->error_message;      \
            bool is_pending;                                          \
            napi_is_exception_pending((env), &is_pending);            \
            if (!is_pending) {                                        \
                const char* message = (err_message == NULL)           \
                    ? "Empty error message"                           \
                    : err_message;                                    \
                napi_throw_error((env), NULL, message);               \
            }                                                         \
            return NULL;                                              \
        }                                                             \
    } while(0)

#define DECLARE_NAPI_METHOD(name, func)                               \
    { name, 0, func, 0, 0, 0, napi_default, 0 }

/* ============================================================
 * ZORYA_INI_NEW - Create new INI context
 * ============================================================ */

/**
 * @brief Create a new INI context
 * 
 * JavaScript: const ctx = ini.create();
 * 
 * @return External pointer to ZoryaIni context
 */
static napi_value ZoryaIniNew(napi_env env, napi_callback_info info) {
    (void)info;  /* Unused */
    
    ZoryaIni *ini = zorya_ini_new();
    if (ini == NULL) {
        napi_throw_error(env, NULL, "Failed to create INI context");
        return NULL;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_create_external(env, ini, NULL, NULL, &result));
    
    return result;
}

/* ============================================================
 * ZORYA_INI_FREE - Free INI context
 * ============================================================ */

/**
 * @brief Free an INI context
 * 
 * JavaScript: ini.free(ctx);
 */
static napi_value ZoryaIniFree(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument: context");
        return NULL;
    }
    
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    zorya_ini_free(ini);
    
    napi_value undefined;
    NAPI_CALL(env, napi_get_undefined(env, &undefined));
    return undefined;
}

/* ============================================================
 * ZORYA_INI_LOAD - Load INI from file
 * ============================================================ */

/**
 * @brief Load INI from file path
 * 
 * JavaScript: ini.load(ctx, '/path/to/config.ini');
 */
static napi_value ZoryaIniLoad(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: context, filepath");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get filepath */
    size_t path_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &path_len));
    
    char *filepath = malloc(path_len + 1);
    if (filepath == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], filepath, path_len + 1, &path_len));
    
    /* Load file */
    zorya_ini_error_t err = zorya_ini_load(ini, filepath);
    free(filepath);
    
    if (err != ZORYA_INI_OK) {
        const ZoryaIniParseError *parse_err = zorya_ini_last_error(ini);
        char error_msg[512];
        if (parse_err != NULL && parse_err->message != NULL) {
            snprintf(error_msg, sizeof(error_msg), "INI parse error at line %d: %s",
                     parse_err->line, parse_err->message);
        } else {
            snprintf(error_msg, sizeof(error_msg), "INI load error: %s",
                     zorya_ini_strerror(err));
        }
        napi_throw_error(env, NULL, error_msg);
        return NULL;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

/* ============================================================
 * ZORYA_INI_LOAD_STRING - Load INI from string
 * ============================================================ */

/**
 * @brief Load INI from string buffer
 * 
 * JavaScript: ini.loadString(ctx, '[section]\nkey = value');
 */
static napi_value ZoryaIniLoadString(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: context, content");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get content string */
    size_t content_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &content_len));
    
    char *content = malloc(content_len + 1);
    if (content == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], content, content_len + 1, &content_len));
    
    /* Load from buffer */
    zorya_ini_error_t err = zorya_ini_load_buffer(ini, content, content_len, "<string>");
    free(content);
    
    if (err != ZORYA_INI_OK) {
        const ZoryaIniParseError *parse_err = zorya_ini_last_error(ini);
        char error_msg[512];
        if (parse_err != NULL && parse_err->message != NULL) {
            snprintf(error_msg, sizeof(error_msg), "INI parse error at line %d: %s",
                     parse_err->line, parse_err->message);
        } else {
            snprintf(error_msg, sizeof(error_msg), "INI parse error: %s",
                     zorya_ini_strerror(err));
        }
        napi_throw_error(env, NULL, error_msg);
        return NULL;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

/* ============================================================
 * ZORYA_INI_GET - Get string value
 * ============================================================ */

/**
 * @brief Get string value by key
 * 
 * JavaScript: const value = ini.get(ctx, 'section.key');
 */
static napi_value ZoryaIniGet(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: context, key");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get key */
    size_t key_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &key_len));
    
    char *key = malloc(key_len + 1);
    if (key == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], key, key_len + 1, &key_len));
    
    /* Get value */
    const char *value = zorya_ini_get(ini, key);
    free(key);
    
    if (value == NULL) {
        napi_value null_val;
        NAPI_CALL(env, napi_get_null(env, &null_val));
        return null_val;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &result));
    return result;
}

/* ============================================================
 * ZORYA_INI_GET_DEFAULT - Get string with default
 * ============================================================ */

/**
 * @brief Get string value with default fallback
 * 
 * JavaScript: const value = ini.getDefault(ctx, 'key', 'default');
 */
static napi_value ZoryaIniGetDefault(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 3) {
        napi_throw_type_error(env, NULL, "Expected 3 arguments: context, key, default");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get key */
    size_t key_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &key_len));
    char *key = malloc(key_len + 1);
    if (key == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], key, key_len + 1, &key_len));
    
    /* Get default */
    size_t def_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[2], NULL, 0, &def_len));
    char *def = malloc(def_len + 1);
    if (def == NULL) {
        free(key);
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[2], def, def_len + 1, &def_len));
    
    /* Get value */
    const char *value = zorya_ini_get_default(ini, key, def);
    free(key);
    
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &result));
    
    free(def);
    return result;
}

/* ============================================================
 * ZORYA_INI_GET_INT - Get integer value
 * ============================================================ */

/**
 * @brief Get integer value by key
 * 
 * JavaScript: const port = ini.getInt(ctx, 'server.port');
 */
static napi_value ZoryaIniGetInt(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: context, key");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get key */
    size_t key_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &key_len));
    char *key = malloc(key_len + 1);
    if (key == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], key, key_len + 1, &key_len));
    
    /* Get value */
    int64_t value = zorya_ini_get_int(ini, key);
    free(key);
    
    napi_value result;
    NAPI_CALL(env, napi_create_int64(env, value, &result));
    return result;
}

/* ============================================================
 * ZORYA_INI_GET_FLOAT - Get float value
 * ============================================================ */

/**
 * @brief Get float value by key
 * 
 * JavaScript: const rate = ini.getFloat(ctx, 'app.rate');
 */
static napi_value ZoryaIniGetFloat(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: context, key");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get key */
    size_t key_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &key_len));
    char *key = malloc(key_len + 1);
    if (key == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], key, key_len + 1, &key_len));
    
    /* Get value */
    double value = zorya_ini_get_float(ini, key);
    free(key);
    
    napi_value result;
    NAPI_CALL(env, napi_create_double(env, value, &result));
    return result;
}

/* ============================================================
 * ZORYA_INI_GET_BOOL - Get boolean value
 * ============================================================ */

/**
 * @brief Get boolean value by key
 * 
 * JavaScript: const enabled = ini.getBool(ctx, 'app.enabled');
 */
static napi_value ZoryaIniGetBool(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: context, key");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get key */
    size_t key_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &key_len));
    char *key = malloc(key_len + 1);
    if (key == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], key, key_len + 1, &key_len));
    
    /* Get value */
    bool value = zorya_ini_get_bool(ini, key);
    free(key);
    
    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, value, &result));
    return result;
}

/* ============================================================
 * ZORYA_INI_GET_ARRAY - Get array value
 * ============================================================ */

/**
 * @brief Get array value by key
 * 
 * JavaScript: const items = ini.getArray(ctx, 'app.features');
 */
static napi_value ZoryaIniGetArray(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: context, key");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get key */
    size_t key_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &key_len));
    char *key = malloc(key_len + 1);
    if (key == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], key, key_len + 1, &key_len));
    
    /* Get array */
    size_t count = 0;
    const char **items = zorya_ini_get_array(ini, key, &count);
    free(key);
    
    if (items == NULL) {
        napi_value empty_array;
        NAPI_CALL(env, napi_create_array_with_length(env, 0, &empty_array));
        return empty_array;
    }
    
    /* Create JavaScript array */
    napi_value result;
    NAPI_CALL(env, napi_create_array_with_length(env, count, &result));
    
    for (size_t i = 0; i < count; i++) {
        napi_value item;
        NAPI_CALL(env, napi_create_string_utf8(env, items[i], NAPI_AUTO_LENGTH, &item));
        NAPI_CALL(env, napi_set_element(env, result, (uint32_t)i, item));
    }
    
    return result;
}

/* ============================================================
 * ZORYA_INI_HAS - Check if key exists
 * ============================================================ */

/**
 * @brief Check if key exists
 * 
 * JavaScript: if (ini.has(ctx, 'section.key')) { ... }
 */
static napi_value ZoryaIniHas(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: context, key");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get key */
    size_t key_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &key_len));
    char *key = malloc(key_len + 1);
    if (key == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], key, key_len + 1, &key_len));
    
    /* Check existence */
    bool exists = zorya_ini_has(ini, key);
    free(key);
    
    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, exists, &result));
    return result;
}

/* ============================================================
 * ZORYA_INI_SET - Set string value
 * ============================================================ */

/**
 * @brief Set string value
 * 
 * JavaScript: ini.set(ctx, 'section.key', 'value');
 */
static napi_value ZoryaIniSet(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 3) {
        napi_throw_type_error(env, NULL, "Expected 3 arguments: context, key, value");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get key */
    size_t key_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], NULL, 0, &key_len));
    char *key = malloc(key_len + 1);
    if (key == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], key, key_len + 1, &key_len));
    
    /* Get value */
    size_t val_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[2], NULL, 0, &val_len));
    char *value = malloc(val_len + 1);
    if (value == NULL) {
        free(key);
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[2], value, val_len + 1, &val_len));
    
    /* Set value */
    zorya_ini_error_t err = zorya_ini_set(ini, key, value);
    free(key);
    free(value);
    
    if (err != ZORYA_INI_OK) {
        napi_throw_error(env, NULL, zorya_ini_strerror(err));
        return NULL;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

/* ============================================================
 * ZORYA_INI_SECTIONS - Get all section names
 * ============================================================ */

/**
 * @brief Get list of section names
 * 
 * JavaScript: const sections = ini.sections(ctx);
 */
static napi_value ZoryaIniSections(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument: context");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get sections - returns internal array, DO NOT FREE */
    size_t count = 0;
    const char **sections = zorya_ini_sections(ini, &count);
    
    if (sections == NULL || count == 0) {
        napi_value empty_array;
        NAPI_CALL(env, napi_create_array_with_length(env, 0, &empty_array));
        return empty_array;
    }
    
    /* Create JavaScript array */
    napi_value result;
    NAPI_CALL(env, napi_create_array_with_length(env, count, &result));
    
    for (size_t i = 0; i < count; i++) {
        napi_value item;
        NAPI_CALL(env, napi_create_string_utf8(env, sections[i], NAPI_AUTO_LENGTH, &item));
        NAPI_CALL(env, napi_set_element(env, result, (uint32_t)i, item));
    }
    
    /* Note: sections array is internal to ini - do NOT free */
    
    return result;
}

/* ============================================================
 * ZORYA_INI_TO_STRING - Serialize to INI string
 * ============================================================ */

/**
 * @brief Serialize INI context to string
 * 
 * JavaScript: const str = ini.toString(ctx);
 * 
 * Note: Simple implementation - returns "[not implemented]" placeholder
 * until zorya_ini_to_string is implemented in zorya_ini.c
 */
static napi_value ZoryaIniToString(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument: context");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* TODO: Implement proper serialization when zorya_ini_to_string is available */
    /* For now, build a simple representation */
    const char *placeholder = "; ZORYA-INI serialization\n; (Full implementation pending)\n[serialized]\nstatus = not_implemented\n";
    
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, placeholder, NAPI_AUTO_LENGTH, &result));
    
    return result;
}

/* ============================================================
 * ZORYA_INI_SAVE - Save to file
 * ============================================================ */

/**
 * @brief Save INI context to file
 * 
 * JavaScript: ini.save(ctx, '/path/to/output.ini');
 * 
 * Note: Not yet implemented - returns false until zorya_ini_save
 * is implemented in zorya_ini.c
 */
static napi_value ZoryaIniSave(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: context, filepath");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    (void)ini;  /* Unused until zorya_ini_save is implemented */
    
    /* TODO: Implement when zorya_ini_save is available */
    napi_throw_error(env, NULL, "zorya_ini_save not yet implemented");
    return NULL;
}

/* ============================================================
 * ZORYA_INI_STATS - Get statistics
 * ============================================================ */

/**
 * @brief Get INI statistics
 * 
 * JavaScript: const stats = ini.stats(ctx);
 */
static napi_value napi_get_stats(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument: context");
        return NULL;
    }
    
    /* Get context */
    ZoryaIni *ini;
    NAPI_CALL(env, napi_get_value_external(env, args[0], (void**)&ini));
    
    /* Get stats */
    ZoryaIniStats ini_stats;
    zorya_ini_stats(ini, &ini_stats);
    
    /* Create JavaScript object */
    napi_value result;
    NAPI_CALL(env, napi_create_object(env, &result));
    
    napi_value sections, keys, includes, memory, load_factor;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)ini_stats.section_count, &sections));
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)ini_stats.key_count, &keys));
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)ini_stats.include_count, &includes));
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)ini_stats.memory_bytes, &memory));
    NAPI_CALL(env, napi_create_double(env, ini_stats.load_factor, &load_factor));
    
    NAPI_CALL(env, napi_set_named_property(env, result, "sections", sections));
    NAPI_CALL(env, napi_set_named_property(env, result, "keys", keys));
    NAPI_CALL(env, napi_set_named_property(env, result, "includes", includes));
    NAPI_CALL(env, napi_set_named_property(env, result, "memoryBytes", memory));
    NAPI_CALL(env, napi_set_named_property(env, result, "loadFactor", load_factor));
    
    return result;
}

/* ============================================================
 * MODULE INITIALIZATION
 * ============================================================ */

/**
 * @brief Initialize the N-API module
 */
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        /* Lifecycle */
        DECLARE_NAPI_METHOD("create", ZoryaIniNew),
        DECLARE_NAPI_METHOD("free", ZoryaIniFree),
        
        /* Loading */
        DECLARE_NAPI_METHOD("load", ZoryaIniLoad),
        DECLARE_NAPI_METHOD("loadString", ZoryaIniLoadString),
        
        /* Getters */
        DECLARE_NAPI_METHOD("get", ZoryaIniGet),
        DECLARE_NAPI_METHOD("getDefault", ZoryaIniGetDefault),
        DECLARE_NAPI_METHOD("getInt", ZoryaIniGetInt),
        DECLARE_NAPI_METHOD("getFloat", ZoryaIniGetFloat),
        DECLARE_NAPI_METHOD("getBool", ZoryaIniGetBool),
        DECLARE_NAPI_METHOD("getArray", ZoryaIniGetArray),
        
        /* Existence */
        DECLARE_NAPI_METHOD("has", ZoryaIniHas),
        
        /* Setters */
        DECLARE_NAPI_METHOD("set", ZoryaIniSet),
        
        /* Serialization */
        DECLARE_NAPI_METHOD("toString", ZoryaIniToString),
        DECLARE_NAPI_METHOD("save", ZoryaIniSave),
        
        /* Metadata */
        DECLARE_NAPI_METHOD("sections", ZoryaIniSections),
        DECLARE_NAPI_METHOD("stats", napi_get_stats),
    };
    
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    
    /* Add version string */
    napi_value version;
    NAPI_CALL(env, napi_create_string_utf8(env, ZORYA_INI_VERSION_STRING, NAPI_AUTO_LENGTH, &version));
    NAPI_CALL(env, napi_set_named_property(env, exports, "version", version));
    
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
