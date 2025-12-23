/**
 * @file zorya_pcm_napi.c
 * @brief N-API bindings for PCM (Performance Critical Macros) bit operations
 *
 * @author Anthony Taliento
 * @date 2025-12-08
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license MIT
 *
 * DESCRIPTION:
 *   Exposes PCM bit manipulation functions to Node.js/Electron via N-API.
 *   These operations compile to single CPU instructions in C but have no
 *   native equivalent in JavaScript.
 *
 * EXPORTS:
 *   Bit counting:
 *   - popcount32(n)      -> Number (count set bits)
 *   - popcount64(n)      -> Number (count set bits, 64-bit)
 *   - clz32(n)           -> Number (count leading zeros)
 *   - clz64(n)           -> Number (count leading zeros, 64-bit)
 *   - ctz32(n)           -> Number (count trailing zeros)
 *   - ctz64(n)           -> Number (count trailing zeros, 64-bit)
 *
 *   Rotation:
 *   - rotl32(x, n)       -> Number (rotate left 32-bit)
 *   - rotr32(x, n)       -> Number (rotate right 32-bit)
 *   - rotl64(x, n)       -> BigInt (rotate left 64-bit)
 *   - rotr64(x, n)       -> BigInt (rotate right 64-bit)
 *
 *   Byte swap:
 *   - bswap16(x)         -> Number (byte swap 16-bit)
 *   - bswap32(x)         -> Number (byte swap 32-bit)
 *   - bswap64(x)         -> BigInt (byte swap 64-bit)
 *
 *   Power of 2:
 *   - isPowerOf2(n)      -> Boolean
 *   - nextPowerOf2_32(n) -> Number
 *   - nextPowerOf2_64(n) -> BigInt
 *
 *   Alignment:
 *   - alignUp(x, align)   -> Number/BigInt
 *   - alignDown(x, align) -> Number/BigInt
 *   - isAligned(x, align) -> Boolean
 *
 * USAGE:
 *   const pcm = require('zorya-pcm');
 *   
 *   // Count bits in a bloom filter
 *   const bits = pcm.popcount64(bitmask);
 *   
 *   // Fast log2
 *   const log2 = 31 - pcm.clz32(n);
 *   
 *   // Binary protocol byte swap
 *   const networkOrder = pcm.bswap32(hostOrder);
 *
 * THREAD SAFETY:
 *   All functions are thread-safe (pure functions, no state).
 */

#include <node_api.h>
#include <stdint.h>
#include <stdbool.h>

/* Include PCM header for macros */
#include "pcm.h"

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
 * BIT COUNTING FUNCTIONS
 * ============================================================ */

/**
 * @brief Count set bits (population count) - 32-bit
 * 
 * JavaScript: const bits = pcm.popcount32(0xDEADBEEF);
 */
static napi_value Popcount32(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument");
        return NULL;
    }
    
    uint32_t value;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
    
    uint32_t count = ZORYA_POPCOUNT32(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, count, &result));
    return result;
}

/**
 * @brief Count set bits (population count) - 64-bit
 * 
 * JavaScript: const bits = pcm.popcount64(0xDEADBEEFCAFEBABEn);
 */
static napi_value Popcount64(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument (BigInt)");
        return NULL;
    }
    
    uint64_t value;
    bool lossless = true;
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
    
    uint32_t count = ZORYA_POPCOUNT64(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, count, &result));
    return result;
}

/**
 * @brief Count leading zeros - 32-bit
 * 
 * JavaScript: const zeros = pcm.clz32(0x00FF0000);  // 8
 */
static napi_value Clz32(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument");
        return NULL;
    }
    
    uint32_t value;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
    
    uint32_t count = ZORYA_CLZ32(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, count, &result));
    return result;
}

/**
 * @brief Count leading zeros - 64-bit
 * 
 * JavaScript: const zeros = pcm.clz64(0x00FFn);  // 56
 */
static napi_value Clz64(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument (BigInt)");
        return NULL;
    }
    
    uint64_t value;
    bool lossless = true;
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
    
    uint32_t count = ZORYA_CLZ64(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, count, &result));
    return result;
}

