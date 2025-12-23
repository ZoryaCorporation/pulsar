/**
 * @file zorya_nxh_napi.c
 * @brief N-API bindings for NXH (Nexus Hash) - High-performance hashing
 *
 * @author Anthony Taliento
 * @date 2025-12-08
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license MIT
 *
 * DESCRIPTION:
 *   Exposes NXH hash functions to Node.js/Electron via N-API.
 *   Provides 64-bit hashing that JavaScript cannot do efficiently
 *   (BigInt is 50-100x slower than native uint64_t).
 *
 * EXPORTS:
 *   - nxh64(buffer, seed?)        -> BigInt (64-bit hash)
 *   - nxh32(buffer, seed?)        -> Number (32-bit hash)
 *   - nxhString(str, seed?)       -> BigInt (hash string directly)
 *   - nxhString32(str, seed?)     -> Number (32-bit string hash)
 *   - nxhCombine(h1, h2)          -> BigInt (combine two hashes)
 *   - nxhInt64(value)             -> BigInt (hash a 64-bit integer)
 *   - nxhInt32(value)             -> BigInt (hash a 32-bit integer)
 *   - version                     -> String ("2.0.0")
 *
 * USAGE:
 *   const nxh = require('zorya-nxh');
 *   
 *   // Hash a buffer
 *   const hash = nxh.nxh64(Buffer.from('hello world'));
 *   console.log(hash);  // 1234567890123456789n (BigInt)
 *   
 *   // Hash a string directly
 *   const strHash = nxh.nxhString('hello world');
 *   
 *   // Combine hashes (for composite keys)
 *   const combined = nxh.nxhCombine(hash1, hash2);
 *
 * THREAD SAFETY:
 *   All functions are thread-safe (no shared state).
 */

#include <node_api.h>
#include <string.h>
#include <stdlib.h>

/* Include NXH header - it's header-only with inline implementations */
#include "nxh.h"

/* ============================================================
 * N-API HELPER MACROS
 * ============================================================ */

#define NAPI_CALL(env, call)                                          \
    do {                                                              \
        napi_status status = (call);                                  \
        if (status != napi_ok) {                                      \
            const napi_extended_error_info *error_info = NULL;        \
            napi_get_last_error_info((env), &error_info);             \
            const char *msg = (error_info && error_info->error_message) \
                ? error_info->error_message                           \
                : "Unknown N-API error";                              \
            napi_throw_error((env), NULL, msg);                       \
            return NULL;                                              \
        }                                                             \
    } while (0)

#define DECLARE_NAPI_METHOD(name, func)                               \
    { name, 0, func, 0, 0, 0, napi_default, 0 }

/* ============================================================
 * NXH64 - 64-bit hash of buffer
 * ============================================================ */

/**
 * @brief Hash a buffer with 64-bit output
 * 
 * JavaScript: const hash = nxh.nxh64(buffer, seed?);
 * 
 * @param buffer - Buffer or TypedArray to hash
 * @param seed   - Optional seed (BigInt, default 0)
 * @return BigInt - 64-bit hash value
 */
static napi_value Nxh64(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected at least 1 argument: buffer");
        return NULL;
    }
    
    /* Get buffer data */
    void *data = NULL;
    size_t length = 0;
    bool is_buffer = false;
    
    NAPI_CALL(env, napi_is_buffer(env, args[0], &is_buffer));
    
    if (is_buffer) {
        NAPI_CALL(env, napi_get_buffer_info(env, args[0], &data, &length));
    } else {
        /* Try as TypedArray */
        bool is_typedarray = false;
        NAPI_CALL(env, napi_is_typedarray(env, args[0], &is_typedarray));
        
        if (is_typedarray) {
            napi_typedarray_type type;
            napi_value arraybuffer;
            size_t byte_offset;
            NAPI_CALL(env, napi_get_typedarray_info(env, args[0], &type, &length, 
                                                     &data, &arraybuffer, &byte_offset));
            /* Adjust for element size */
            size_t elem_size = 1;
            switch (type) {
                case napi_int16_array:
                case napi_uint16_array:
                    elem_size = 2;
                    break;
                case napi_int32_array:
                case napi_uint32_array:
                case napi_float32_array:
                    elem_size = 4;
                    break;
                case napi_float64_array:
                case napi_bigint64_array:
                case napi_biguint64_array:
                    elem_size = 8;
                    break;
                default:
                    elem_size = 1;
            }
            length *= elem_size;
        } else {
            napi_throw_type_error(env, NULL, "First argument must be Buffer or TypedArray");
            return NULL;
        }
    }
    
    /* Get optional seed */
    uint64_t seed = NXH_SEED_DEFAULT;
    if (argc >= 2) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, args[1], &type));
        
        if (type == napi_bigint) {
            bool lossless = true;
            NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[1], &seed, &lossless));
        } else if (type == napi_number) {
            double num;
            NAPI_CALL(env, napi_get_value_double(env, args[1], &num));
            seed = (uint64_t)num;
        }
    }
    
    /* Compute hash */
    uint64_t hash = nxh64(data, length, seed);
    
    /* Return as BigInt */
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, hash, &result));
    
    return result;
}

