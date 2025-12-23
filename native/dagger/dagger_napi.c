/**
 * @file zorya_dagger_napi.c
 * @brief N-API bindings for DAGGER O(1) hash table
 *
 * @author Anthony Taliento
 * @date 2025-12-08
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * DESCRIPTION:
 *   Node.js N-API bindings for the DAGGER hash table library.
 *   Provides O(1) guaranteed lookup performance with Robin Hood hashing
 *   and Cuckoo fallback. Ideal for large caches, registries, and maps
 *   where JS Map() overhead becomes significant.
 *
 * EXPORTS:
 *   DaggerTable class:
 *     - new DaggerTable(capacity?)
 *     - set(key, value)
 *     - get(key) -> value | undefined
 *     - has(key) -> boolean
 *     - delete(key) -> boolean
 *     - clear()
 *     - keys() -> string[]
 *     - values() -> any[]
 *     - entries() -> [key, value][]
 *     - forEach(callback)
 *     - size (getter)
 *     - capacity (getter)
 *     - stats() -> DaggerStats
 *
 * WHY NATIVE:
 *   - JS Map() has 100+ byte overhead per entry
 *   - DAGGER: ~1 byte overhead per slot (40 byte entries)
 *   - Robin Hood minimizes probe variance (cache-friendly)
 *   - Cuckoo fallback guarantees O(1) worst case
 *   - Direct memcmp comparison (no JS string interning overhead)
 *
 * DEPENDENCIES:
 *   - dagger.c, nxh.c
 *   - node_api.h (N-API headers)
 *
 * THREAD SAFETY:
 *   Not thread-safe. Each DaggerTable instance should be used from one thread.
 */

#include <node_api.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dagger.h"

/* ============================================================
 * CONSTANTS
 * ============================================================ */

/** Default initial capacity */
#define DEFAULT_CAPACITY 64

/* ============================================================
 * HELPER MACROS
 * ============================================================ */

#define NAPI_CALL(env, call)                                                   \
    do {                                                                       \
        napi_status status = (call);                                           \
        if (status != napi_ok) {                                               \
            const napi_extended_error_info *error_info = NULL;                 \
            napi_get_last_error_info((env), &error_info);                      \
            napi_throw_error((env), NULL,                                      \
                error_info->error_message ?                                    \
                error_info->error_message : "N-API call failed");              \
            return NULL;                                                       \
        }                                                                      \
    } while (0)

#define NAPI_CALL_VOID(env, call)                                              \
    do {                                                                       \
        napi_status status = (call);                                           \
        if (status != napi_ok) {                                               \
            const napi_extended_error_info *error_info = NULL;                 \
            napi_get_last_error_info((env), &error_info);                      \
            napi_throw_error((env), NULL,                                      \
                error_info->error_message ?                                    \
                error_info->error_message : "N-API call failed");              \
            return;                                                            \
        }                                                                      \
    } while (0)

/* ============================================================
 * WRAPPER STRUCTURE
 * 
 * Wraps DaggerTable with N-API reference tracking for values.
 * JS values are stored as napi_ref to prevent garbage collection.
 * ============================================================ */

typedef struct {
    DaggerTable *table;       /**< Underlying DAGGER table */
    napi_env env;             /**< Cached environment (for cleanup) */
} NapiDaggerTable;

/**
 * @brief Value wrapper to hold napi_ref
 * 
 * Primitives (number, string, boolean, null, undefined, bigint, symbol)
 * cannot have references created directly in N-API. We wrap them in
 * an object { __dagger_value__: primitive } and set is_primitive=true.
 */
typedef struct {
    napi_ref ref;             /**< Reference to JS value (or wrapper object for primitives) */
    napi_env env;             /**< Environment for the ref */
    int is_primitive;         /**< 1 if value was wrapped (primitive type) */
} DaggerValue;

/* ============================================================
 * VALUE MANAGEMENT
 * ============================================================ */

