/**
 * @file weave_napi.c
 * @brief NAPI bindings for Weave string library and Arena allocator
 *
 * @author Anthony Taliento
 * @date 2025-12-22
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * DESCRIPTION:
 *   Node.js NAPI bindings exposing Weave's industrial-strength string
 *   handling and Arena's high-performance memory allocation to JavaScript.
 *
 * EXPOSED CLASSES:
 *   - Weave: Mutable fat strings with O(1) length
 *   - Tablet: Interned string pool with O(1) comparison
 *   - Cord: Deferred concatenation (rope-lite)
 *   - Arena: Bump allocator with temp scopes
 */

/* Use stable NAPI version */
#define NAPI_VERSION 8

#include <node_api.h>
#include <stdlib.h>
#include <string.h>

/* Include Weave with its dependency prefix for zorya/ */
#define WEAVE_EMBEDDED 1
#include "weave.h"
#include "zorya_arena.h"

/* ============================================================
 * Version Info
 * ============================================================ */

#define WEAVE_NAPI_VERSION "1.0.0"

/* ============================================================
 * Utility Macros
 * ============================================================ */

#define NAPI_CALL(env, call)                                      \
  do {                                                            \
    napi_status status = (call);                                  \
    if (status != napi_ok) {                                      \
      napi_throw_error(env, NULL, "NAPI call failed: " #call);    \
      return NULL;                                                \
    }                                                             \
  } while(0)

#define NAPI_CALL_VOID(env, call)                                 \
  do {                                                            \
    napi_status status = (call);                                  \
    if (status != napi_ok) {                                      \
      napi_throw_error(env, NULL, "NAPI call failed: " #call);    \
      return;                                                     \
    }                                                             \
  } while(0)

/* ============================================================
 * Weave Class Bindings
 * ============================================================
 *
 * We use a wrapper struct because Weave* can be reallocated
 * during mutation (append/prepend/reserve). The wrapper lets
 * us update the pointer while keeping the same external.
 */

typedef struct WeaveWrapper {
    Weave *w;
} WeaveWrapper;

static void weave_destructor(napi_env env, void *data, void *hint) {
    (void)env;
    (void)hint;
    WeaveWrapper *ww = (WeaveWrapper *)data;
    if (ww != NULL) {
        if (ww->w != NULL) {
            weave_free(ww->w);
        }
        free(ww);
    }
}

/**
 * @brief Weave.create(str) - Create new Weave from string
 * 
 * Uses WeaveWrapper to handle pointer updates after realloc operations.
 */
static napi_value weave_js_create(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    Weave *w = NULL;

    if (argc == 0) {
        w = weave_new("");
    } else {
        size_t len;
        NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], NULL, 0, &len));
        
        char *buf = malloc(len + 1);
        if (buf == NULL) {
            napi_throw_error(env, NULL, "Memory allocation failed");
            return NULL;
        }
        
        NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], buf, len + 1, &len));
        w = weave_new_len(buf, len);
        free(buf);
    }

    if (w == NULL) {
        napi_throw_error(env, NULL, "Failed to create Weave");
        return NULL;
    }

    /* Wrap in WeaveWrapper to handle pointer updates after realloc */
    WeaveWrapper *ww = malloc(sizeof(WeaveWrapper));
    if (ww == NULL) {
        weave_free(w);
        napi_throw_error(env, NULL, "Memory allocation failed for wrapper");
        return NULL;
    }
    ww->w = w;

    napi_value external;
    NAPI_CALL(env, napi_create_external(env, ww, weave_destructor, NULL, &external));
    return external;
}

/**
 * @brief Weave.append(weave, str) - Append string to Weave
 *
 * NOTE: weave_append may realloc the Weave, but we can't update
 * the external's pointer. Solution: always return argv[0] and
 * accept that the pointer in the external may be stale if realloc
 * moved it. This works because we only have one reference.
 *
 * Better solution: Use a wrapper struct that holds Weave**.
 * For now, this is safe because we control all access.
 */
static napi_value weave_js_append(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    if (argc < 2) {
        napi_throw_error(env, NULL, "append requires 2 arguments");
        return NULL;
    }

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    size_t len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &len));
    
    char *buf = malloc(len + 1);
    if (buf == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], buf, len + 1, &len));
    Weave *result = weave_append_len(ww->w, buf, len);
    free(buf);

    if (result == NULL) {
        napi_throw_error(env, NULL, "Append failed");
        return NULL;
    }

    /* Update wrapper pointer in place - no new external needed */
    ww->w = result;
    return argv[0];  /* Return the same external */
}

