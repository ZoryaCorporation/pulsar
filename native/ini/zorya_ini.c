/**
 * @file zorya_ini.c
 * @brief ZORYA-INI Parser Implementation
 *
 * @author Anthony Taliento
 * @date 2025-12-07
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * DESCRIPTION:
 *   Implementation of the ZORYA-INI parser with DAGGER-backed
 *   hash table for O(1) key lookup.
 *
 * DEPENDENCIES:
 *   - dagger.h (hash table)
 *   - nxh.h (hash function)
 */

#define _GNU_SOURCE  /* For strcasecmp */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* For strcasecmp on some systems */
#include <ctype.h>
#include <errno.h>
#include "zorya_ini.h"
#include "dagger.h"
#include "weave.h"  /* Tablet for string interning */

/* ============================================================
 * INTERNAL CONSTANTS
 * ============================================================ */

#define INI_MAX_LINE_LENGTH     4096
#define INI_MAX_KEY_LENGTH      256
#define INI_MAX_SECTION_LENGTH  256
#define INI_MAX_INCLUDE_DEPTH   16
#define INI_INITIAL_CAPACITY    64

/* ============================================================
 * INTERNAL STRUCTURES
 * ============================================================ */

/**
 * @brief Internal INI entry (memory-optimized)
 * 
 * Uses Tablet for string interning - identical strings share storage.
 * This reduces memory from ~140 bytes/key to ~50 bytes/key.
 * 
 * Note: resolved_value is only allocated when interpolation changes
 * the value. Otherwise it's NULL and we use raw_value.
 */
typedef struct {
    const Weave *section;       /**< Section name (interned, NOT owned) */
    const Weave *key;           /**< Key name (interned, NOT owned) */
    const Weave *raw_value;     /**< Raw string value (interned, NOT owned) */
    char *resolved_value;       /**< Resolved value after interpolation (OWNED, may be NULL) */
    ZoryaIniValue parsed;       /**< Parsed value */
    zorya_ini_type_t hint;      /**< Type hint from key:type */
    int line;                   /**< Source line number */
} IniEntry;

/**
 * @brief Internal INI context
 */
struct ZoryaIni {
    DaggerTable *entries;       /**< Key -> IniEntry* hash table */
    Tablet *strings;            /**< String interning pool (MEMORY OPTIMIZATION) */
    char **sections;            /**< Section names array (owned, for API compat) */
    size_t section_count;       /**< Number of sections */
    size_t section_capacity;    /**< Allocated section slots */
    
    /* Include tracking */
    char *base_path;            /**< Base directory for includes */
    int include_depth;          /**< Current include depth */
    char **include_stack;       /**< Include file stack */
    
    /* Error tracking */
    ZoryaIniParseError last_error;
    char *error_message;        /**< Owned error message */
    char *error_file;           /**< Owned error file path */
    
    /* Statistics */
    size_t key_count;
    size_t include_count;
};

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

static void entry_destroy(void *value);
static void key_destroy(const void *key, uint32_t key_len);
static zorya_ini_error_t parse_buffer(ZoryaIni *ini, const char *data, 
                                       size_t len, const char *filepath);
static char* make_full_key(const char *section, const char *key);
static zorya_ini_type_t parse_type_hint(const char *hint);
static bool parse_bool_value(const char *str);
static char** parse_array(const char *value, size_t *count);
static zorya_ini_error_t resolve_all_interpolations(ZoryaIni *ini);
static char* resolve_string(ZoryaIni *ini, const char *str, 
                            const char *current_section, int depth);

/* ============================================================
 * HELPER FUNCTIONS
 * ============================================================ */

/**
 * @brief Skip whitespace
 */
static const char* skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

/**
 * @brief Trim trailing whitespace in place
 */
static void trim_trailing(char *str) {
    if (str == NULL || *str == '\0') return;
    size_t len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\t' ||
                       str[len-1] == '\r' || str[len-1] == '\n')) {
        str[--len] = '\0';
    }
}

/**
 * @brief Duplicate string
 */
static char* str_dup(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (copy != NULL) {
        memcpy(copy, s, len + 1);
    }
    return copy;
}

/**
 * @brief Get directory from path
 */
static char* get_directory(const char *path) {
    if (path == NULL) return str_dup(".");
    
    const char *last_slash = strrchr(path, '/');
    if (last_slash == NULL) {
        return str_dup(".");
    }
    
    size_t len = (size_t)(last_slash - path);
    if (len == 0) return str_dup("/");
    
    char *dir = malloc(len + 1);
    if (dir != NULL) {
        memcpy(dir, path, len);
        dir[len] = '\0';
    }
    return dir;
}

/**
 * @brief Join path components
 */
static char* join_path(const char *base, const char *file) {
    if (file[0] == '/') {
        return str_dup(file);
    }
    
    size_t base_len = strlen(base);
    size_t file_len = strlen(file);
    
    char *path = malloc(base_len + file_len + 2);
    if (path == NULL) return NULL;
    
    memcpy(path, base, base_len);
    path[base_len] = '/';
    memcpy(path + base_len + 1, file, file_len + 1);
    
    return path;
}

/* ============================================================
 * INTERPOLATION ENGINE
 * ============================================================ */

#define INTERP_MAX_DEPTH 32   /* Maximum recursion for nested ${...} */
#define INTERP_MAX_OUTPUT 65536  /* Maximum interpolated string length */

/**
 * @brief Find variable value by name
 * 
 * Resolution order per ZORYA-INI spec:
 * 1. ${var} - Look in current section, then [default]/[project] section
 * 2. ${@section:key} - Explicit cross-section reference
 * 3. ${env:VAR} - Environment variable
 * 4. ${var:-default} - Fallback if not found
 *
 * @param ini             INI context
 * @param var_name        Variable name (without ${})
 * @param current_section Current section for relative lookups
 * @param depth           Recursion depth for circular detection
 * @return Resolved value (must be freed) or NULL
 */