/**
 * @brief Destroy a DaggerValue (releases napi_ref)
 */
static void dagger_value_destructor(void *ptr) {
    DaggerValue *val = (DaggerValue *)ptr;
    if (val != NULL) {
        if (val->ref != NULL && val->env != NULL) {
            napi_delete_reference(val->env, val->ref);
        }
        free(val);
    }
}

/**
 * @brief Destroy a key (keys are copied, so free the copy)
 */
static void dagger_key_destructor(const void *key, uint32_t key_len) {
    (void)key_len;
    free((void *)key);
}

/* ============================================================
 * CLASS CONSTRUCTOR / DESTRUCTOR
 * ============================================================ */

/**
 * @brief Destructor called when JS object is garbage collected
 */
static void napi_dagger_destructor(napi_env env, void *data, void *hint) {
    (void)env;
    (void)hint;
    
    NapiDaggerTable *wrapper = (NapiDaggerTable *)data;
    if (wrapper != NULL) {
        if (wrapper->table != NULL) {
            dagger_destroy(wrapper->table);
            wrapper->table = NULL;
        }
        free(wrapper);
    }
}

/**
 * @brief Constructor: new DaggerTable(capacity?)
 */
static napi_value napi_dagger_constructor(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &this_arg, NULL));
    
    /* Get capacity (optional) */
    size_t capacity = DEFAULT_CAPACITY;
    if (argc >= 1) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, argv[0], &type));
        if (type == napi_number) {
            uint32_t cap;
            NAPI_CALL(env, napi_get_value_uint32(env, argv[0], &cap));
            capacity = (size_t)cap;
        }
    }
    
    /* Create wrapper */
    NapiDaggerTable *wrapper = (NapiDaggerTable *)calloc(1, sizeof(NapiDaggerTable));
    if (wrapper == NULL) {
        napi_throw_error(env, NULL, "Failed to allocate DaggerTable wrapper");
        return NULL;
    }
    
    /* Create DAGGER table */
    wrapper->table = dagger_create(capacity, NULL);
    if (wrapper->table == NULL) {
        free(wrapper);
        napi_throw_error(env, NULL, "Failed to create DaggerTable");
        return NULL;
    }
    
    /* Set up destructors */
    wrapper->env = env;
    dagger_set_value_destroy(wrapper->table, dagger_value_destructor);
    dagger_set_key_destroy(wrapper->table, dagger_key_destructor);
    
    /* Wrap native data */
    NAPI_CALL(env, napi_wrap(env, this_arg, wrapper, napi_dagger_destructor, NULL, NULL));
    
    return this_arg;
}

/* ============================================================
 * INSTANCE METHODS
 * ============================================================ */

/**
 * @brief set(key, value) -> this
 * 
 * Sets a key-value pair. Key is converted to string.
 * Value can be any JS value (stored by reference).
 */