/**
 * @brief Weave.toString(weave) - Get string value
 */
static napi_value weave_js_to_string(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, weave_cstr(ww->w), weave_len(ww->w), &result));
    return result;
}

/**
 * @brief Weave.length(weave) - Get length in O(1)
 */
static napi_value weave_js_length(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)weave_len(ww->w), &result));
    return result;
}

/**
 * @brief Weave.capacity(weave) - Get allocated capacity
 */
static napi_value weave_js_capacity(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)weave_cap(ww->w), &result));
    return result;
}

/**
 * @brief Weave.reserve(weave, capacity) - Pre-allocate capacity
 */
static napi_value weave_js_reserve(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    uint32_t cap;
    NAPI_CALL(env, napi_get_value_uint32(env, argv[1], &cap));

    Weave *result = weave_reserve(ww->w, cap);
    if (result == NULL) {
        napi_throw_error(env, NULL, "Reserve failed");
        return NULL;
    }

    /* Update wrapper pointer in place */
    ww->w = result;
    return argv[0];
}

/**
 * @brief Weave.prepend(weave, str) - Prepend string
 */
static napi_value weave_js_prepend(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    size_t len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &len));
    
    char *buf = malloc(len + 1);
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], buf, len + 1, &len));
    
    Weave *result = weave_prepend_len(ww->w, buf, len);
    free(buf);

    /* Update wrapper pointer in place */
    ww->w = result;
    return argv[0];
}

/**
 * @brief Weave.find(weave, needle) - Find substring
 */
static napi_value weave_js_find(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    size_t len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &len));
    
    char *needle = malloc(len + 1);
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], needle, len + 1, &len));
    
    int64_t idx = weave_find(ww->w, needle);
    free(needle);

    napi_value result;
    NAPI_CALL(env, napi_create_int64(env, idx, &result));
    return result;
}

/**
 * @brief Weave.contains(weave, needle) - Check if contains
 */
static napi_value weave_js_contains(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    size_t len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &len));
    
    char *needle = malloc(len + 1);
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], needle, len + 1, &len));
    
    bool found = weave_contains(ww->w, needle);
    free(needle);

    napi_value result;
    NAPI_CALL(env, napi_create_int32(env, found ? 1 : 0, &result));
    return result;
}

/**
 * @brief Weave.replace(weave, old, new) - Replace first occurrence
 */
static napi_value weave_js_replace(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    size_t old_len, new_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &old_len));
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[2], NULL, 0, &new_len));
    
    char *old_str = malloc(old_len + 1);
    char *new_str = malloc(new_len + 1);
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], old_str, old_len + 1, &old_len));
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[2], new_str, new_len + 1, &new_len));
    
    Weave *result = weave_replace(ww->w, old_str, new_str);
    free(old_str);
    free(new_str);

    if (result == NULL) {
        napi_throw_error(env, NULL, "Replace failed");
        return NULL;
    }

    /* Update wrapper pointer in place */
    ww->w = result;
    return argv[0];
}

/**
 * @brief Weave.replaceAll(weave, old, new) - Replace all occurrences
 */
static napi_value weave_js_replace_all(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    size_t old_len, new_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &old_len));
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[2], NULL, 0, &new_len));
    
    char *old_str = malloc(old_len + 1);
    char *new_str = malloc(new_len + 1);
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], old_str, old_len + 1, &old_len));
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[2], new_str, new_len + 1, &new_len));
    
    Weave *result = weave_replace_all(ww->w, old_str, new_str);
    free(old_str);
    free(new_str);

    if (result == NULL) {
        napi_throw_error(env, NULL, "Replace all failed");
        return NULL;
    }

    /* Update wrapper pointer in place */
    ww->w = result;
    return argv[0];
}

/**
 * @brief Weave.trim(weave) - Trim whitespace
 */
static napi_value weave_js_trim(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    Weave *result = weave_trim(ww->w);
    if (result == NULL) {
        napi_throw_error(env, NULL, "Trim failed");
        return NULL;
    }

    /* Update wrapper pointer in place */
    ww->w = result;
    return argv[0];
}

/**
 * @brief Weave.split(weave, delimiter) - Split into array
 * 
 * Note: This function returns an array of strings, not Weaves.
 * The returned strings are copied and the internal Weave array is freed.
 */
