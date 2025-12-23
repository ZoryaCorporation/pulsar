/**
 * @file zorya_ini.h
 * @brief ZORYA-INI Parser - Human-First, AI-Native Configuration Format
 *
 * @author Anthony Taliento
 * @date 2025-12-07
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * DESCRIPTION:
 *   ZORYA-INI is a modern take on the classic INI format with:
 *   - Hierarchical sections via dot notation
 *   - Pipe-delimited arrays
 *   - Indented multiline strings
 *   - Variable interpolation
 *   - Optional type hints
 *   - Binary compilation to DAGGER hash tables
 *
 * PHILOSOPHY:
 *   What's readable to humans is readable to AI.
 *   One concept per line. Simplicity is power.
 *
 * EXAMPLE:
 *   ```ini
 *   ::include defaults.ini
 *
 *   [app]
 *   name = ZORYA Server
 *   port:int = 8080
 *
 *   [app.features]
 *   enabled = auth | logging | metrics
 *   description = 
 *       A powerful server
 *       with many features
 *   ```
 *
 * USAGE:
 *   ```c
 *   ZoryaIni *ini = zorya_ini_new();
 *   zorya_ini_load(ini, "config.ini");
 *   
 *   const char *name = zorya_ini_get(ini, "app.name");
 *   int port = zorya_ini_get_int(ini, "app.port");
 *   
 *   zorya_ini_free(ini);
 *   ```
 */

#ifndef ZORYA_INI_H_
#define ZORYA_INI_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * VERSION
 * ============================================================ */

#define ZORYA_INI_VERSION_MAJOR 1
#define ZORYA_INI_VERSION_MINOR 0
#define ZORYA_INI_VERSION_PATCH 0
#define ZORYA_INI_VERSION_STRING "1.0.0"

/* ============================================================
 * ERROR CODES
 * ============================================================ */

/**
 * @brief ZORYA-INI error codes
 */
typedef enum {
    ZORYA_INI_OK = 0,              /**< Success */
    ZORYA_INI_ERROR_NULLPTR = -1,  /**< NULL pointer argument */
    ZORYA_INI_ERROR_NOMEM = -2,    /**< Memory allocation failed */
    ZORYA_INI_ERROR_IO = -3,       /**< File I/O error */
    ZORYA_INI_ERROR_SYNTAX = -4,   /**< Syntax error in INI file */
    ZORYA_INI_ERROR_NOT_FOUND = -5,/**< Key not found */
    ZORYA_INI_ERROR_TYPE = -6,     /**< Type conversion error */
    ZORYA_INI_ERROR_CIRCULAR = -7, /**< Circular reference detected */
    ZORYA_INI_ERROR_INCLUDE = -8,  /**< Include file error */
    ZORYA_INI_ERROR_INTERP = -9,   /**< Interpolation error */
} zorya_ini_error_t;

/* ============================================================
 * VALUE TYPES
 * ============================================================ */

/**
 * @brief Value type enumeration
 */
typedef enum {
    ZORYA_INI_TYPE_STRING = 0,     /**< String (default) */
    ZORYA_INI_TYPE_INT,            /**< Integer */
    ZORYA_INI_TYPE_FLOAT,          /**< Floating point */
    ZORYA_INI_TYPE_BOOL,           /**< Boolean */
    ZORYA_INI_TYPE_PATH,           /**< File path */
    ZORYA_INI_TYPE_URL,            /**< URL */
    ZORYA_INI_TYPE_DATE,           /**< ISO date */
    ZORYA_INI_TYPE_DATETIME,       /**< ISO datetime */
    ZORYA_INI_TYPE_ARRAY,          /**< Array of values */
} zorya_ini_type_t;

/* ============================================================
 * DATA STRUCTURES
 * ============================================================ */

/**
 * @brief Forward declaration of INI context
 */
typedef struct ZoryaIni ZoryaIni;

/**
 * @brief INI value structure
 */
typedef struct {
    zorya_ini_type_t type;         /**< Value type */
    union {
        char *str;                 /**< String value */
        int64_t i;                 /**< Integer value */
        double f;                  /**< Float value */
        bool b;                    /**< Boolean value */
        struct {
            char **items;          /**< Array items */
            size_t count;          /**< Array length */
        } arr;
    } v;
    bool is_array;                 /**< True if array type */
} ZoryaIniValue;

/**
 * @brief Parse error information
 */
typedef struct {
    int line;                      /**< Line number (1-based) */
    int column;                    /**< Column number (1-based) */
    const char *message;           /**< Error message */
    const char *file;              /**< File path (if from include) */
} ZoryaIniParseError;

/**
 * @brief INI statistics
 */
typedef struct {
    size_t section_count;          /**< Number of sections */
    size_t key_count;              /**< Number of keys */
    size_t include_count;          /**< Number of included files */
    size_t memory_bytes;           /**< Approximate memory usage */
    double load_factor;            /**< Hash table load factor */
} ZoryaIniStats;

/* ============================================================
 * LIFECYCLE
 * ============================================================ */