static char* find_variable(
    ZoryaIni *ini,
    const char *var_name,
    const char *current_section,
    int depth
) {
    if (var_name == NULL || *var_name == '\0') {
        return NULL;
    }
    
    /* Check for default value syntax: ${var:-default} */
    const char *default_sep = strstr(var_name, ":-");
    char *base_var = NULL;
    const char *default_val = NULL;
    
    if (default_sep != NULL) {
        size_t base_len = (size_t)(default_sep - var_name);
        base_var = malloc(base_len + 1);
        if (base_var == NULL) return NULL;
        memcpy(base_var, var_name, base_len);
        base_var[base_len] = '\0';
        default_val = default_sep + 2;
        var_name = base_var;
    }
    
    char *result = NULL;
    
    /* Check for environment variable: ${env:VAR} */
    if (strncmp(var_name, "env:", 4) == 0) {
        const char *env_name = var_name + 4;
        const char *env_val = getenv(env_name);
        if (env_val != NULL) {
            result = str_dup(env_val);
        }
        goto done;
    }
    
    /* Check for explicit cross-section: ${@section:key} */
    if (var_name[0] == '@') {
        const char *colon = strchr(var_name + 1, ':');
        if (colon != NULL) {
            /* Extract section and key */
            size_t sec_len = (size_t)(colon - var_name - 1);
            char *section = malloc(sec_len + 1);
            if (section == NULL) goto done;
            memcpy(section, var_name + 1, sec_len);
            section[sec_len] = '\0';
            
            const char *key = colon + 1;
            char *full_key = make_full_key(section, key);
            free(section);
            
            if (full_key != NULL) {
                const char *val = zorya_ini_get(ini, full_key);
                if (val != NULL) {
                    /* Recursively resolve nested interpolations */
                    result = resolve_string(ini, val, current_section, depth + 1);
                }
                free(full_key);
            }
        }
        goto done;
    }
    
    /* Standard variable lookup */
    /* Try 1: current_section.var */
    if (current_section != NULL && *current_section != '\0') {
        char *full_key = make_full_key(current_section, var_name);
        if (full_key != NULL) {
            const char *val = zorya_ini_get(ini, full_key);
            free(full_key);
            if (val != NULL) {
                result = resolve_string(ini, val, current_section, depth + 1);
                goto done;
            }
        }
    }
    
    /* Try 2: Root level (no section prefix) */
    const char *val = zorya_ini_get(ini, var_name);
    if (val != NULL) {
        result = resolve_string(ini, val, current_section, depth + 1);
        goto done;
    }
    
    /* Try 3: [default] section */
    char *default_key = make_full_key("default", var_name);
    if (default_key != NULL) {
        val = zorya_ini_get(ini, default_key);
        free(default_key);
        if (val != NULL) {
            result = resolve_string(ini, val, "default", depth + 1);
            goto done;
        }
    }
    
    /* Try 4: [project] section */
    char *project_key = make_full_key("project", var_name);
    if (project_key != NULL) {
        val = zorya_ini_get(ini, project_key);
        free(project_key);
        if (val != NULL) {
            result = resolve_string(ini, val, "project", depth + 1);
            goto done;
        }
    }
    
    /* Try 5: [env] section (for Ordinal compatibility) */
    char *env_section_key = make_full_key("env", var_name);
    if (env_section_key != NULL) {
        val = zorya_ini_get(ini, env_section_key);
        free(env_section_key);
        if (val != NULL) {
            result = resolve_string(ini, val, "env", depth + 1);
            goto done;
        }
    }

done:
    /* If no result found, use default value */
    if (result == NULL && default_val != NULL) {
        result = resolve_string(ini, default_val, current_section, depth + 1);
    }
    
    free(base_var);
    return result;
}

/**
 * @brief Resolve all ${...} interpolations in a string
 *
 * @param ini             INI context
 * @param str             String with potential ${...} references
 * @param current_section Current section for relative lookups
 * @param depth           Recursion depth (for circular detection)
 * @return Newly allocated resolved string, or NULL on error
 */
static char* resolve_string(
    ZoryaIni *ini,
    const char *str,
    const char *current_section,
    int depth
) {
    if (str == NULL) return NULL;
    if (depth > INTERP_MAX_DEPTH) return NULL;  /* Circular reference */
    
    /* Quick check: no interpolation needed */
    if (strchr(str, '$') == NULL) {
        return str_dup(str);
    }
    
    /* Allocate output buffer */
    size_t out_cap = strlen(str) * 2 + 64;
    if (out_cap > INTERP_MAX_OUTPUT) out_cap = INTERP_MAX_OUTPUT;
    
    char *out = malloc(out_cap);
    if (out == NULL) return NULL;
    
    size_t out_len = 0;
    const char *p = str;
    
    while (*p != '\0') {
        /* Check for ${ */
        if (p[0] == '$' && p[1] == '{') {
            /* Find closing } */
            const char *start = p + 2;
            int brace_depth = 1;
            const char *end = start;
            
            while (*end != '\0' && brace_depth > 0) {
                if (*end == '{') brace_depth++;
                else if (*end == '}') brace_depth--;
                if (brace_depth > 0) end++;
            }
            
            if (brace_depth == 0) {
                /* Extract variable name */
                size_t var_len = (size_t)(end - start);
                char *var_name = malloc(var_len + 1);
                if (var_name == NULL) {
                    free(out);
                    return NULL;
                }
                memcpy(var_name, start, var_len);
                var_name[var_len] = '\0';
                
                /* Runtime variables (underscore prefix) are preserved verbatim.
                 * They will be resolved by the application at runtime.
                 * E.g., Ordinal's ${_target}, ${_deps}, ${_platform} */
                if (var_name[0] == '_') {
                    /* Preserve the ${_var} syntax in output */
                    size_t preserve_len = var_len + 3;  /* ${ + var + } */
                    while (out_len + preserve_len + 1 > out_cap) {
                        out_cap *= 2;
                        if (out_cap > INTERP_MAX_OUTPUT) {
                            free(var_name);
                            free(out);
                            return NULL;
                        }
                        char *new_out = realloc(out, out_cap);
                        if (new_out == NULL) {
                            free(var_name);
                            free(out);
                            return NULL;
                        }
                        out = new_out;
                    }
                    out[out_len++] = '$';
                    out[out_len++] = '{';
                    memcpy(out + out_len, var_name, var_len);
                    out_len += var_len;
                    out[out_len++] = '}';
                    free(var_name);
                    p = end + 1;
                    continue;
                }
                
                /* Resolve variable */
                char *resolved = find_variable(ini, var_name, current_section, depth);
                free(var_name);
                
                if (resolved != NULL) {
                    size_t res_len = strlen(resolved);
                    
                    /* Grow buffer if needed */
                    while (out_len + res_len + 1 > out_cap) {
                        out_cap *= 2;
                        if (out_cap > INTERP_MAX_OUTPUT) {
                            free(resolved);
                            free(out);
                            return NULL;
                        }
                        char *new_out = realloc(out, out_cap);
                        if (new_out == NULL) {
                            free(resolved);
                            free(out);
                            return NULL;
                        }
                        out = new_out;
                    }
                    
                    memcpy(out + out_len, resolved, res_len);
                    out_len += res_len;
                    free(resolved);
                }
                /* If not resolved, variable is silently dropped */
                
                p = end + 1;  /* Skip past } */
                continue;
            }
        }
        
        /* Regular character - copy to output */
        if (out_len + 1 >= out_cap) {
            out_cap *= 2;
            if (out_cap > INTERP_MAX_OUTPUT) {
                free(out);
                return NULL;
            }
            char *new_out = realloc(out, out_cap);
            if (new_out == NULL) {
                free(out);
                return NULL;
            }
            out = new_out;
        }
        
        out[out_len++] = *p++;
    }
    
    out[out_len] = '\0';
    return out;
}