/* ============================================================
 * NXH32 - 32-bit hash of buffer
 * ============================================================ */

/**
 * @brief Hash a buffer with 32-bit output
 * 
 * JavaScript: const hash = nxh.nxh32(buffer, seed?);
 * 
 * @param buffer - Buffer or TypedArray to hash
 * @param seed   - Optional seed (Number, default 0)
 * @return Number - 32-bit hash value
 */
static napi_value Nxh32(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected at least 1 argument: buffer");
        return NULL;
    }
    
    /* Get buffer data */
    void *data = NULL;
    size_t length = 0;
    bool is_buffer = false;
    
    NAPI_CALL(env, napi_is_buffer(env, args[0], &is_buffer));
    
    if (is_buffer) {
        NAPI_CALL(env, napi_get_buffer_info(env, args[0], &data, &length));
    } else {
        bool is_typedarray = false;
        NAPI_CALL(env, napi_is_typedarray(env, args[0], &is_typedarray));
        
        if (is_typedarray) {
            napi_typedarray_type type;
            napi_value arraybuffer;
            size_t byte_offset;
            NAPI_CALL(env, napi_get_typedarray_info(env, args[0], &type, &length,
                                                     &data, &arraybuffer, &byte_offset));
        } else {
            napi_throw_type_error(env, NULL, "First argument must be Buffer or TypedArray");
            return NULL;
        }
    }
    
    /* Get optional seed */
    uint32_t seed = 0;
    if (argc >= 2) {
        double num;
        NAPI_CALL(env, napi_get_value_double(env, args[1], &num));
        seed = (uint32_t)num;
    }
    
    /* Compute hash */
    uint32_t hash = nxh32(data, length, seed);
    
    /* Return as Number */
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, hash, &result));
    
    return result;
}

/* ============================================================
 * NXH_STRING - Hash a string directly (64-bit)
 * ============================================================ */

/**
 * @brief Hash a string with 64-bit output
 * 
 * JavaScript: const hash = nxh.nxhString('hello world', seed?);
 * 
 * @param str  - String to hash
 * @param seed - Optional seed (BigInt, default 0)
 * @return BigInt - 64-bit hash value
 */
static napi_value NxhString(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected at least 1 argument: string");
        return NULL;
    }
    
    /* Get string */
    size_t str_len = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], NULL, 0, &str_len));
    
    char *str = malloc(str_len + 1);
    if (str == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], str, str_len + 1, &str_len));
    
    /* Get optional seed */
    uint64_t seed = NXH_SEED_DEFAULT;
    if (argc >= 2) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, args[1], &type));
        
        if (type == napi_bigint) {
            bool lossless = true;
            NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[1], &seed, &lossless));
        } else if (type == napi_number) {
            double num;
            NAPI_CALL(env, napi_get_value_double(env, args[1], &num));
            seed = (uint64_t)num;
        }
    }
    
    /* Compute hash */
    uint64_t hash = nxh64(str, str_len, seed);
    free(str);
    
    /* Return as BigInt */
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, hash, &result));
    
    return result;
}

/* ============================================================
 * NXH_STRING32 - Hash a string directly (32-bit)
 * ============================================================ */

/**
 * @brief Hash a string with 32-bit output
 * 
 * JavaScript: const hash = nxh.nxhString32('hello world', seed?);
 * 
 * @param str  - String to hash
 * @param seed - Optional seed (Number, default 0)
 * @return Number - 32-bit hash value
 */
static napi_value NxhString32(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected at least 1 argument: string");
        return NULL;
    }
    
    /* Get string */
    size_t str_len = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], NULL, 0, &str_len));
    
    char *str = malloc(str_len + 1);
    if (str == NULL) {
        napi_throw_error(env, NULL, "Memory allocation failed");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], str, str_len + 1, &str_len));
    
    /* Get optional seed */
    uint32_t seed = 0;
    if (argc >= 2) {
        double num;
        NAPI_CALL(env, napi_get_value_double(env, args[1], &num));
        seed = (uint32_t)num;
    }
    
    /* Compute hash */
    uint32_t hash = nxh32(str, str_len, seed);
    free(str);
    
    /* Return as Number */
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, hash, &result));
    
    return result;
}

/* ============================================================
 * NXH_COMBINE - Combine two hashes
 * ============================================================ */

/**
 * @brief Combine two 64-bit hashes into one
 * 
 * JavaScript: const combined = nxh.nxhCombine(hash1, hash2);
 * 
 * Useful for creating composite keys from multiple values.
 * 
 * @param h1 - First hash (BigInt)
 * @param h2 - Second hash (BigInt)
 * @return BigInt - Combined hash
 */
static napi_value NxhCombine(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: hash1, hash2");
        return NULL;
    }
    
    uint64_t h1, h2;
    bool lossless = true;
    
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &h1, &lossless));
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[1], &h2, &lossless));
    
    /* Combine using NXH's combine function */
    uint64_t combined = nxh_combine(h1, h2);
    
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, combined, &result));
    
    return result;
}