/**
 * @brief Count trailing zeros - 32-bit
 * 
 * JavaScript: const zeros = pcm.ctz32(0x00FF0000);  // 16
 */
static napi_value Ctz32(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument");
        return NULL;
    }
    
    uint32_t value;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
    
    uint32_t count = ZORYA_CTZ32(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, count, &result));
    return result;
}

/**
 * @brief Count trailing zeros - 64-bit
 * 
 * JavaScript: const zeros = pcm.ctz64(0xFF00000000000000n);  // 56
 */
static napi_value Ctz64(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument (BigInt)");
        return NULL;
    }
    
    uint64_t value;
    bool lossless = true;
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
    
    uint32_t count = ZORYA_CTZ64(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, count, &result));
    return result;
}

/* ============================================================
 * ROTATION FUNCTIONS
 * ============================================================ */

/**
 * @brief Rotate left - 32-bit
 * 
 * JavaScript: const rotated = pcm.rotl32(0xDEADBEEF, 13);
 */
static napi_value Rotl32(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: value, rotation");
        return NULL;
    }
    
    uint32_t value, rotation;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
    NAPI_CALL(env, napi_get_value_uint32(env, args[1], &rotation));
    
    uint32_t result_val = ZORYA_ROTL32(value, rotation);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, result_val, &result));
    return result;
}

/**
 * @brief Rotate right - 32-bit
 * 
 * JavaScript: const rotated = pcm.rotr32(0xDEADBEEF, 13);
 */
static napi_value Rotr32(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: value, rotation");
        return NULL;
    }
    
    uint32_t value, rotation;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
    NAPI_CALL(env, napi_get_value_uint32(env, args[1], &rotation));
    
    uint32_t result_val = ZORYA_ROTR32(value, rotation);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, result_val, &result));
    return result;
}

/**
 * @brief Rotate left - 64-bit
 * 
 * JavaScript: const rotated = pcm.rotl64(0xDEADBEEFCAFEBABEn, 13);
 */
static napi_value Rotl64(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: value (BigInt), rotation");
        return NULL;
    }
    
    uint64_t value;
    uint32_t rotation;
    bool lossless = true;
    
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
    NAPI_CALL(env, napi_get_value_uint32(env, args[1], &rotation));
    
    uint64_t result_val = ZORYA_ROTL64(value, rotation);
    
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, result_val, &result));
    return result;
}

/**
 * @brief Rotate right - 64-bit
 * 
 * JavaScript: const rotated = pcm.rotr64(0xDEADBEEFCAFEBABEn, 13);
 */
static napi_value Rotr64(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: value (BigInt), rotation");
        return NULL;
    }
    
    uint64_t value;
    uint32_t rotation;
    bool lossless = true;
    
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
    NAPI_CALL(env, napi_get_value_uint32(env, args[1], &rotation));
    
    uint64_t result_val = ZORYA_ROTR64(value, rotation);
    
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, result_val, &result));
    return result;
}

/* ============================================================
 * BYTE SWAP FUNCTIONS
 * ============================================================ */

/**
 * @brief Byte swap - 16-bit
 * 
 * JavaScript: const swapped = pcm.bswap16(0x1234);  // 0x3412
 */
static napi_value Bswap16(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument");
        return NULL;
    }
    
    uint32_t value;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
    
    uint16_t result_val = ZORYA_BSWAP16((uint16_t)value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, result_val, &result));
    return result;
}

/**
 * @brief Byte swap - 32-bit
 * 
 * JavaScript: const swapped = pcm.bswap32(0x12345678);  // 0x78563412
 */
static napi_value Bswap32(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument");
        return NULL;
    }
    
    uint32_t value;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
    
    uint32_t result_val = ZORYA_BSWAP32(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, result_val, &result));
    return result;
}

/**
 * @brief Byte swap - 64-bit
 * 
 * JavaScript: const swapped = pcm.bswap64(0x123456789ABCDEFn);
 */
static napi_value Bswap64(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument (BigInt)");
        return NULL;
    }
    
    uint64_t value;
    bool lossless = true;
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
    
    uint64_t result_val = ZORYA_BSWAP64(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, result_val, &result));
    return result;
}