/**
 * @brief DAGGER iteration callback for resolving interpolations
 */
typedef struct {
    ZoryaIni *ini;
    zorya_ini_error_t error;
} InterpContext;

static int resolve_entry_callback(
    const void *key,
    uint32_t key_len,
    void *value,
    void *userdata
) {
    (void)key;
    (void)key_len;
    
    InterpContext *ctx = (InterpContext*)userdata;
    IniEntry *entry = (IniEntry*)value;
    
    if (entry->raw_value == NULL) return 0;
    
    const char *raw_str = weave_cstr(entry->raw_value);
    
    /* Check if interpolation needed */
    if (strchr(raw_str, '$') == NULL) {
        return 0;  /* No interpolation needed */
    }
    
    /* Resolve interpolations in raw value */
    char *resolved = resolve_string(ctx->ini, raw_str, 
                                    weave_cstr(entry->section), 0);
    
    if (resolved == NULL) {
        ctx->error = ZORYA_INI_ERROR_INTERP;
        return 1;  /* Stop iteration */
    }
    
    /* Store resolved value (raw_value remains interned, unchanged) */
    free(entry->resolved_value);  /* In case called multiple times */
    entry->resolved_value = resolved;
    
    /* Re-parse based on type hint */
    if (entry->parsed.is_array || strchr(resolved, '|') != NULL) {
        /* Free old array if exists */
        if (entry->parsed.is_array && entry->parsed.v.arr.items != NULL) {
            for (size_t i = 0; i < entry->parsed.v.arr.count; i++) {
                free(entry->parsed.v.arr.items[i]);
            }
            free(entry->parsed.v.arr.items);
        }
        entry->parsed.is_array = true;
        entry->parsed.type = ZORYA_INI_TYPE_ARRAY;
        entry->parsed.v.arr.items = parse_array(resolved, &entry->parsed.v.arr.count);
    } else {
        entry->parsed.is_array = false;
        
        switch (entry->hint) {
            case ZORYA_INI_TYPE_INT:
                entry->parsed.type = ZORYA_INI_TYPE_INT;
                entry->parsed.v.i = strtoll(resolved, NULL, 10);
                break;
                
            case ZORYA_INI_TYPE_FLOAT:
                entry->parsed.type = ZORYA_INI_TYPE_FLOAT;
                entry->parsed.v.f = strtod(resolved, NULL);
                break;
                
            case ZORYA_INI_TYPE_BOOL:
                entry->parsed.type = ZORYA_INI_TYPE_BOOL;
                entry->parsed.v.b = parse_bool_value(resolved);
                break;
                
            default:
                entry->parsed.type = ZORYA_INI_TYPE_STRING;
                entry->parsed.v.str = resolved;  /* Point to resolved_value */
                break;
        }
    }
    
    return 0;  /* Continue iteration */
}

/**
 * @brief Resolve all interpolations in all entries
 *
 * Called after initial parsing is complete. Iterates all entries
 * and resolves ${...} references.
 *
 * @param ini INI context
 * @return ZORYA_INI_OK on success, error code on failure
 */
static zorya_ini_error_t resolve_all_interpolations(ZoryaIni *ini) {
    if (ini == NULL) return ZORYA_INI_ERROR_NULLPTR;
    
    InterpContext ctx = {
        .ini = ini,
        .error = ZORYA_INI_OK
    };
    
    dagger_foreach(ini->entries, resolve_entry_callback, &ctx);
    
    return ctx.error;
}

/* ============================================================
 * LIFECYCLE
 * ============================================================ */

ZoryaIni* zorya_ini_new(void) {
    ZoryaIni *ini = calloc(1, sizeof(ZoryaIni));
    if (ini == NULL) return NULL;
    
    /* Create hash table */
    ini->entries = dagger_create(INI_INITIAL_CAPACITY, NULL);
    if (ini->entries == NULL) {
        free(ini);
        return NULL;
    }
    
    /* Set up destructors */
    dagger_set_value_destroy(ini->entries, entry_destroy);
    dagger_set_key_destroy(ini->entries, key_destroy);
    
    /* Create Tablet for string interning (MEMORY OPTIMIZATION) */
    ini->strings = tablet_create();
    if (ini->strings == NULL) {
        dagger_destroy(ini->entries);
        free(ini);
        return NULL;
    }
    
    /* Allocate section array (owned strings for API compatibility) */
    ini->section_capacity = 16;
    ini->sections = calloc(ini->section_capacity, sizeof(char*));
    if (ini->sections == NULL) {
        tablet_destroy(ini->strings);
        dagger_destroy(ini->entries);
        free(ini);
        return NULL;
    }
    
    /* Allocate include stack */
    ini->include_stack = calloc(INI_MAX_INCLUDE_DEPTH, sizeof(char*));
    if (ini->include_stack == NULL) {
        free(ini->sections);
        tablet_destroy(ini->strings);
        dagger_destroy(ini->entries);
        free(ini);
        return NULL;
    }
    
    return ini;
}