/**
 * @brief Create a new INI context
 *
 * @return New context or NULL on failure
 *
 * @example
 *   ZoryaIni *ini = zorya_ini_new();
 *   if (ini == NULL) {
 *       fprintf(stderr, "Failed to create INI context\n");
 *   }
 */
ZoryaIni* zorya_ini_new(void);

/**
 * @brief Free INI context and all associated memory
 *
 * @param ini Context to free (NULL safe)
 */
void zorya_ini_free(ZoryaIni *ini);

/* ============================================================
 * LOADING
 * ============================================================ */

/**
 * @brief Load INI from file
 *
 * Parses the file and resolves all includes and interpolations.
 *
 * @param ini      INI context
 * @param filepath Path to INI file
 * @return ZORYA_INI_OK on success, error code on failure
 *
 * @example
 *   zorya_ini_error_t err = zorya_ini_load(ini, "config.ini");
 *   if (err != ZORYA_INI_OK) {
 *       fprintf(stderr, "Load failed: %s\n", zorya_ini_strerror(err));
 *   }
 */
zorya_ini_error_t zorya_ini_load(ZoryaIni *ini, const char *filepath);

/**
 * @brief Load INI from memory buffer
 *
 * @param ini    INI context
 * @param data   Buffer containing INI text
 * @param len    Buffer length
 * @param name   Optional name for error messages
 * @return ZORYA_INI_OK on success, error code on failure
 */
zorya_ini_error_t zorya_ini_load_buffer(
    ZoryaIni *ini,
    const char *data,
    size_t len,
    const char *name
);

/**
 * @brief Load compiled binary INI
 *
 * @param ini      INI context
 * @param filepath Path to compiled .dd file
 * @return ZORYA_INI_OK on success, error code on failure
 */
zorya_ini_error_t zorya_ini_load_binary(ZoryaIni *ini, const char *filepath);

/* ============================================================
 * GETTERS (String)
 * ============================================================ */

/**
 * @brief Get string value by key
 *
 * Key format: "section.key" or "section.subsection.key"
 * For root-level keys, use "key" without section prefix.
 *
 * @param ini INI context
 * @param key Full key path
 * @return String value or NULL if not found
 *
 * @example
 *   const char *host = zorya_ini_get(ini, "database.host");
 *   const char *name = zorya_ini_get(ini, "app.name");
 */
const char* zorya_ini_get(const ZoryaIni *ini, const char *key);

/**
 * @brief Get string value with default
 *
 * @param ini INI context
 * @param key Full key path
 * @param def Default value if key not found
 * @return String value or default
 */
const char* zorya_ini_get_default(
    const ZoryaIni *ini,
    const char *key,
    const char *def
);

/* ============================================================
 * GETTERS (Typed)
 * ============================================================ */

/**
 * @brief Get integer value
 *
 * @param ini INI context
 * @param key Full key path
 * @return Integer value or 0 if not found/invalid
 */
int64_t zorya_ini_get_int(const ZoryaIni *ini, const char *key);

/**
 * @brief Get integer with default
 *
 * @param ini INI context
 * @param key Full key path
 * @param def Default value
 * @return Integer value or default
 */
int64_t zorya_ini_get_int_default(
    const ZoryaIni *ini,
    const char *key,
    int64_t def
);

/**
 * @brief Get float value
 *
 * @param ini INI context
 * @param key Full key path
 * @return Float value or 0.0 if not found/invalid
 */
double zorya_ini_get_float(const ZoryaIni *ini, const char *key);

/**
 * @brief Get float with default
 */
double zorya_ini_get_float_default(
    const ZoryaIni *ini,
    const char *key,
    double def
);

/**
 * @brief Get boolean value
 *
 * Recognizes: true/false, yes/no, on/off, 1/0
 *
 * @param ini INI context
 * @param key Full key path
 * @return Boolean value or false if not found/invalid
 */
bool zorya_ini_get_bool(const ZoryaIni *ini, const char *key);

/**
 * @brief Get boolean with default
 */
bool zorya_ini_get_bool_default(
    const ZoryaIni *ini,
    const char *key,
    bool def
);

/* ============================================================
 * GETTERS (Arrays)
 * ============================================================ */

/**
 * @brief Get array value
 *
 * Returns the parsed array. Do not free the returned pointer.
 *
 * @param ini   INI context
 * @param key   Full key path
 * @param count Output: number of elements
 * @return Array of strings or NULL if not found
 *
 * @example
 *   size_t count;
 *   const char **roles = zorya_ini_get_array(ini, "user.roles", &count);
 *   for (size_t i = 0; i < count; i++) {
 *       printf("Role: %s\n", roles[i]);
 *   }
 */
const char** zorya_ini_get_array(
    const ZoryaIni *ini,
    const char *key,
    size_t *count
);

/**
 * @brief Get array length
 *
 * @param ini INI context
 * @param key Full key path
 * @return Array length or 0 if not found
 */
size_t zorya_ini_get_array_len(const ZoryaIni *ini, const char *key);

/* ============================================================
 * EXISTENCE CHECKS
 * ============================================================ */

/**
 * @brief Check if key exists
 *
 * @param ini INI context
 * @param key Full key path
 * @return true if key exists
 */