/* ============================================================
 * POWER OF 2 FUNCTIONS
 * ============================================================ */

/**
 * @brief Check if value is power of 2
 * 
 * JavaScript: const isPow2 = pcm.isPowerOf2(1024);  // true
 */
static napi_value IsPowerOf2(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument");
        return NULL;
    }
    
    /* Handle both Number and BigInt */
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, args[0], &type));
    
    bool is_pow2 = false;
    
    if (type == napi_bigint) {
        uint64_t value;
        bool lossless = true;
        NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
        is_pow2 = ZORYA_IS_POWER_OF_2(value);
    } else {
        uint32_t value;
        NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
        is_pow2 = ZORYA_IS_POWER_OF_2(value);
    }
    
    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, is_pow2, &result));
    return result;
}

/**
 * @brief Next power of 2 - 32-bit
 * 
 * JavaScript: const next = pcm.nextPowerOf2_32(1000);  // 1024
 */
static napi_value NextPowerOf2_32(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument");
        return NULL;
    }
    
    uint32_t value;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
    
    uint32_t result_val = ZORYA_NEXT_POWER_OF_2_32(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, result_val, &result));
    return result;
}

/**
 * @brief Next power of 2 - 64-bit
 * 
 * JavaScript: const next = pcm.nextPowerOf2_64(1000n);  // 1024n
 */
static napi_value NextPowerOf2_64(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument (BigInt)");
        return NULL;
    }
    
    uint64_t value;
    bool lossless = true;
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
    
    uint64_t result_val = ZORYA_NEXT_POWER_OF_2_64(value);
    
    napi_value result;
    NAPI_CALL(env, napi_create_bigint_uint64(env, result_val, &result));
    return result;
}

/* ============================================================
 * ALIGNMENT FUNCTIONS
 * ============================================================ */

/**
 * @brief Align up to boundary
 * 
 * JavaScript: const aligned = pcm.alignUp(1000, 16);  // 1008
 */
static napi_value AlignUp(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: value, alignment");
        return NULL;
    }
    
    /* Handle both Number and BigInt */
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, args[0], &type));
    
    if (type == napi_bigint) {
        uint64_t value, align;
        bool lossless = true;
        NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
        NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[1], &align, &lossless));
        
        uint64_t result_val = ZORYA_ALIGN_UP(value, align);
        
        napi_value result;
        NAPI_CALL(env, napi_create_bigint_uint64(env, result_val, &result));
        return result;
    } else {
        uint32_t value, align;
        NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
        NAPI_CALL(env, napi_get_value_uint32(env, args[1], &align));
        
        uint32_t result_val = (uint32_t)ZORYA_ALIGN_UP(value, align);
        
        napi_value result;
        NAPI_CALL(env, napi_create_uint32(env, result_val, &result));
        return result;
    }
}

/**
 * @brief Align down to boundary
 * 
 * JavaScript: const aligned = pcm.alignDown(1000, 16);  // 992
 */
static napi_value AlignDown(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: value, alignment");
        return NULL;
    }
    
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, args[0], &type));
    
    if (type == napi_bigint) {
        uint64_t value, align;
        bool lossless = true;
        NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
        NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[1], &align, &lossless));
        
        uint64_t result_val = ZORYA_ALIGN_DOWN(value, align);
        
        napi_value result;
        NAPI_CALL(env, napi_create_bigint_uint64(env, result_val, &result));
        return result;
    } else {
        uint32_t value, align;
        NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
        NAPI_CALL(env, napi_get_value_uint32(env, args[1], &align));
        
        uint32_t result_val = (uint32_t)ZORYA_ALIGN_DOWN(value, align);
        
        napi_value result;
        NAPI_CALL(env, napi_create_uint32(env, result_val, &result));
        return result;
    }
}

/**
 * @brief Check if value is aligned
 * 
 * JavaScript: const aligned = pcm.isAligned(1024, 16);  // true
 */