void zorya_ini_free(ZoryaIni *ini) {
    if (ini == NULL) return;
    
    /* Free hash table (entries freed by destructor) */
    if (ini->entries != NULL) {
        dagger_destroy(ini->entries);
    }
    
    /* Free Tablet - this frees all interned strings at once */
    /* MUST happen AFTER dagger_destroy since entries reference Tablet strings */
    if (ini->strings != NULL) {
        tablet_destroy(ini->strings);
    }
    
    /* Free section names (owned strings, NOT interned) */
    if (ini->sections != NULL) {
        for (size_t i = 0; i < ini->section_count; i++) {
            free(ini->sections[i]);
        }
        free(ini->sections);
    }
    
    /* Free include stack */
    if (ini->include_stack != NULL) {
        for (int i = 0; i < INI_MAX_INCLUDE_DEPTH; i++) {
            free(ini->include_stack[i]);
        }
        free(ini->include_stack);
    }
    
    /* Free error strings */
    free(ini->error_message);
    free(ini->error_file);
    free(ini->base_path);
    
    free(ini);
}

/**
 * @brief Entry destructor for DAGGER
 * 
 * NOTE: section, key, raw_value are interned in Tablet - DO NOT FREE THEM.
 * The Tablet owns those strings and will free them when destroyed.
 */
static void entry_destroy(void *value) {
    IniEntry *entry = (IniEntry*)value;
    if (entry == NULL) return;
    
    /* DO NOT free entry->section, entry->key, entry->raw_value */
    /* They are interned Weave pointers owned by the Tablet */
    
    /* Free resolved_value if it was allocated during interpolation */
    free(entry->resolved_value);
    
    /* Free parsed array if applicable */
    if (entry->parsed.is_array && entry->parsed.v.arr.items != NULL) {
        for (size_t i = 0; i < entry->parsed.v.arr.count; i++) {
            free(entry->parsed.v.arr.items[i]);
        }
        free(entry->parsed.v.arr.items);
    } else if (entry->parsed.type == ZORYA_INI_TYPE_STRING && 
               entry->parsed.v.str != NULL &&
               entry->parsed.v.str != weave_cstr(entry->raw_value) &&
               entry->parsed.v.str != entry->resolved_value) {
        /* Only free if it's a third allocation (shouldn't happen normally) */
        free(entry->parsed.v.str);
    }
    
    free(entry);
}

/**
 * @brief Key destructor for DAGGER
 */
static void key_destroy(const void *key, uint32_t key_len) {
    (void)key_len;
    free((void*)key);
}

/* ============================================================
 * LOADING
 * ============================================================ */

zorya_ini_error_t zorya_ini_load(ZoryaIni *ini, const char *filepath) {
    if (ini == NULL || filepath == NULL) {
        return ZORYA_INI_ERROR_NULLPTR;
    }
    
    /* Store base path for includes */
    free(ini->base_path);
    ini->base_path = get_directory(filepath);
    
    /* Open file */
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL) {
        return ZORYA_INI_ERROR_IO;
    }
    
    /* Get file size */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    /* Allow empty files (file_size == 0), reject only negative or huge files */
    if (file_size < 0 || file_size > 100 * 1024 * 1024) {
        fclose(fp);
        return ZORYA_INI_ERROR_IO;
    }
    
    /* Handle empty file early */
    if (file_size == 0) {
        fclose(fp);
        return ZORYA_INI_OK;  /* Empty file is valid */
    }
    
    /* Read file */
    char *data = malloc((size_t)file_size + 1);
    if (data == NULL) {
        fclose(fp);
        return ZORYA_INI_ERROR_NOMEM;
    }
    
    size_t bytes_read = fread(data, 1, (size_t)file_size, fp);
    fclose(fp);
    
    if (bytes_read != (size_t)file_size) {
        free(data);
        return ZORYA_INI_ERROR_IO;
    }
    data[bytes_read] = '\0';
    
    /* Parse */
    zorya_ini_error_t result = parse_buffer(ini, data, bytes_read, filepath);
    
    free(data);
    return result;
}

zorya_ini_error_t zorya_ini_load_buffer(
    ZoryaIni *ini,
    const char *data,
    size_t len,
    const char *name
) {
    if (ini == NULL || data == NULL) {
        return ZORYA_INI_ERROR_NULLPTR;
    }
    
    return parse_buffer(ini, data, len, name ? name : "<buffer>");
}

/* ============================================================
 * PARSING
 * ============================================================ */

/**
 * @brief Add section to list if not exists
 */
static void add_section(ZoryaIni *ini, const char *section) {
    /* Check if already exists */
    for (size_t i = 0; i < ini->section_count; i++) {
        if (strcmp(ini->sections[i], section) == 0) {
            return;
        }
    }
    
    /* Grow array if needed */
    if (ini->section_count >= ini->section_capacity) {
        size_t new_cap = ini->section_capacity * 2;
        char **new_arr = realloc(ini->sections, new_cap * sizeof(char*));
        if (new_arr == NULL) return;
        ini->sections = new_arr;
        ini->section_capacity = new_cap;
    }
    
    ini->sections[ini->section_count++] = str_dup(section);
}

/**
 * @brief Parse type hint from "key:type"
 */
static zorya_ini_type_t parse_type_hint(const char *hint) {
    if (hint == NULL) return ZORYA_INI_TYPE_STRING;
    
    if (strcmp(hint, "int") == 0) return ZORYA_INI_TYPE_INT;
    if (strcmp(hint, "float") == 0) return ZORYA_INI_TYPE_FLOAT;
    if (strcmp(hint, "bool") == 0) return ZORYA_INI_TYPE_BOOL;
    if (strcmp(hint, "path") == 0) return ZORYA_INI_TYPE_PATH;
    if (strcmp(hint, "url") == 0) return ZORYA_INI_TYPE_URL;
    if (strcmp(hint, "date") == 0) return ZORYA_INI_TYPE_DATE;
    if (strcmp(hint, "datetime") == 0) return ZORYA_INI_TYPE_DATETIME;
    if (strcmp(hint, "str") == 0) return ZORYA_INI_TYPE_STRING;
    
    /* Check for array types */
    size_t len = strlen(hint);
    if (len > 2 && hint[len-2] == '[' && hint[len-1] == ']') {
        return ZORYA_INI_TYPE_ARRAY;
    }
    
    return ZORYA_INI_TYPE_STRING;
}

/**
 * @brief Parse boolean value
 */
static bool parse_bool_value(const char *str) {
    if (str == NULL) return false;
    
    /* Skip whitespace */
    while (*str == ' ' || *str == '\t') str++;
    
    /* True values */
    if (strcasecmp(str, "true") == 0) return true;
    if (strcasecmp(str, "yes") == 0) return true;
    if (strcasecmp(str, "on") == 0) return true;
    if (strcmp(str, "1") == 0) return true;
    
    return false;
}

/**
 * @brief Parse pipe-delimited array
 */