static napi_value napi_dagger_set(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &this_arg, NULL));
    
    if (argc < 2) {
        napi_throw_error(env, NULL, "set() requires key and value arguments");
        return NULL;
    }
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    /* Convert key to string */
    napi_value key_str;
    NAPI_CALL(env, napi_coerce_to_string(env, argv[0], &key_str));
    
    size_t key_len = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, key_str, NULL, 0, &key_len));
    
    char *key_buf = (char *)malloc(key_len + 1);
    if (key_buf == NULL) {
        napi_throw_error(env, NULL, "Failed to allocate key buffer");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, key_str, key_buf, key_len + 1, &key_len));
    
    /* Create reference for value */
    DaggerValue *val_wrapper = (DaggerValue *)calloc(1, sizeof(DaggerValue));
    if (val_wrapper == NULL) {
        free(key_buf);
        napi_throw_error(env, NULL, "Failed to allocate value wrapper");
        return NULL;
    }
    
    /* 
     * N-API can only create references to objects, not primitives.
     * We need to wrap primitives in an object to store them.
     * We'll use a wrapper object: { __dagger_value__: primitive }
     */
    napi_value value_to_ref = argv[1];
    napi_valuetype value_type;
    NAPI_CALL(env, napi_typeof(env, argv[1], &value_type));
    
    bool is_primitive = (value_type == napi_undefined ||
                         value_type == napi_null ||
                         value_type == napi_boolean ||
                         value_type == napi_number ||
                         value_type == napi_string ||
                         value_type == napi_bigint ||
                         value_type == napi_symbol);
    
    val_wrapper->is_primitive = is_primitive;
    
    if (is_primitive) {
        /* Wrap primitive in an object */
        napi_value wrapper_obj;
        NAPI_CALL(env, napi_create_object(env, &wrapper_obj));
        NAPI_CALL(env, napi_set_named_property(env, wrapper_obj, "__dagger_value__", argv[1]));
        value_to_ref = wrapper_obj;
    }
    
    NAPI_CALL(env, napi_create_reference(env, value_to_ref, 1, &val_wrapper->ref));
    val_wrapper->env = env;
    
    /* Check if key exists - if so, the old value will be destroyed */
    dagger_result_t result = dagger_set(wrapper->table, key_buf, (uint32_t)key_len, val_wrapper, 1);
    
    if (result != DAGGER_OK) {
        napi_delete_reference(env, val_wrapper->ref);
        free(val_wrapper);
        free(key_buf);
        
        if (result == DAGGER_ERROR_NOMEM) {
            napi_throw_error(env, NULL, "Out of memory");
        } else {
            napi_throw_error(env, NULL, "Failed to set value");
        }
        return NULL;
    }
    
    /* Return this for chaining */
    return this_arg;
}

/**
 * @brief get(key) -> value | undefined
 */
static napi_value napi_dagger_get(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &this_arg, NULL));
    
    if (argc < 1) {
        napi_throw_error(env, NULL, "get() requires a key argument");
        return NULL;
    }
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    /* Convert key to string */
    napi_value key_str;
    NAPI_CALL(env, napi_coerce_to_string(env, argv[0], &key_str));
    
    size_t key_len = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, key_str, NULL, 0, &key_len));
    
    char *key_buf = (char *)malloc(key_len + 1);
    if (key_buf == NULL) {
        napi_throw_error(env, NULL, "Failed to allocate key buffer");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, key_str, key_buf, key_len + 1, &key_len));
    
    /* Lookup */
    void *value = NULL;
    dagger_result_t result = dagger_get(wrapper->table, key_buf, (uint32_t)key_len, &value);
    
    free(key_buf);
    
    if (result == DAGGER_NOT_FOUND || value == NULL) {
        napi_value undefined;
        NAPI_CALL(env, napi_get_undefined(env, &undefined));
        return undefined;
    }
    
    /* Get JS value from reference */
    DaggerValue *val_wrapper = (DaggerValue *)value;
    napi_value js_value;
    NAPI_CALL(env, napi_get_reference_value(env, val_wrapper->ref, &js_value));
    
    /* If this was a primitive, unwrap it from the wrapper object */
    if (val_wrapper->is_primitive) {
        napi_value actual_value;
        NAPI_CALL(env, napi_get_named_property(env, js_value, "__dagger_value__", &actual_value));
        return actual_value;
    }
    
    return js_value;
}

/**
 * @brief has(key) -> boolean
 */
static napi_value napi_dagger_has(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &this_arg, NULL));
    
    if (argc < 1) {
        napi_throw_error(env, NULL, "has() requires a key argument");
        return NULL;
    }
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    /* Convert key to string */
    napi_value key_str;
    NAPI_CALL(env, napi_coerce_to_string(env, argv[0], &key_str));
    
    size_t key_len = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, key_str, NULL, 0, &key_len));
    
    char *key_buf = (char *)malloc(key_len + 1);
    if (key_buf == NULL) {
        napi_throw_error(env, NULL, "Failed to allocate key buffer");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, key_str, key_buf, key_len + 1, &key_len));
    
    /* Check */
    int exists = dagger_contains(wrapper->table, key_buf, (uint32_t)key_len);
    
    free(key_buf);
    
    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, exists, &result));
    return result;
}