bool zorya_ini_has(const ZoryaIni *ini, const char *key);

/**
 * @brief Check if section exists
 *
 * @param ini     INI context
 * @param section Section name
 * @return true if section exists
 */
bool zorya_ini_has_section(const ZoryaIni *ini, const char *section);

/* ============================================================
 * SETTERS
 * ============================================================ */

/**
 * @brief Set string value
 *
 * @param ini   INI context
 * @param key   Full key path (section will be created if needed)
 * @param value String value
 * @return ZORYA_INI_OK on success
 */
zorya_ini_error_t zorya_ini_set(
    ZoryaIni *ini,
    const char *key,
    const char *value
);

/**
 * @brief Set integer value
 */
zorya_ini_error_t zorya_ini_set_int(
    ZoryaIni *ini,
    const char *key,
    int64_t value
);

/**
 * @brief Set float value
 */
zorya_ini_error_t zorya_ini_set_float(
    ZoryaIni *ini,
    const char *key,
    double value
);

/**
 * @brief Set boolean value
 */
zorya_ini_error_t zorya_ini_set_bool(
    ZoryaIni *ini,
    const char *key,
    bool value
);

/**
 * @brief Set array value
 *
 * @param ini    INI context
 * @param key    Full key path
 * @param values Array of strings
 * @param count  Number of elements
 * @return ZORYA_INI_OK on success
 */
zorya_ini_error_t zorya_ini_set_array(
    ZoryaIni *ini,
    const char *key,
    const char **values,
    size_t count
);

/* ============================================================
 * SERIALIZATION
 * ============================================================ */

/**
 * @brief Write INI to file
 *
 * @param ini      INI context
 * @param filepath Output file path
 * @return ZORYA_INI_OK on success
 */
zorya_ini_error_t zorya_ini_save(const ZoryaIni *ini, const char *filepath);

/**
 * @brief Write INI to string buffer
 *
 * Caller must free the returned buffer.
 *
 * @param ini INI context
 * @param len Output: buffer length
 * @return Allocated buffer or NULL on failure
 */
char* zorya_ini_to_string(const ZoryaIni *ini, size_t *len);

/**
 * @brief Compile to binary format
 *
 * Creates a DAGGER-backed binary file for O(1) lookups.
 *
 * @param ini      INI context
 * @param filepath Output .dd file path
 * @return ZORYA_INI_OK on success
 */
zorya_ini_error_t zorya_ini_compile(const ZoryaIni *ini, const char *filepath);

/* ============================================================
 * ITERATION
 * ============================================================ */

/**
 * @brief Iteration callback type
 *
 * @param section  Section name (NULL for root)
 * @param key      Key name
 * @param value    Value as string
 * @param userdata User context
 * @return 0 to continue, non-zero to stop
 */
typedef int (*zorya_ini_callback_t)(
    const char *section,
    const char *key,
    const char *value,
    void *userdata
);

/**
 * @brief Iterate over all key-value pairs
 *
 * @param ini      INI context
 * @param callback Function to call for each pair
 * @param userdata User context passed to callback
 * @return Number of iterations or -1 on error
 */
int zorya_ini_foreach(
    const ZoryaIni *ini,
    zorya_ini_callback_t callback,
    void *userdata
);

/**
 * @brief Iterate over keys in a section
 *
 * @param ini      INI context
 * @param section  Section name
 * @param callback Function to call for each pair
 * @param userdata User context
 * @return Number of iterations or -1 on error
 */
int zorya_ini_foreach_section(
    const ZoryaIni *ini,
    const char *section,
    zorya_ini_callback_t callback,
    void *userdata
);

/**
 * @brief Get list of section names
 *
 * Caller must free the returned array (but not the strings).
 *
 * @param ini   INI context
 * @param count Output: number of sections
 * @return Array of section names or NULL
 */
const char** zorya_ini_sections(const ZoryaIni *ini, size_t *count);

/* ============================================================
 * ERROR HANDLING
 * ============================================================ */

/**
 * @brief Get error message for error code
 *
 * @param err Error code
 * @return Static error message string
 */
const char* zorya_ini_strerror(zorya_ini_error_t err);

/**
 * @brief Get last parse error details
 *
 * @param ini INI context
 * @return Parse error info or NULL if no error
 */
const ZoryaIniParseError* zorya_ini_last_error(const ZoryaIni *ini);

/* ============================================================
 * DIAGNOSTICS
 * ============================================================ */

/**
 * @brief Get INI statistics
 *
 * @param ini   INI context
 * @param stats Output statistics
 */
void zorya_ini_stats(const ZoryaIni *ini, ZoryaIniStats *stats);

/**
 * @brief Print INI summary to stdout
 *
 * @param ini INI context
 */
void zorya_ini_print_stats(const ZoryaIni *ini);

/**
 * @brief Dump INI contents to stdout (for debugging)
 *
 * @param ini INI context
 */
void zorya_ini_dump(const ZoryaIni *ini);

#ifdef __cplusplus
}
#endif

#endif /* ZORYA_INI_H_ */