static char** parse_array(const char *value, size_t *count) {
    *count = 0;
    if (value == NULL || *value == '\0') return NULL;
    
    /* Count pipes to estimate size */
    size_t capacity = 8;
    const char *p = value;
    while (*p) {
        if (*p == '|') capacity++;
        p++;
    }
    
    char **items = calloc(capacity, sizeof(char*));
    if (items == NULL) return NULL;
    
    /* Parse items */
    p = value;
    while (*p) {
        /* Skip leading whitespace */
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;
        
        /* Find end of item */
        const char *start = p;
        while (*p && *p != '|') p++;
        
        /* Calculate length (trim trailing whitespace) */
        const char *end = p;
        while (end > start && (end[-1] == ' ' || end[-1] == '\t')) end--;
        
        size_t len = (size_t)(end - start);
        if (len > 0) {
            items[*count] = malloc(len + 1);
            if (items[*count] != NULL) {
                memcpy(items[*count], start, len);
                items[*count][len] = '\0';
                (*count)++;
            }
        }
        
        /* Skip pipe */
        if (*p == '|') p++;
    }
    
    return items;
}

/**
 * @brief Make full key from section and key
 */
static char* make_full_key(const char *section, const char *key) {
    if (section == NULL || *section == '\0') {
        return str_dup(key);
    }
    
    size_t sec_len = strlen(section);
    size_t key_len = strlen(key);
    
    char *full = malloc(sec_len + key_len + 2);
    if (full == NULL) return NULL;
    
    memcpy(full, section, sec_len);
    full[sec_len] = '.';
    memcpy(full + sec_len + 1, key, key_len + 1);
    
    return full;
}

/**
 * @brief Add entry to hash table
 */
static zorya_ini_error_t add_entry(
    ZoryaIni *ini,
    const char *section,
    const char *key,
    const char *value,
    zorya_ini_type_t hint,
    int line
) {
    /* Create full key for DAGGER lookup */
    char *full_key = make_full_key(section, key);
    if (full_key == NULL) return ZORYA_INI_ERROR_NOMEM;
    
    /* Create entry */
    IniEntry *entry = calloc(1, sizeof(IniEntry));
    if (entry == NULL) {
        free(full_key);
        return ZORYA_INI_ERROR_NOMEM;
    }
    
    /* Intern strings via Tablet (MEMORY OPTIMIZATION)
     * Same strings share storage - huge savings when same section
     * has many keys, or same keys appear in different sections */
    entry->section = tablet_intern(ini->strings, section ? section : "");
    entry->key = tablet_intern(ini->strings, key);
    entry->raw_value = tablet_intern(ini->strings, value);
    entry->hint = hint;
    entry->line = line;
    
    /* Parse value based on type hint */
    if (strchr(value, '|') != NULL) {
        /* Array value */
        entry->parsed.is_array = true;
        entry->parsed.type = ZORYA_INI_TYPE_ARRAY;
        entry->parsed.v.arr.items = parse_array(value, &entry->parsed.v.arr.count);
    } else {
        entry->parsed.is_array = false;
        
        switch (hint) {
            case ZORYA_INI_TYPE_INT:
                entry->parsed.type = ZORYA_INI_TYPE_INT;
                entry->parsed.v.i = strtoll(value, NULL, 10);
                break;
                
            case ZORYA_INI_TYPE_FLOAT:
                entry->parsed.type = ZORYA_INI_TYPE_FLOAT;
                entry->parsed.v.f = strtod(value, NULL);
                break;
                
            case ZORYA_INI_TYPE_BOOL:
                entry->parsed.type = ZORYA_INI_TYPE_BOOL;
                entry->parsed.v.b = parse_bool_value(value);
                break;
                
            default:
                entry->parsed.type = ZORYA_INI_TYPE_STRING;
                /* Point to interned string's C-string */
                entry->parsed.v.str = (char*)weave_cstr(entry->raw_value);
                break;
        }
    }
    
    /* Insert into hash table */
    dagger_result_t r = dagger_set(ini->entries, full_key, 
                                   (uint32_t)strlen(full_key), entry, 1);
    
    if (r != DAGGER_OK) {
        /* Don't call entry_destroy - it would try to free Tablet strings */
        free(entry);
        free(full_key);
        return ZORYA_INI_ERROR_NOMEM;
    }
    
    ini->key_count++;
    return ZORYA_INI_OK;
}

/**
 * @brief Parse INI buffer
 */