static napi_value IsAligned(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 2) {
        napi_throw_type_error(env, NULL, "Expected 2 arguments: value, alignment");
        return NULL;
    }
    
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, args[0], &type));
    
    bool is_aligned = false;
    
    if (type == napi_bigint) {
        uint64_t value, align;
        bool lossless = true;
        NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
        NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[1], &align, &lossless));
        is_aligned = (value & (align - 1)) == 0;
    } else {
        uint32_t value, align;
        NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
        NAPI_CALL(env, napi_get_value_uint32(env, args[1], &align));
        is_aligned = (value & (align - 1)) == 0;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_get_boolean(env, is_aligned, &result));
    return result;
}

/* ============================================================
 * UTILITY FUNCTIONS
 * ============================================================ */

/**
 * @brief Fast log2 (floor) - 32-bit
 * 
 * JavaScript: const log = pcm.log2_32(1000);  // 9
 */
static napi_value Log2_32(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument");
        return NULL;
    }
    
    uint32_t value;
    NAPI_CALL(env, napi_get_value_uint32(env, args[0], &value));
    
    /* log2(x) = 31 - clz(x) for x > 0 */
    uint32_t result_val = value ? (31 - ZORYA_CLZ32(value)) : 0;
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, result_val, &result));
    return result;
}

/**
 * @brief Fast log2 (floor) - 64-bit
 * 
 * JavaScript: const log = pcm.log2_64(1000n);  // 9
 */
static napi_value Log2_64(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));
    
    if (argc < 1) {
        napi_throw_type_error(env, NULL, "Expected 1 argument (BigInt)");
        return NULL;
    }
    
    uint64_t value;
    bool lossless = true;
    NAPI_CALL(env, napi_get_value_bigint_uint64(env, args[0], &value, &lossless));
    
    /* log2(x) = 63 - clz(x) for x > 0 */
    uint32_t result_val = value ? (63 - ZORYA_CLZ64(value)) : 0;
    
    napi_value result;
    NAPI_CALL(env, napi_create_uint32(env, result_val, &result));
    return result;
}

/* ============================================================
 * MODULE INITIALIZATION
 * ============================================================ */

static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        /* Bit counting */
        DECLARE_NAPI_METHOD("popcount32", Popcount32),
        DECLARE_NAPI_METHOD("popcount64", Popcount64),
        DECLARE_NAPI_METHOD("clz32", Clz32),
        DECLARE_NAPI_METHOD("clz64", Clz64),
        DECLARE_NAPI_METHOD("ctz32", Ctz32),
        DECLARE_NAPI_METHOD("ctz64", Ctz64),
        
        /* Rotation */
        DECLARE_NAPI_METHOD("rotl32", Rotl32),
        DECLARE_NAPI_METHOD("rotr32", Rotr32),
        DECLARE_NAPI_METHOD("rotl64", Rotl64),
        DECLARE_NAPI_METHOD("rotr64", Rotr64),
        
        /* Byte swap */
        DECLARE_NAPI_METHOD("bswap16", Bswap16),
        DECLARE_NAPI_METHOD("bswap32", Bswap32),
        DECLARE_NAPI_METHOD("bswap64", Bswap64),
        
        /* Power of 2 */
        DECLARE_NAPI_METHOD("isPowerOf2", IsPowerOf2),
        DECLARE_NAPI_METHOD("nextPowerOf2_32", NextPowerOf2_32),
        DECLARE_NAPI_METHOD("nextPowerOf2_64", NextPowerOf2_64),
        
        /* Alignment */
        DECLARE_NAPI_METHOD("alignUp", AlignUp),
        DECLARE_NAPI_METHOD("alignDown", AlignDown),
        DECLARE_NAPI_METHOD("isAligned", IsAligned),
        
        /* Utilities */
        DECLARE_NAPI_METHOD("log2_32", Log2_32),
        DECLARE_NAPI_METHOD("log2_64", Log2_64),
    };
    
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    
    /* Add version */
    napi_value version;
    NAPI_CALL(env, napi_create_string_utf8(env, "1.0.0", NAPI_AUTO_LENGTH, &version));
    NAPI_CALL(env, napi_set_named_property(env, exports, "version", version));
    
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