static napi_value weave_js_split(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    WeaveWrapper *ww;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&ww));

    size_t delim_len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &delim_len));
    
    char *delim = malloc(delim_len + 1);
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], delim, delim_len + 1, &delim_len));
    
    size_t count = 0;
    Weave **parts = weave_split(ww->w, delim, &count);
    free(delim);

    if (parts == NULL) {
        napi_throw_error(env, NULL, "Split failed");
        return NULL;
    }

    napi_value arr;
    NAPI_CALL(env, napi_create_array_with_length(env, count, &arr));

    for (size_t i = 0; i < count; i++) {
        napi_value str;
        NAPI_CALL(env, napi_create_string_utf8(env, weave_cstr(parts[i]), weave_len(parts[i]), &str));
        NAPI_CALL(env, napi_set_element(env, arr, (uint32_t)i, str));
    }

    weave_free_array(parts, count);
    return arr;
}

/* ============================================================
 * Cord Class Bindings (Deferred Concatenation)
 * ============================================================ */

static void cord_destructor(napi_env env, void *data, void *hint) {
    (void)env;
    (void)hint;
    Cord *c = (Cord *)data;
    if (c != NULL) {
        cord_free(c);
    }
}

/**
 * @brief Cord.create() - Create new Cord
 */
static napi_value cord_js_create(napi_env env, napi_callback_info info) {
    (void)info;
    
    Cord *c = cord_new();
    if (c == NULL) {
        napi_throw_error(env, NULL, "Failed to create Cord");
        return NULL;
    }

    napi_value external;
    NAPI_CALL(env, napi_create_external(env, c, cord_destructor, NULL, &external));
    return external;
}

/**
 * @brief Cord.append(cord, str) - Append string to Cord
 */
static napi_value cord_js_append(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    Cord *c;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&c));

    size_t len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &len));
    
    char *buf = malloc(len + 1);
    if (buf == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], buf, len + 1, &len));
    
    /* cord_append_len makes its own copy, so we free buf after */
    Cord *result = cord_append_len(c, buf, len);
    free(buf);
    
    if (result == NULL) {
        napi_throw_error(env, NULL, "Cord append failed");
        return NULL;
    }

    return argv[0];  /* Return same Cord for chaining */
}

/**
 * @brief Cord.length(cord) - Get total length
 */
static napi_value cord_js_length(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    Cord *c;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&c));

    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)cord_len(c), &result));
    return result;
}

/**
 * @brief Cord.chunkCount(cord) - Get number of chunks
 */
static napi_value cord_js_chunk_count(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    Cord *c;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&c));

    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)cord_chunk_count(c), &result));
    return result;
}

/**
 * @brief Cord.toString(cord) - Flatten to string (single allocation)
 */
static napi_value cord_js_to_string(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    Cord *c;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&c));

    char *str = cord_to_cstr(c);
    if (str == NULL) {
        napi_throw_error(env, NULL, "Cord to string failed");
        return NULL;
    }

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, str, cord_len(c), &result));
    free(str);
    return result;
}

/**
 * @brief Cord.clear(cord) - Clear all chunks
 */
static napi_value cord_js_clear(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    Cord *c;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&c));

    cord_clear(c);
    return argv[0];
}

/* ============================================================
 * Tablet Class Bindings (Interned Strings)
 * ============================================================ */

static void tablet_destructor(napi_env env, void *data, void *hint) {
    (void)env;
    (void)hint;
    Tablet *T = (Tablet *)data;
    if (T != NULL) {
        tablet_destroy(T);
    }
}

/**
 * @brief Tablet.create() - Create new string pool
 */
static napi_value tablet_js_create(napi_env env, napi_callback_info info) {
    (void)info;
    
    Tablet *T = tablet_create();
    if (T == NULL) {
        napi_throw_error(env, NULL, "Failed to create Tablet");
        return NULL;
    }

    napi_value external;
    NAPI_CALL(env, napi_create_external(env, T, tablet_destructor, NULL, &external));
    return external;
}

/**
 * @brief Tablet.intern(tablet, str) - Intern string, return ID
 */
static napi_value tablet_js_intern(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    Tablet *T;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&T));

    size_t len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &len));
    
    char *buf = malloc(len + 1);
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], buf, len + 1, &len));
    
    const Weave *interned = tablet_intern_len(T, buf, len);
    free(buf);

    if (interned == NULL) {
        napi_throw_error(env, NULL, "Intern failed");
        return NULL;
    }

    /* Return pointer as BigInt for O(1) comparison */
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, (uint64_t)(uintptr_t)interned, &result));
    return result;
}

/**
 * @brief Tablet.get(tablet, id) - Get string by ID
 */
static napi_value tablet_js_get(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    Tablet *T;
    (void)T;  /* Unused - we just need the pointer */
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&T));

    uint64_t id;
    bool lossless;
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, argv[1], &id, &lossless));

    const Weave *w = (const Weave *)(uintptr_t)id;
    
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, weave_cstr(w), weave_len(w), &result));
    return result;
}