static zorya_ini_error_t parse_buffer(
    ZoryaIni *ini,
    const char *data,
    size_t len,
    const char *filepath
) {
    (void)filepath;  /* Reserved for error messages */
    
    char current_section[INI_MAX_SECTION_LENGTH] = "";
    char line_buf[INI_MAX_LINE_LENGTH];
    char value_buf[INI_MAX_LINE_LENGTH * 4];  /* For multiline */
    int line_num = 0;
    int value_start_line = 0;
    bool in_multiline = false;
    char pending_key[INI_MAX_KEY_LENGTH] = "";
    zorya_ini_type_t pending_hint = ZORYA_INI_TYPE_STRING;
    
    const char *p = data;
    const char *end = data + len;
    
    while (p < end) {
        /* Read line */
        size_t line_len = 0;
        while (p < end && *p != '\n' && line_len < INI_MAX_LINE_LENGTH - 1) {
            line_buf[line_len++] = *p++;
        }
        line_buf[line_len] = '\0';
        if (p < end && *p == '\n') p++;
        line_num++;
        
        /* Remove CR if present */
        if (line_len > 0 && line_buf[line_len - 1] == '\r') {
            line_buf[--line_len] = '\0';
        }
        
        /* Check for continuation (indented line) */
        if (in_multiline && (line_buf[0] == ' ' || line_buf[0] == '\t')) {
            /* Find first non-whitespace */
            const char *content = skip_ws(line_buf);
            
            /* Append to value buffer */
            size_t val_len = strlen(value_buf);
            size_t add_len = strlen(content);
            if (val_len + add_len + 2 < sizeof(value_buf)) {
                if (val_len > 0) {
                    value_buf[val_len++] = '\n';
                }
                memcpy(value_buf + val_len, content, add_len + 1);
            }
            continue;
        }
        
        /* End of multiline - save pending entry */
        if (in_multiline) {
            add_entry(ini, current_section, pending_key, value_buf,
                     pending_hint, value_start_line);
            in_multiline = false;
            pending_key[0] = '\0';
            value_buf[0] = '\0';
        }
        
        /* Skip empty lines */
        const char *lp = skip_ws(line_buf);
        if (*lp == '\0') continue;
        
        /* Skip comments */
        if (*lp == '#' || *lp == ';') continue;
        
        /* Check for directive */
        if (lp[0] == ':' && lp[1] == ':') {
            lp += 2;
            
            /* Parse directive name */
            if (strncmp(lp, "include", 7) == 0) {
                lp += 7;
                bool optional = (*lp == '?');
                if (optional) lp++;
                
                lp = skip_ws(lp);
                trim_trailing((char*)lp);
                
                /* Resolve include path */
                char *inc_path = join_path(ini->base_path, lp);
                if (inc_path == NULL) {
                    return ZORYA_INI_ERROR_NOMEM;
                }
                
                /* Check include depth */
                if (ini->include_depth >= INI_MAX_INCLUDE_DEPTH) {
                    free(inc_path);
                    return ZORYA_INI_ERROR_CIRCULAR;
                }
                
                /* Load included file */
                FILE *inc_fp = fopen(inc_path, "rb");
                if (inc_fp == NULL) {
                    free(inc_path);
                    if (optional) continue;
                    return ZORYA_INI_ERROR_INCLUDE;
                }
                
                /* Read file */
                fseek(inc_fp, 0, SEEK_END);
                long inc_size = ftell(inc_fp);
                fseek(inc_fp, 0, SEEK_SET);
                
                char *inc_data = malloc((size_t)inc_size + 1);
                if (inc_data == NULL) {
                    fclose(inc_fp);
                    free(inc_path);
                    return ZORYA_INI_ERROR_NOMEM;
                }
                
                size_t inc_read = fread(inc_data, 1, (size_t)inc_size, inc_fp);
                fclose(inc_fp);
                inc_data[inc_read] = '\0';
                
                /* Save current base path and recurse */
                char *old_base = ini->base_path;
                ini->base_path = get_directory(inc_path);
                ini->include_depth++;
                ini->include_count++;
                
                zorya_ini_error_t inc_err = parse_buffer(ini, inc_data, 
                                                          inc_read, inc_path);
                
                ini->include_depth--;
                free(ini->base_path);
                ini->base_path = old_base;
                free(inc_data);
                free(inc_path);
                
                if (inc_err != ZORYA_INI_OK) {
                    return inc_err;
                }
            }
            continue;
        }
        
        /* Check for section */
        if (*lp == '[') {
            lp++;
            const char *sec_start = lp;
            while (*lp && *lp != ']') lp++;
            
            size_t sec_len = (size_t)(lp - sec_start);
            if (sec_len > 0 && sec_len < INI_MAX_SECTION_LENGTH) {
                memcpy(current_section, sec_start, sec_len);
                current_section[sec_len] = '\0';
                add_section(ini, current_section);
            }
            continue;
        }
        
        /* Must be key = value */
        const char *key_start = lp;
        
        /* Find = or : (for type hint) */
        while (*lp && *lp != '=' && *lp != ':') lp++;
        
        if (*lp == '\0') continue;  /* Invalid line */
        
        /* Extract key */
        const char *key_end = lp;
        while (key_end > key_start && (key_end[-1] == ' ' || key_end[-1] == '\t')) {
            key_end--;
        }
        
        size_t key_len = (size_t)(key_end - key_start);
        if (key_len == 0 || key_len >= INI_MAX_KEY_LENGTH) continue;
        
        char key[INI_MAX_KEY_LENGTH];
        memcpy(key, key_start, key_len);
        key[key_len] = '\0';
        
        /* Check for type hint */
        zorya_ini_type_t hint = ZORYA_INI_TYPE_STRING;
        if (*lp == ':') {
            lp++;
            const char *hint_start = lp;
            while (*lp && *lp != '=' && *lp != ' ' && *lp != '\t') lp++;
            
            size_t hint_len = (size_t)(lp - hint_start);
            char hint_str[32];
            if (hint_len > 0 && hint_len < sizeof(hint_str)) {
                memcpy(hint_str, hint_start, hint_len);
                hint_str[hint_len] = '\0';
                hint = parse_type_hint(hint_str);
            }
            
            /* Skip to = */
            while (*lp && *lp != '=') lp++;
        }
        
        if (*lp != '=') continue;  /* Invalid line */
        lp++;
        
        /* Skip whitespace after = */
        lp = skip_ws(lp);
        
        /* Check for empty value (multiline follows) */
        if (*lp == '\0') {
            in_multiline = true;
            strncpy(pending_key, key, INI_MAX_KEY_LENGTH - 1);
            pending_key[INI_MAX_KEY_LENGTH - 1] = '\0';
            pending_hint = hint;
            value_buf[0] = '\0';
            value_start_line = line_num;
            continue;
        }
        
        /* Single-line value */
        char value[INI_MAX_LINE_LENGTH];
        strncpy(value, lp, INI_MAX_LINE_LENGTH - 1);
        value[INI_MAX_LINE_LENGTH - 1] = '\0';
        trim_trailing(value);
        
        add_entry(ini, current_section, key, value, hint, line_num);
    }
    
    /* Handle trailing multiline */
    if (in_multiline && pending_key[0] != '\0') {
        add_entry(ini, current_section, pending_key, value_buf,
                 pending_hint, value_start_line);
    }
    
    /* Resolve interpolations (only at top level, not during includes) */
    if (ini->include_depth == 0) {
        zorya_ini_error_t interp_err = resolve_all_interpolations(ini);
        if (interp_err != ZORYA_INI_OK) {
            return interp_err;
        }
    }
    
    return ZORYA_INI_OK;
}

/* ============================================================
 * GETTERS
 * ============================================================ */

const char* zorya_ini_get(const ZoryaIni *ini, const char *key) {
    if (ini == NULL || key == NULL) return NULL;
    
    void *value = NULL;
    dagger_result_t r = dagger_get(ini->entries, key, 
                                   (uint32_t)strlen(key), &value);
    
    if (r != DAGGER_OK || value == NULL) return NULL;
    
    IniEntry *entry = (IniEntry*)value;
    /* Return resolved value if available, otherwise raw value */
    if (entry->resolved_value != NULL) {
        return entry->resolved_value;
    }
    return weave_cstr(entry->raw_value);
}

const char* zorya_ini_get_default(
    const ZoryaIni *ini,
    const char *key,
    const char *def
) {
    const char *val = zorya_ini_get(ini, key);
    return val ? val : def;
}