/**
 * @brief delete(key) -> boolean
 * 
 * Returns true if key existed and was deleted.
 */
static napi_value napi_dagger_delete(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &this_arg, NULL));
    
    if (argc < 1) {
        napi_throw_error(env, NULL, "delete() requires a key argument");
        return NULL;
    }
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    /* Convert key to string */
    napi_value key_str;
    NAPI_CALL(env, napi_coerce_to_string(env, argv[0], &key_str));
    
    size_t key_len = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, key_str, NULL, 0, &key_len));
    
    char *key_buf = (char *)malloc(key_len + 1);
    if (key_buf == NULL) {
        napi_throw_error(env, NULL, "Failed to allocate key buffer");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, key_str, key_buf, key_len + 1, &key_len));
    
    /* Remove */
    dagger_result_t result = dagger_remove(wrapper->table, key_buf, (uint32_t)key_len);
    
    free(key_buf);
    
    napi_value deleted;
    NAPI_CALL(env, napi_get_boolean(env, result == DAGGER_OK, &deleted));
    return deleted;
}

/**
 * @brief clear() -> void
 * 
 * Removes all entries.
 */
static napi_value napi_dagger_clear(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    dagger_clear(wrapper->table);
    
    napi_value undefined;
    NAPI_CALL(env, napi_get_undefined(env, &undefined));
    return undefined;
}

/* ============================================================
 * ITERATION HELPERS
 * ============================================================ */

typedef struct {
    napi_env env;
    napi_value array;
    uint32_t index;
    int include_values;  /* 0=keys, 1=values, 2=entries */
} IterContext;

/**
 * @brief Helper to get the actual JS value from a DaggerValue (unwraps primitives)
 */
static napi_value get_actual_value(napi_env env, DaggerValue *val_wrapper) {
    napi_value js_value;
    napi_get_reference_value(env, val_wrapper->ref, &js_value);
    
    if (val_wrapper->is_primitive) {
        napi_value actual_value;
        napi_get_named_property(env, js_value, "__dagger_value__", &actual_value);
        return actual_value;
    }
    return js_value;
}

/**
 * @brief Iterator callback for keys()/values()/entries()
 */
static int iter_callback(const void *key, uint32_t key_len, void *value, void *ctx) {
    IterContext *ic = (IterContext *)ctx;
    napi_value item;
    
    if (ic->include_values == 0) {
        /* keys() - just the key string */
        napi_create_string_utf8(ic->env, (const char *)key, key_len, &item);
    } else if (ic->include_values == 1) {
        /* values() - just the value */
        DaggerValue *val_wrapper = (DaggerValue *)value;
        item = get_actual_value(ic->env, val_wrapper);
    } else {
        /* entries() - [key, value] tuple */
        napi_value tuple;
        napi_create_array_with_length(ic->env, 2, &tuple);
        
        napi_value key_str;
        napi_create_string_utf8(ic->env, (const char *)key, key_len, &key_str);
        napi_set_element(ic->env, tuple, 0, key_str);
        
        DaggerValue *val_wrapper = (DaggerValue *)value;
        napi_value js_value = get_actual_value(ic->env, val_wrapper);
        napi_set_element(ic->env, tuple, 1, js_value);
        
        item = tuple;
    }
    
    napi_set_element(ic->env, ic->array, ic->index++, item);
    return 0; /* continue iteration (0 = continue, non-zero = stop) */
}

/**
 * @brief keys() -> string[]
 */
static napi_value napi_dagger_keys(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    /* Create result array */
    napi_value result;
    NAPI_CALL(env, napi_create_array_with_length(env, wrapper->table->count, &result));
    
    /* Iterate */
    IterContext ctx = { env, result, 0, 0 };
    dagger_foreach(wrapper->table, iter_callback, &ctx);
    
    return result;
}