/**
 * @brief Tablet.count(tablet) - Get number of interned strings
 */
static napi_value tablet_js_count(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    Tablet *T;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&T));

    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)tablet_count(T), &result));
    return result;
}

/**
 * @brief Tablet.memory(tablet) - Get memory usage
 */
static napi_value tablet_js_memory(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    Tablet *T;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&T));

    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)tablet_memory(T), &result));
    return result;
}

/* ============================================================
 * Arena Class Bindings
 * ============================================================ */

typedef struct ArenaWrapper {
    Arena *arena;
    ArenaChunk *temp_chunk;  /* Saved for temp scope */
    size_t temp_used;
} ArenaWrapper;

static void arena_destructor(napi_env env, void *data, void *hint) {
    (void)env;
    (void)hint;
    ArenaWrapper *aw = (ArenaWrapper *)data;
    if (aw != NULL) {
        if (aw->arena != NULL) {
            arena_destroy(aw->arena);
        }
        free(aw);
    }
}

/**
 * @brief Arena.create(size?) - Create new arena
 */
static napi_value arena_js_create(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    size_t chunk_size = 0;
    if (argc > 0) {
        uint32_t size;
        NAPI_CALL(env, napi_get_value_uint32(env, argv[0], &size));
        chunk_size = size;
    }

    ArenaWrapper *aw = malloc(sizeof(ArenaWrapper));
    if (aw == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }

    aw->arena = arena_create(chunk_size);
    aw->temp_chunk = NULL;
    aw->temp_used = 0;

    if (aw->arena == NULL) {
        free(aw);
        napi_throw_error(env, NULL, "Failed to create Arena");
        return NULL;
    }

    napi_value external;
    NAPI_CALL(env, napi_create_external(env, aw, arena_destructor, NULL, &external));
    return external;
}

/**
 * @brief Arena.alloc(arena, size) - Allocate bytes, return as Buffer
 */
static napi_value arena_js_alloc(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    ArenaWrapper *aw;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&aw));

    uint32_t size;
    NAPI_CALL(env, napi_get_value_uint32(env, argv[1], &size));

    void *ptr = arena_alloc(aw->arena, size);
    if (ptr == NULL) {
        napi_throw_error(env, NULL, "Arena allocation failed");
        return NULL;
    }

    /* Return as external buffer (no copy) */
    napi_value buffer;
    NAPI_CALL(env, napi_create_external_buffer(env, size, ptr, NULL, NULL, &buffer));
    return buffer;
}

/**
 * @brief Arena.allocString(arena, str) - Allocate string in arena
 */
static napi_value arena_js_alloc_string(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    ArenaWrapper *aw;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&aw));

    size_t len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], NULL, 0, &len));
    
    char *str = arena_alloc(aw->arena, len + 1);
    if (str == NULL) {
        napi_throw_error(env, NULL, "Arena string allocation failed");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, argv[1], str, len + 1, &len));

    /* Return pointer as BigInt for later retrieval */
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, (uint64_t)(uintptr_t)str, &result));
    return result;
}

/**
 * @brief Arena.getString(arena, ptr) - Get string from arena pointer
 */
static napi_value arena_js_get_string(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    /* Don't need arena, just the pointer */
    uint64_t ptr;
    bool lossless;
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, argv[1], &ptr, &lossless));

    const char *str = (const char *)(uintptr_t)ptr;
    
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &result));
    return result;
}

/**
 * @brief Arena.tempBegin(arena) - Save arena state
 */
static napi_value arena_js_temp_begin(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    ArenaWrapper *aw;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&aw));

    /* Save current state */
    aw->temp_chunk = aw->arena->current;
    aw->temp_used = aw->arena->current->used;

    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

/**
 * @brief Arena.tempEnd(arena) - Restore arena state (free all since tempBegin)
 */
static napi_value arena_js_temp_end(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    ArenaWrapper *aw;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&aw));

    if (aw->temp_chunk == NULL) {
        napi_throw_error(env, NULL, "No temp scope active");
        return NULL;
    }

    /* Free chunks allocated after temp begin */
    ArenaChunk *chunk = aw->temp_chunk->next;
    while (chunk != NULL) {
        ArenaChunk *next = chunk->next;
        free(chunk);
        chunk = next;
    }

    /* Restore state */
    aw->temp_chunk->next = NULL;
    aw->temp_chunk->used = aw->temp_used;
    aw->arena->current = aw->temp_chunk;
    aw->temp_chunk = NULL;
    aw->temp_used = 0;

    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