int64_t zorya_ini_get_int(const ZoryaIni *ini, const char *key) {
    return zorya_ini_get_int_default(ini, key, 0);
}

int64_t zorya_ini_get_int_default(
    const ZoryaIni *ini,
    const char *key,
    int64_t def
) {
    if (ini == NULL || key == NULL) return def;
    
    void *value = NULL;
    dagger_result_t r = dagger_get(ini->entries, key,
                                   (uint32_t)strlen(key), &value);
    
    if (r != DAGGER_OK || value == NULL) return def;
    
    IniEntry *entry = (IniEntry*)value;
    
    if (entry->parsed.type == ZORYA_INI_TYPE_INT) {
        return entry->parsed.v.i;
    }
    
    /* Try to parse as int - use resolved_value if available */
    const char *str_val = entry->resolved_value ? 
                          entry->resolved_value : 
                          weave_cstr(entry->raw_value);
    char *endptr;
    long long val = strtoll(str_val, &endptr, 10);
    if (endptr == str_val) return def;
    
    return (int64_t)val;
}

double zorya_ini_get_float(const ZoryaIni *ini, const char *key) {
    return zorya_ini_get_float_default(ini, key, 0.0);
}

double zorya_ini_get_float_default(
    const ZoryaIni *ini,
    const char *key,
    double def
) {
    if (ini == NULL || key == NULL) return def;
    
    void *value = NULL;
    dagger_result_t r = dagger_get(ini->entries, key,
                                   (uint32_t)strlen(key), &value);
    
    if (r != DAGGER_OK || value == NULL) return def;
    
    IniEntry *entry = (IniEntry*)value;
    
    if (entry->parsed.type == ZORYA_INI_TYPE_FLOAT) {
        return entry->parsed.v.f;
    }
    
    /* Try to parse as float - use resolved_value if available */
    const char *str_val = entry->resolved_value ? 
                          entry->resolved_value : 
                          weave_cstr(entry->raw_value);
    char *endptr;
    double val = strtod(str_val, &endptr);
    if (endptr == str_val) return def;
    
    return val;
}

bool zorya_ini_get_bool(const ZoryaIni *ini, const char *key) {
    return zorya_ini_get_bool_default(ini, key, false);
}

bool zorya_ini_get_bool_default(
    const ZoryaIni *ini,
    const char *key,
    bool def
) {
    if (ini == NULL || key == NULL) return def;
    
    void *value = NULL;
    dagger_result_t r = dagger_get(ini->entries, key,
                                   (uint32_t)strlen(key), &value);
    
    if (r != DAGGER_OK || value == NULL) return def;
    
    IniEntry *entry = (IniEntry*)value;
    
    if (entry->parsed.type == ZORYA_INI_TYPE_BOOL) {
        return entry->parsed.v.b;
    }
    
    /* Use resolved_value if available */
    const char *str_val = entry->resolved_value ? 
                          entry->resolved_value : 
                          weave_cstr(entry->raw_value);
    return parse_bool_value(str_val);
}

const char** zorya_ini_get_array(
    const ZoryaIni *ini,
    const char *key,
    size_t *count
) {
    if (count != NULL) *count = 0;
    if (ini == NULL || key == NULL) return NULL;
    
    void *value = NULL;
    dagger_result_t r = dagger_get(ini->entries, key,
                                   (uint32_t)strlen(key), &value);
    
    if (r != DAGGER_OK || value == NULL) return NULL;
    
    IniEntry *entry = (IniEntry*)value;
    
    if (!entry->parsed.is_array) return NULL;
    
    if (count != NULL) {
        *count = entry->parsed.v.arr.count;
    }
    
    return (const char**)entry->parsed.v.arr.items;
}

size_t zorya_ini_get_array_len(const ZoryaIni *ini, const char *key) {
    size_t count = 0;
    zorya_ini_get_array(ini, key, &count);
    return count;
}

/* ============================================================
 * EXISTENCE CHECKS
 * ============================================================ */

bool zorya_ini_has(const ZoryaIni *ini, const char *key) {
    if (ini == NULL || key == NULL) return false;
    
    void *value = NULL;
    dagger_result_t r = dagger_get(ini->entries, key,
                                   (uint32_t)strlen(key), &value);
    
    return r == DAGGER_OK && value != NULL;
}

bool zorya_ini_has_section(const ZoryaIni *ini, const char *section) {
    if (ini == NULL || section == NULL) return false;
    
    for (size_t i = 0; i < ini->section_count; i++) {
        if (strcmp(ini->sections[i], section) == 0) {
            return true;
        }
    }
    
    return false;
}

/* ============================================================
 * SETTERS
 * ============================================================ */

zorya_ini_error_t zorya_ini_set(
    ZoryaIni *ini,
    const char *key,
    const char *value
) {
    if (ini == NULL || key == NULL) {
        return ZORYA_INI_ERROR_NULLPTR;
    }
    
    /* Find section and key parts */
    const char *dot = strrchr(key, '.');
    char section[INI_MAX_SECTION_LENGTH] = "";
    const char *key_part = key;
    
    if (dot != NULL) {
        size_t sec_len = (size_t)(dot - key);
        if (sec_len >= INI_MAX_SECTION_LENGTH) {
            return ZORYA_INI_ERROR_SYNTAX;
        }
        memcpy(section, key, sec_len);
        section[sec_len] = '\0';
        key_part = dot + 1;
        
        add_section(ini, section);
    }
    
    return add_entry(ini, section, key_part, value ? value : "", 
                    ZORYA_INI_TYPE_STRING, 0);
}

zorya_ini_error_t zorya_ini_set_int(
    ZoryaIni *ini,
    const char *key,
    int64_t value
) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)value);
    return zorya_ini_set(ini, key, buf);
}

zorya_ini_error_t zorya_ini_set_float(
    ZoryaIni *ini,
    const char *key,
    double value
) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", value);
    return zorya_ini_set(ini, key, buf);
}

zorya_ini_error_t zorya_ini_set_bool(
    ZoryaIni *ini,
    const char *key,
    bool value
) {
    return zorya_ini_set(ini, key, value ? "true" : "false");
}