/* ============================================================
 * NXH_INT64 - Hash a 64-bit integer
 * ============================================================ */

/**
 * @brief Hash a 64-bit integer value
 * 
 * JavaScript: const hash = nxh.nxhInt64(12345678901234567890n);
 * 
 * Useful for hashing IDs, timestamps, etc.
 * 
 * @param value - BigInt to hash
 * @return BigInt - 64-bit hash
 */
static napi_value NxhInt64(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument: value (BigInt)");
        return NULL;
    }
    
    uint64_t value;
    bool lossless = true;
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
    
    uint64_t hash = nxh_int64(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, hash, &result));
    
    return result;
}

/* ============================================================
 * NXH_INT32 - Hash a 32-bit integer
 * ============================================================ */

/**
 * @brief Hash a 32-bit integer value
 * 
 * JavaScript: const hash = nxh.nxhInt32(12345);
 * 
 * @param value - Number to hash
 * @return BigInt - 64-bit hash
 */
static napi_value NxhInt32(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument: value (Number)");
        return NULL;
    }
    
    uint32_t value;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
    
    uint64_t hash = nxh_int32(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, hash, &result));
    
    return result;
}

/* ============================================================
 * NXH64_ALT - Alternate 64-bit hash (for Cuckoo hashing)
 * ============================================================ */

/**
 * @brief Alternate 64-bit hash function
 * 
 * JavaScript: const hash = nxh.nxh64Alt(buffer, seed?);
 * 
 * Produces statistically independent hash from nxh64().
 * Used by DAGGER for Cuckoo hashing fallback.
 * 
 * @param buffer - Buffer or TypedArray to hash
 * @param seed   - Optional seed (BigInt, default NXH_SEED_ALT)
 * @return BigInt - 64-bit hash value
 */
static napi_value Nxh64Alt(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected at least 1 argument: buffer");
        return NULL;
    }
    
    /* Get buffer data */
    void *data = NULL;
    size_t length = 0;
    bool is_buffer = false;
    
    NAPI_CALL(env, napi_is_buffer(env, args[0], &is_buffer));
    
    if (is_buffer) {
        NAPI_CALL(env, napi_get_buffer_info(env, args[0], &data, &length));
    } else {
        bool is_typedarray = false;
        NAPI_CALL(env, napi_is_typedarray(env, args[0], &is_typedarray));
        
        if (is_typedarray) {
            napi_typedarray_type type;
            napi_value arraybuffer;
            size_t byte_offset;
            NAPI_CALL(env, napi_get_typedarray_info(env, args[0], &type, &length,
                                                     &data, &arraybuffer, &byte_offset));
        } else {
            napi_throw_type_error(env, NULL, "First argument must be Buffer or TypedArray");
            return NULL;
        }
    }
    
    /* Get optional seed */
    uint64_t seed = NXH_SEED_ALT;
    if (argc >= 2) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, args[1], &type));
        
        if (type == napi_bigint) {
            bool lossless = true;
            NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[1], &seed, &lossless));
        } else if (type == napi_number) {
            double num;
            NAPI_CALL(env, napi_get_value_double(env, args[1], &num));
            seed = (uint64_t)num;
        }
    }
    
    /* Compute alternate hash */
    uint64_t hash = nxh64_alt(data, length, seed);
    
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, hash, &result));
    
    return result;
}

/* ============================================================
 * MODULE INITIALIZATION
 * ============================================================ */

static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        /* Core hash functions */
        DECLARE_NAPI_METHOD("nxh64", Nxh64),
        DECLARE_NAPI_METHOD("nxh32", Nxh32),
        DECLARE_NAPI_METHOD("nxh64Alt", Nxh64Alt),
        
        /* String hashing */
        DECLARE_NAPI_METHOD("nxhString", NxhString),
        DECLARE_NAPI_METHOD("nxhString32", NxhString32),
        
        /* Integer hashing */
        DECLARE_NAPI_METHOD("nxhInt64", NxhInt64),
        DECLARE_NAPI_METHOD("nxhInt32", NxhInt32),
        
        /* Utilities */
        DECLARE_NAPI_METHOD("nxhCombine", NxhCombine),
    };
    
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    
    /* Add version string */
    napi_value version;
    NAPI_CALL(env, napi_create_string_utf8(env, NXH_VERSION_STRING, NAPI_AUTO_LENGTH, &version));
    NAPI_CALL(env, napi_set_named_property(env, exports, "version", version));
    
    /* Add constants */
    napi_value seed_default, seed_alt;
    NAPI_CALL(env, napi_create_bigint_uint64(env, NXH_SEED_DEFAULT, &seed_default));
    NAPI_CALL(env, napi_create_bigint_uint64(env, NXH_SEED_ALT, &seed_alt));
    NAPI_CALL(env, napi_set_named_property(env, exports, "SEED_DEFAULT", seed_default));
    NAPI_CALL(env, napi_set_named_property(env, exports, "SEED_ALT", seed_alt));
    
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