/**
 * @brief values() -> any[]
 */
static napi_value napi_dagger_values(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    /* Create result array */
    napi_value result;
    NAPI_CALL(env, napi_create_array_with_length(env, wrapper->table->count, &result));
    
    /* Iterate */
    IterContext ctx = { env, result, 0, 1 };
    dagger_foreach(wrapper->table, iter_callback, &ctx);
    
    return result;
}

/**
 * @brief entries() -> [key, value][]
 */
static napi_value napi_dagger_entries(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    /* Create result array */
    napi_value result;
    NAPI_CALL(env, napi_create_array_with_length(env, wrapper->table->count, &result));
    
    /* Iterate */
    IterContext ctx = { env, result, 0, 2 };
    dagger_foreach(wrapper->table, iter_callback, &ctx);
    
    return result;
}

/* ============================================================
 * FOREACH WITH CALLBACK
 * ============================================================ */

typedef struct {
    napi_env env;
    napi_value callback;
    napi_value this_arg;
    napi_value table_obj;
} ForEachContext;

/**
 * @brief forEach iterator callback
 */
static int foreach_callback(const void *key, uint32_t key_len, void *value, void *ctx) {
    ForEachContext *fc = (ForEachContext *)ctx;
    
    /* Get value from reference (unwrap primitives) */
    DaggerValue *val_wrapper = (DaggerValue *)value;
    napi_value js_value = get_actual_value(fc->env, val_wrapper);
    
    /* Create key string */
    napi_value key_str;
    napi_create_string_utf8(fc->env, (const char *)key, key_len, &key_str);
    
    /* Call: callback(value, key, table) */
    napi_value argv[3] = { js_value, key_str, fc->table_obj };
    napi_value result;
    napi_call_function(fc->env, fc->this_arg, fc->callback, 3, argv, &result);
    
    return 0; /* continue iteration (0 = continue, non-zero = stop) */
}

/**
 * @brief forEach(callback, thisArg?) -> void
 */
static napi_value napi_dagger_forEach(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &this_arg, NULL));
    
    if (argc < 1) {
        napi_throw_error(env, NULL, "forEach() requires a callback argument");
        return NULL;
    }
    
    /* Check callback is function */
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type));
    if (type != napi_function) {
        napi_throw_type_error(env, NULL, "callback must be a function");
        return NULL;
    }
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    /* Get thisArg (or use undefined) */
    napi_value callback_this;
    if (argc >= 2) {
        callback_this = argv[1];
    } else {
        NAPI_CALL(env, napi_get_undefined(env, &callback_this));
    }
    
    /* Iterate */
    ForEachContext ctx = { env, argv[0], callback_this, this_arg };
    dagger_foreach(wrapper->table, foreach_callback, &ctx);
    
    napi_value undefined;
    NAPI_CALL(env, napi_get_undefined(env, &undefined));
    return undefined;
}

/* ============================================================
 * PROPERTY GETTERS
 * ============================================================ */

/**
 * @brief size getter -> number
 */
static napi_value napi_dagger_size_getter(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)wrapper->table->count, &result));
    return result;
}

/**
 * @brief capacity getter -> number
 */
static napi_value napi_dagger_capacity_getter(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)wrapper->table->capacity, &result));
    return result;
}

/**
 * @brief stats() -> { count, capacity, maxPsl, cuckooCount, resizeCount, loadFactor, avgProbes }
 */