zorya_ini_error_t zorya_ini_set_array(
    ZoryaIni *ini,
    const char *key,
    const char **values,
    size_t count
) {
    if (ini == NULL || key == NULL) {
        return ZORYA_INI_ERROR_NULLPTR;
    }
    
    if (count == 0 || values == NULL) {
        return zorya_ini_set(ini, key, "");
    }
    
    /* Calculate buffer size */
    size_t total_len = 0;
    for (size_t i = 0; i < count; i++) {
        if (values[i] != NULL) {
            total_len += strlen(values[i]);
        }
        if (i < count - 1) {
            total_len += 3;  /* " | " */
        }
    }
    
    char *buf = malloc(total_len + 1);
    if (buf == NULL) {
        return ZORYA_INI_ERROR_NOMEM;
    }
    
    /* Build pipe-separated string */
    char *p = buf;
    for (size_t i = 0; i < count; i++) {
        if (values[i] != NULL) {
            size_t len = strlen(values[i]);
            memcpy(p, values[i], len);
            p += len;
        }
        if (i < count - 1) {
            memcpy(p, " | ", 3);
            p += 3;
        }
    }
    *p = '\0';
    
    zorya_ini_error_t result = zorya_ini_set(ini, key, buf);
    free(buf);
    
    return result;
}

/* ============================================================
 * ITERATION
 * ============================================================ */

/**
 * @brief Iteration context
 */
typedef struct {
    zorya_ini_callback_t callback;
    void *userdata;
    const char *filter_section;
    int count;
} IterContext;

/**
 * @brief DAGGER iteration callback
 */
static int iter_callback(
    const void *key,
    uint32_t key_len,
    void *value,
    void *userdata
) {
    (void)key;
    (void)key_len;
    
    IterContext *ctx = (IterContext*)userdata;
    IniEntry *entry = (IniEntry*)value;
    
    const char *section_str = weave_cstr(entry->section);
    const char *key_str = weave_cstr(entry->key);
    /* Use resolved_value if available, otherwise raw_value */
    const char *value_str = entry->resolved_value ? 
                            entry->resolved_value : 
                            weave_cstr(entry->raw_value);
    
    /* Filter by section if specified */
    if (ctx->filter_section != NULL) {
        if (strcmp(section_str, ctx->filter_section) != 0) {
            return 0;  /* Continue */
        }
    }
    
    ctx->count++;
    
    return ctx->callback(
        section_str[0] ? section_str : NULL,
        key_str,
        value_str,
        ctx->userdata
    );
}

int zorya_ini_foreach(
    const ZoryaIni *ini,
    zorya_ini_callback_t callback,
    void *userdata
) {
    if (ini == NULL || callback == NULL) return -1;
    
    IterContext ctx = {
        .callback = callback,
        .userdata = userdata,
        .filter_section = NULL,
        .count = 0
    };
    
    dagger_foreach(ini->entries, iter_callback, &ctx);
    
    return ctx.count;
}

int zorya_ini_foreach_section(
    const ZoryaIni *ini,
    const char *section,
    zorya_ini_callback_t callback,
    void *userdata
) {
    if (ini == NULL || callback == NULL) return -1;
    
    IterContext ctx = {
        .callback = callback,
        .userdata = userdata,
        .filter_section = section,
        .count = 0
    };
    
    dagger_foreach(ini->entries, iter_callback, &ctx);
    
    return ctx.count;
}

const char** zorya_ini_sections(const ZoryaIni *ini, size_t *count) {
    if (ini == NULL) {
        if (count) *count = 0;
        return NULL;
    }
    
    if (count) *count = ini->section_count;
    return (const char**)ini->sections;
}

/* ============================================================
 * ERROR HANDLING
 * ============================================================ */

const char* zorya_ini_strerror(zorya_ini_error_t err) {
    switch (err) {
        case ZORYA_INI_OK:           return "Success";
        case ZORYA_INI_ERROR_NULLPTR: return "NULL pointer argument";
        case ZORYA_INI_ERROR_NOMEM:   return "Memory allocation failed";
        case ZORYA_INI_ERROR_IO:      return "File I/O error";
        case ZORYA_INI_ERROR_SYNTAX:  return "Syntax error";
        case ZORYA_INI_ERROR_NOT_FOUND: return "Key not found";
        case ZORYA_INI_ERROR_TYPE:    return "Type conversion error";
        case ZORYA_INI_ERROR_CIRCULAR: return "Circular reference detected";
        case ZORYA_INI_ERROR_INCLUDE: return "Include file error";
        case ZORYA_INI_ERROR_INTERP:  return "Interpolation error";
        default:                      return "Unknown error";
    }
}

const ZoryaIniParseError* zorya_ini_last_error(const ZoryaIni *ini) {
    if (ini == NULL) return NULL;
    if (ini->last_error.line == 0) return NULL;
    return &ini->last_error;
}

/* ============================================================
 * DIAGNOSTICS
 * ============================================================ */

void zorya_ini_stats(const ZoryaIni *ini, ZoryaIniStats *stats) {
    if (ini == NULL || stats == NULL) return;
    
    memset(stats, 0, sizeof(*stats));
    stats->section_count = ini->section_count;
    stats->key_count = ini->key_count;
    stats->include_count = ini->include_count;
    
    /* Estimate memory from key count */
    stats->memory_bytes = ini->key_count * 200;  /* Rough estimate */
    stats->load_factor = 0.5;  /* Placeholder */
}

void zorya_ini_print_stats(const ZoryaIni *ini) {
    if (ini == NULL) return;
    
    ZoryaIniStats stats;
    zorya_ini_stats(ini, &stats);
    
    printf("=== ZORYA-INI Stats ===\n");
    printf("Sections:   %zu\n", stats.section_count);
    printf("Keys:       %zu\n", stats.key_count);
    printf("Includes:   %zu\n", stats.include_count);
    printf("Memory:     %.2f KB\n", (double)stats.memory_bytes / 1024.0);
    printf("Load:       %.1f%%\n", stats.load_factor * 100.0);
    printf("=======================\n");
}

/**
 * @brief Dump callback
 */
static int dump_callback(
    const char *section,
    const char *key,
    const char *value,
    void *userdata
) {
    (void)userdata;
    
    if (section != NULL && section[0] != '\0') {
        printf("[%s] %s = %s\n", section, key, value);
    } else {
        printf("%s = %s\n", key, value);
    }
    
    return 0;
}

void zorya_ini_dump(const ZoryaIni *ini) {
    if (ini == NULL) return;
    
    printf("=== ZORYA-INI Dump ===\n");
    zorya_ini_foreach(ini, dump_callback, NULL);
    printf("======================\n");
}