/**
 * @brief Arena.reset(arena) - Reset arena (keep first chunk)
 */
static napi_value arena_js_reset(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    ArenaWrapper *aw;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&aw));

    arena_reset(aw->arena);
    aw->temp_chunk = NULL;
    aw->temp_used = 0;

    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

/**
 * @brief Arena.stats(arena) - Get arena statistics
 */
static napi_value arena_js_stats(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));

    ArenaWrapper *aw;
    NAPI_CALL(env, napi_get_value_external(env, argv[0], (void **)&aw));

    size_t allocated, capacity, chunks;
    arena_stats(aw->arena, &allocated, &capacity, &chunks);

    napi_value result;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value val;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)allocated, &val));
    NAPI_CALL(env, napi_set_named_property(env, result, "allocated", val));

    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)capacity, &val));
    NAPI_CALL(env, napi_set_named_property(env, result, "capacity", val));

    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)chunks, &val));
    NAPI_CALL(env, napi_set_named_property(env, result, "chunks", val));

    double util = capacity > 0 ? (double)allocated / (double)capacity : 0.0;
    NAPI_CALL(env, napi_create_double(env, util, &val));
    NAPI_CALL(env, napi_set_named_property(env, result, "utilization", val));

    return result;
}

/* ============================================================
 * Version Info
 * ============================================================ */

static napi_value get_version(napi_env env, napi_callback_info info) {
    (void)info;
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, WEAVE_NAPI_VERSION, NAPI_AUTO_LENGTH, &result));
    return result;
}

static napi_value get_weave_version(napi_env env, napi_callback_info info) {
    (void)info;
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, WEAVE_VERSION_STRING, NAPI_AUTO_LENGTH, &result));
    return result;
}

/* ============================================================
 * Module Initialization
 * ============================================================ */

static napi_value Init(napi_env env, napi_value exports) {
    napi_value fn;
    
    /* Helper macro to export a function */
    #define EXPORT_FN(name, func) \
        napi_create_function(env, name, NAPI_AUTO_LENGTH, func, NULL, &fn); \
        napi_set_named_property(env, exports, name, fn);
    
    /* Version */
    EXPORT_FN("version", get_version);
    EXPORT_FN("weaveVersion", get_weave_version);
    
    /* Weave */
    EXPORT_FN("weaveCreate", weave_js_create);
    EXPORT_FN("weaveAppend", weave_js_append);
    EXPORT_FN("weavePrepend", weave_js_prepend);
    EXPORT_FN("weaveToString", weave_js_to_string);
    EXPORT_FN("weaveLength", weave_js_length);
    EXPORT_FN("weaveCapacity", weave_js_capacity);
    EXPORT_FN("weaveReserve", weave_js_reserve);
    EXPORT_FN("weaveFind", weave_js_find);
    EXPORT_FN("weaveContains", weave_js_contains);
    EXPORT_FN("weaveReplace", weave_js_replace);
    EXPORT_FN("weaveReplaceAll", weave_js_replace_all);
    EXPORT_FN("weaveTrim", weave_js_trim);
    EXPORT_FN("weaveSplit", weave_js_split);
    
    /* Cord */
    EXPORT_FN("cordCreate", cord_js_create);
    EXPORT_FN("cordAppend", cord_js_append);
    EXPORT_FN("cordLength", cord_js_length);
    EXPORT_FN("cordChunkCount", cord_js_chunk_count);
    EXPORT_FN("cordToString", cord_js_to_string);
    EXPORT_FN("cordClear", cord_js_clear);
    
    /* Tablet */
    EXPORT_FN("tabletCreate", tablet_js_create);
    EXPORT_FN("tabletIntern", tablet_js_intern);
    EXPORT_FN("tabletGet", tablet_js_get);
    EXPORT_FN("tabletCount", tablet_js_count);
    EXPORT_FN("tabletMemory", tablet_js_memory);
    
    /* Arena */
    EXPORT_FN("arenaCreate", arena_js_create);
    EXPORT_FN("arenaAlloc", arena_js_alloc);
    EXPORT_FN("arenaAllocString", arena_js_alloc_string);
    EXPORT_FN("arenaGetString", arena_js_get_string);
    EXPORT_FN("arenaTempBegin", arena_js_temp_begin);
    EXPORT_FN("arenaTempEnd", arena_js_temp_end);
    EXPORT_FN("arenaReset", arena_js_reset);
    EXPORT_FN("arenaStats", arena_js_stats);
    
    #undef EXPORT_FN
    
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