static napi_value napi_dagger_stats(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    /* Get wrapper */
    NapiDaggerTable *wrapper = NULL;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper == NULL || wrapper->table == NULL) {
        napi_throw_error(env, NULL, "Invalid DaggerTable");
        return NULL;
    }
    
    /* Get stats */
    DaggerStats stats;
    dagger_stats(wrapper->table, &stats);
    
    /* Build result object */
    napi_value result;
    NAPI_CALL(env, napi_create_object(env, &result));
    
    napi_value v;
    
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)stats.count, &v));
    NAPI_CALL(env, napi_set_named_property(env, result, "count", v));
    
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)stats.capacity, &v));
    NAPI_CALL(env, napi_set_named_property(env, result, "capacity", v));
    
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)stats.max_psl, &v));
    NAPI_CALL(env, napi_set_named_property(env, result, "maxPsl", v));
    
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)stats.cuckoo_count, &v));
    NAPI_CALL(env, napi_set_named_property(env, result, "cuckooCount", v));
    
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)stats.resize_count, &v));
    NAPI_CALL(env, napi_set_named_property(env, result, "resizeCount", v));
    
    NAPI_CALL(env, napi_create_double(env, stats.load_factor, &v));
    NAPI_CALL(env, napi_set_named_property(env, result, "loadFactor", v));
    
    NAPI_CALL(env, napi_create_double(env, stats.avg_probes, &v));
    NAPI_CALL(env, napi_set_named_property(env, result, "avgProbes", v));
    
    return result;
}

/* ============================================================
 * MODULE INITIALIZATION
 * ============================================================ */

static napi_value Init(napi_env env, napi_value exports) {
    /* Define properties for DaggerTable class */
    napi_property_descriptor props[] = {
        /* Methods */
        { "set", NULL, napi_dagger_set, NULL, NULL, NULL, napi_default, NULL },
        { "get", NULL, napi_dagger_get, NULL, NULL, NULL, napi_default, NULL },
        { "has", NULL, napi_dagger_has, NULL, NULL, NULL, napi_default, NULL },
        { "delete", NULL, napi_dagger_delete, NULL, NULL, NULL, napi_default, NULL },
        { "clear", NULL, napi_dagger_clear, NULL, NULL, NULL, napi_default, NULL },
        { "keys", NULL, napi_dagger_keys, NULL, NULL, NULL, napi_default, NULL },
        { "values", NULL, napi_dagger_values, NULL, NULL, NULL, napi_default, NULL },
        { "entries", NULL, napi_dagger_entries, NULL, NULL, NULL, napi_default, NULL },
        { "forEach", NULL, napi_dagger_forEach, NULL, NULL, NULL, napi_default, NULL },
        { "stats", NULL, napi_dagger_stats, NULL, NULL, NULL, napi_default, NULL },
        
        /* Getters */
        { "size", NULL, NULL, napi_dagger_size_getter, NULL, NULL, napi_default, NULL },
        { "capacity", NULL, NULL, napi_dagger_capacity_getter, NULL, NULL, napi_default, NULL },
    };
    
    napi_value constructor;
    NAPI_CALL(env, napi_define_class(
        env,
        "DaggerTable",
        NAPI_AUTO_LENGTH,
        napi_dagger_constructor,
        NULL,
        sizeof(props) / sizeof(props[0]),
        props,
        &constructor
    ));
    
    /* Export DaggerTable class */
    NAPI_CALL(env, napi_set_named_property(env, exports, "DaggerTable", constructor));
    
    /* Export version info */
    napi_value version;
    NAPI_CALL(env, napi_create_string_utf8(env, DAGGER_VERSION_STRING, NAPI_AUTO_LENGTH, &version));
    NAPI_CALL(env, napi_set_named_property(env, exports, "version", version));
    
    /* Export constants */
    napi_value psl_threshold;
    NAPI_CALL(env, napi_create_uint32(env, DAGGER_PSL_THRESHOLD, &psl_threshold));
    NAPI_CALL(env, napi_set_named_property(env, exports, "PSL_THRESHOLD", psl_threshold));
    
    napi_value load_factor;
    NAPI_CALL(env, napi_create_uint32(env, DAGGER_LOAD_FACTOR_PERCENT, &load_factor));
    NAPI_CALL(env, napi_set_named_property(env, exports, "LOAD_FACTOR_PERCENT", load_factor));
    
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
