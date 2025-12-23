/**
 * @file ordinal.c
 * @brief Ordinal Build System - Implementation
 *
 * @author Anthony Taliento
 * @date 2025-12-18
 * @version 0.1.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * ZORYA-C COMPLIANCE: v2.0.0 (Strict Mode)
 *
 * DESCRIPTION:
 *   Ordinal is a Makefile replacement built on ZORYA-INI.
 *   Same mental model, less bullshit.
 *
 * DEPENDENCIES:
 *   - zorya_ini.h (INI parser)
 *   - dagger.h (hash tables)
 *   - glob.h (file patterns)
 */

#define _GNU_SOURCE  /* For strdup, popen, etc. */

#include "ordinal.h"
#include "zorya_ini.h"
#include "dagger.h"
#include "weave.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glob.h>
#include <dirent.h>

/* ============================================================
 * INTERNAL CONSTANTS
 * ============================================================ */

#define ORD_MAX_TARGETS      256
#define ORD_MAX_DEPS         128
#define ORD_MAX_CMD_LENGTH   8192
#define ORD_MAX_VAR_LENGTH   4096
#define ORD_MAX_PATH_LENGTH  4096
#define ORD_MAX_RECURSION    32

/* Default Ordinal file names to search */
static const char *ORDINAL_FILENAMES[] = {
    "Ordinal",
    "Ordinal.ini",
    "ordinal",
    "ordinal.ini",
    NULL
};

/* ============================================================
 * INTERNAL STRUCTURES
 * ============================================================ */

/**
 * @brief Internal target representation
 */
typedef struct OrdinalTargetInternal {
    char *name;                    /**< Target name */
    char *section;                 /**< INI section (e.g., "build", "clean") */
    char **deps;                   /**< Dependency names/patterns */
    size_t dep_count;              /**< Number of dependencies */
    char **resolved_deps;          /**< Resolved file paths */
    size_t resolved_dep_count;     /**< Number of resolved deps */
    char *command;                 /**< Command template */
    char *resolved_command;        /**< Command with vars substituted */
    char *target_file;             /**< Output file path (if applicable) */
    ordinal_status_t status;       /**< Build status */
    double build_time_ms;          /**< Build time */
    bool is_phony;                 /**< True if no output file */
    bool visited;                  /**< For cycle detection */
    bool in_stack;                 /**< For cycle detection */
} OrdinalTargetInternal;

/**
 * @brief Ordinal context
 */
struct Ordinal {
    /* INI backing store */
    ZoryaIni *ini;
    
    /* Configuration */
    OrdinalConfig config;
    
    /* Targets */
    OrdinalTargetInternal *targets;
    size_t target_count;
    size_t target_capacity;
    DaggerTable *target_map;       /**< name -> target index */
    
    /* WEAVE components */
    Tablet *strings;               /**< Interned strings */
    const Weave *sec_build;        /**< Cached "build" */
    const Weave *sec_clean;        /**< Cached "clean" */
    const Weave *sec_test;         /**< Cached "test" */
    const Weave *sec_install;      /**< Cached "install" */
    const Weave *sec_project;      /**< Cached "project" */
    const Weave *sec_env;          /**< Cached "env" */
    
    /* Default target */
    char *default_target;
    
    /* Project info */
    char *project_name;
    char *project_version;
    
    /* Paths */
    char *ordinal_dir;             /**< Directory containing Ordinal file */
    char *cwd;                     /**< Original working directory */
    
    /* Built-in variables */
    char *var_platform;
    char *var_arch;
    int var_nproc;
    
    /* Callbacks */
    ordinal_progress_fn progress_fn;
    void *progress_userdata;
    ordinal_output_fn output_fn;
    void *output_userdata;
    
    /* Results */
    OrdinalResult result;
    
    /* Error handling */
    char error_msg[1024];
};

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

static void target_destroy(OrdinalTargetInternal *target);
static ordinal_error_t discover_targets(Ordinal *ord);
static ordinal_error_t resolve_target_deps(Ordinal *ord, OrdinalTargetInternal *target);
static ordinal_error_t resolve_command(Ordinal *ord, OrdinalTargetInternal *target);
static ordinal_error_t build_target(Ordinal *ord, const char *name, int depth);
static bool needs_rebuild(Ordinal *ord, OrdinalTargetInternal *target);
static ordinal_error_t execute_command(Ordinal *ord, OrdinalTargetInternal *target);
static char* expand_glob(const char *pattern, size_t *count);
static time_t get_mtime(const char *path);
static char* str_dup(const char *s);

/* ============================================================
 * PLATFORM DETECTION
 * ============================================================ */

const char* ordinal_detect_platform(void) {
#if defined(__linux__)
    return "linux";
#elif defined(__APPLE__)
    return "darwin";
#elif defined(_WIN32) || defined(_WIN64)
    return "windows";
#elif defined(__FreeBSD__)
    return "freebsd";
#elif defined(__OpenBSD__)
    return "openbsd";
#else
    return "unknown";
#endif
}

const char* ordinal_detect_arch(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#elif defined(__riscv)
    return "riscv";
#elif defined(__powerpc64__)
    return "ppc64";
#else
    return "unknown";
#endif
}

int ordinal_detect_nproc(void) {
#if defined(_SC_NPROCESSORS_ONLN)
    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    return (nproc > 0) ? (int)nproc : 1;
#elif defined(_WIN32)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return 1;
#endif
}

/* ============================================================
 * HELPER FUNCTIONS
 * ============================================================ */

static char* str_dup(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (copy != NULL) {
        memcpy(copy, s, len + 1);
    }
    return copy;
}

static time_t get_mtime(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;  /* File doesn't exist */
    }
    return st.st_mtime;
}

/**
 * @brief Expand glob pattern to file list
 *
 * @param pattern Glob pattern (e.g., "src/[star].c")
 * @param count   Output: number of matches
 * @return Space-separated list of files (caller frees)
 */
static char* expand_glob(const char *pattern, size_t *count) {
    glob_t gl;
    *count = 0;
    
    int ret = glob(pattern, GLOB_NOSORT | GLOB_NOCHECK, NULL, &gl);
    if (ret != 0 && ret != GLOB_NOMATCH) {
        return str_dup(pattern);  /* Return pattern as-is on error */
    }
    
    if (gl.gl_pathc == 0) {
        globfree(&gl);
        return str_dup("");
    }
    
    /* Calculate total length */
    size_t total_len = 0;
    for (size_t i = 0; i < gl.gl_pathc; i++) {
        total_len += strlen(gl.gl_pathv[i]) + 1;  /* +1 for space */
    }
    
    /* Build result */
    char *result = malloc(total_len + 1);
    if (result == NULL) {
        globfree(&gl);
        return NULL;
    }
    
    char *p = result;
    for (size_t i = 0; i < gl.gl_pathc; i++) {
        size_t len = strlen(gl.gl_pathv[i]);
        memcpy(p, gl.gl_pathv[i], len);
        p += len;
        if (i < gl.gl_pathc - 1) {
            *p++ = ' ';
        }
    }
    *p = '\0';
    
    *count = gl.gl_pathc;
    globfree(&gl);
    return result;
}

/**
 * @brief Get current time in milliseconds
 */
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

/* ============================================================
 * LIFECYCLE
 * ============================================================ */

Ordinal* ordinal_new(void) {
    Ordinal *ord = calloc(1, sizeof(Ordinal));
    if (ord == NULL) return NULL;
    
    /* Create INI parser */
    ord->ini = zorya_ini_new();
    if (ord->ini == NULL) {
        free(ord);
        return NULL;
    }
    
    /* Create target map */
    ord->target_map = dagger_create(64, NULL);
    if (ord->target_map == NULL) {
        zorya_ini_free(ord->ini);
        free(ord);
        return NULL;
    }
    
    /* Allocate target array */
    ord->target_capacity = 32;
    ord->targets = calloc(ord->target_capacity, sizeof(OrdinalTargetInternal));
    if (ord->targets == NULL) {
        dagger_destroy(ord->target_map);
        zorya_ini_free(ord->ini);
        free(ord);
        return NULL;
    }
    
    /* Create WEAVE string interning table */
    ord->strings = tablet_create();
    if (ord->strings == NULL) {
        free(ord->targets);
        dagger_destroy(ord->target_map);
        zorya_ini_free(ord->ini);
        free(ord);
        return NULL;
    }
    
    /* Intern common section names for O(1) comparison */
    ord->sec_build = tablet_intern(ord->strings, "build");
    ord->sec_clean = tablet_intern(ord->strings, "clean");
    ord->sec_test = tablet_intern(ord->strings, "test");
    ord->sec_install = tablet_intern(ord->strings, "install");
    ord->sec_project = tablet_intern(ord->strings, "project");
    ord->sec_env = tablet_intern(ord->strings, "env");
    
    /* Default config */
    ord->config = (OrdinalConfig)ORDINAL_CONFIG_DEFAULT;
    
    /* Detect platform info */
    ord->var_platform = str_dup(ordinal_detect_platform());
    ord->var_arch = str_dup(ordinal_detect_arch());
    ord->var_nproc = ordinal_detect_nproc();
    
    /* Get current directory */
    char cwd[ORD_MAX_PATH_LENGTH];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        ord->cwd = str_dup(cwd);
    }
    
    return ord;
}

void ordinal_free(Ordinal *ord) {
    if (ord == NULL) return;
    
    /* Free targets */
    if (ord->targets != NULL) {
        for (size_t i = 0; i < ord->target_count; i++) {
            target_destroy(&ord->targets[i]);
        }
        free(ord->targets);
    }
    
    /* Free target map */
    if (ord->target_map != NULL) {
        dagger_destroy(ord->target_map);
    }
    
    /* Free WEAVE string table */
    if (ord->strings != NULL) {
        tablet_destroy(ord->strings);
    }
    
    /* Free INI */
    if (ord->ini != NULL) {
        zorya_ini_free(ord->ini);
    }
    
    /* Free strings */
    free(ord->default_target);
    free(ord->project_name);
    free(ord->project_version);
    free(ord->ordinal_dir);
    free(ord->cwd);
    free(ord->var_platform);
    free(ord->var_arch);
    
    free(ord);
}

static void target_destroy(OrdinalTargetInternal *target) {
    if (target == NULL) return;
    
    free(target->name);
    free(target->section);
    
    if (target->deps != NULL) {
        for (size_t i = 0; i < target->dep_count; i++) {
            free(target->deps[i]);
        }
        free(target->deps);
    }
    
    if (target->resolved_deps != NULL) {
        for (size_t i = 0; i < target->resolved_dep_count; i++) {
            free(target->resolved_deps[i]);
        }
        free(target->resolved_deps);
    }
    
    free(target->command);
    free(target->resolved_command);
    free(target->target_file);
}

/* ============================================================
 * CONFIGURATION
 * ============================================================ */

ordinal_error_t ordinal_configure(Ordinal *ord, const OrdinalConfig *config) {
    if (ord == NULL) return ORDINAL_ERROR_NULLPTR;
    if (config != NULL) {
        ord->config = *config;
    }
    if (ord->config.jobs <= 0) {
        ord->config.jobs = ord->var_nproc;
    }
    return ORDINAL_OK;
}

void ordinal_set_progress_callback(
    Ordinal *ord,
    ordinal_progress_fn callback,
    void *userdata
) {
    if (ord == NULL) return;
    ord->progress_fn = callback;
    ord->progress_userdata = userdata;
}

void ordinal_set_output_callback(
    Ordinal *ord,
    ordinal_output_fn callback,
    void *userdata
) {
    if (ord == NULL) return;
    ord->output_fn = callback;
    ord->output_userdata = userdata;
}

/* ============================================================
 * LOADING
 * ============================================================ */

ordinal_error_t ordinal_load(Ordinal *ord, const char *filepath) {
    if (ord == NULL) return ORDINAL_ERROR_NULLPTR;
    
    const char *path = filepath;
    char found_path[ORD_MAX_PATH_LENGTH];
    
    /* Auto-detect Ordinal file if not specified */
    if (path == NULL) {
        bool found = false;
        for (int i = 0; ORDINAL_FILENAMES[i] != NULL; i++) {
            if (access(ORDINAL_FILENAMES[i], R_OK) == 0) {
                snprintf(found_path, sizeof(found_path), "%s", ORDINAL_FILENAMES[i]);
                path = found_path;
                found = true;
                break;
            }
        }
        if (!found) {
            snprintf(ord->error_msg, sizeof(ord->error_msg),
                     "No Ordinal file found (tried: Ordinal, Ordinal.ini, ordinal, ordinal.ini)");
            return ORDINAL_ERROR_IO;
        }
    }
    
    /* Store directory for relative paths */
    char *last_slash = strrchr(path, '/');
    if (last_slash != NULL) {
        size_t dir_len = (size_t)(last_slash - path);
        ord->ordinal_dir = malloc(dir_len + 1);
        if (ord->ordinal_dir != NULL) {
            memcpy(ord->ordinal_dir, path, dir_len);
            ord->ordinal_dir[dir_len] = '\0';
        }
    } else {
        ord->ordinal_dir = str_dup(".");
    }
    
    /* Load INI file */
    zorya_ini_error_t err = zorya_ini_load(ord->ini, path);
    if (err != ZORYA_INI_OK) {
        snprintf(ord->error_msg, sizeof(ord->error_msg),
                 "Failed to load: %s", zorya_ini_strerror(err));
        return ORDINAL_ERROR_SYNTAX;
    }
    
    /* Extract project info */
    ord->project_name = str_dup(zorya_ini_get(ord->ini, "project.name"));
    ord->project_version = str_dup(zorya_ini_get(ord->ini, "project.version"));
    
    /* Discover targets from sections */
    ordinal_error_t result = discover_targets(ord);
    if (result != ORDINAL_OK) {
        return result;
    }
    
    if (ord->config.debug) {
        printf("[ordinal] Loaded %zu targets from %s\n", ord->target_count, path);
    }
    
    return ORDINAL_OK;
}

ordinal_error_t ordinal_load_buffer(Ordinal *ord, const char *data, size_t len) {
    if (ord == NULL || data == NULL) return ORDINAL_ERROR_NULLPTR;
    
    ord->ordinal_dir = str_dup(".");
    
    zorya_ini_error_t err = zorya_ini_load_buffer(ord->ini, data, len, "<buffer>");
    if (err != ZORYA_INI_OK) {
        snprintf(ord->error_msg, sizeof(ord->error_msg),
                 "Failed to parse Ordinal: %s", zorya_ini_strerror(err));
        return ORDINAL_ERROR_SYNTAX;
    }
    
    ord->project_name = str_dup(zorya_ini_get(ord->ini, "project.name"));
    ord->project_version = str_dup(zorya_ini_get(ord->ini, "project.version"));
    
    return discover_targets(ord);
}

/* ============================================================
 * TARGET DISCOVERY
 * ============================================================ */

/**
 * @brief Add a target to the list
 */
static ordinal_error_t add_target(
    Ordinal *ord,
    const char *name,
    const char *section
) {
    /* Grow array if needed */
    if (ord->target_count >= ord->target_capacity) {
        size_t new_cap = ord->target_capacity * 2;
        OrdinalTargetInternal *new_targets = realloc(
            ord->targets, 
            new_cap * sizeof(OrdinalTargetInternal)
        );
        if (new_targets == NULL) {
            return ORDINAL_ERROR_NOMEM;
        }
        ord->targets = new_targets;
        ord->target_capacity = new_cap;
    }
    
    OrdinalTargetInternal *t = &ord->targets[ord->target_count];
    memset(t, 0, sizeof(*t));
    
    t->name = str_dup(name);
    t->section = str_dup(section);
    t->status = ORDINAL_STATUS_PENDING;
    
    /* Get target file (output) */
    char key[256];
    snprintf(key, sizeof(key), "%s.target", section);
    const char *target_file = zorya_ini_get(ord->ini, key);
    if (target_file != NULL) {
        t->target_file = str_dup(target_file);
    }
    
    /* Get command */
    snprintf(key, sizeof(key), "%s.command", section);
    const char *cmd = zorya_ini_get(ord->ini, key);
    if (cmd != NULL) {
        t->command = str_dup(cmd);
    }
    
    /* Get dependencies */
    snprintf(key, sizeof(key), "%s.deps", section);
    size_t dep_count = 0;
    const char **deps = zorya_ini_get_array(ord->ini, key, &dep_count);
    if (deps != NULL && dep_count > 0) {
        t->deps = calloc(dep_count, sizeof(char*));
        if (t->deps != NULL) {
            for (size_t i = 0; i < dep_count; i++) {
                t->deps[i] = str_dup(deps[i]);
            }
            t->dep_count = dep_count;
        }
    } else {
        /* Try single-value deps */
        const char *single_dep = zorya_ini_get(ord->ini, key);
        if (single_dep != NULL && *single_dep != '\0') {
            t->deps = calloc(1, sizeof(char*));
            if (t->deps != NULL) {
                t->deps[0] = str_dup(single_dep);
                t->dep_count = 1;
            }
        }
    }
    
    /* Determine if phony (no target file and no glob deps) */
    t->is_phony = (t->target_file == NULL);
    
    /* Add to map */
    size_t index = ord->target_count;
    dagger_set(ord->target_map, t->name, (uint32_t)strlen(t->name), 
               (void*)(uintptr_t)(index + 1), 1);  /* +1 to avoid NULL, replace=1 */
    
    ord->target_count++;
    
    /* First build target is default */
    if (ord->default_target == NULL && 
        (strcmp(name, "build") == 0 || strncmp(section, "build", 5) == 0)) {
        ord->default_target = str_dup(name);
    }
    
    return ORDINAL_OK;
}

/**
 * @brief Discover all targets from INI sections
 */
static ordinal_error_t discover_targets(Ordinal *ord) {
    size_t section_count = 0;
    const char **sections = zorya_ini_sections(ord->ini, &section_count);
    if (sections == NULL) {
        return ORDINAL_OK;  /* Empty file */
    }
    
    for (size_t i = 0; i < section_count; i++) {
        const char *section = sections[i];
        
        /* Intern section name for fast comparison */
        const Weave *sec_weave = tablet_intern(ord->strings, section);
        
        /* Skip non-target sections using pointer comparison */
        if (sec_weave == ord->sec_project ||
            sec_weave == ord->sec_env ||
            strncmp(section, "project.", 8) == 0 ||
            strncmp(section, "env.", 4) == 0) {
            continue;
        }
        
        /* Extract target name from section */
        /* "build" -> "build" */
        /* "build:debug" -> "debug" */
        /* "clean" -> "clean" */
        const char *name = section;
        const char *colon = strchr(section, ':');
        if (colon != NULL) {
            name = colon + 1;  /* Use part after colon */
        }
        
        ordinal_error_t err = add_target(ord, name, section);
        if (err != ORDINAL_OK) {
            return err;
        }
    }
    
    /* If no default target, use first one */
    if (ord->default_target == NULL && ord->target_count > 0) {
        ord->default_target = str_dup(ord->targets[0].name);
    }
    
    return ORDINAL_OK;
}

/* ============================================================
 * VARIABLE RESOLUTION
 * ============================================================ */

/* NOTE: Variable interpolation is now handled by zorya_ini at parse time.
 * Ordinal only resolves runtime variables (${_...} prefix) at execution time.
 * This clean separation means:
 *   - zorya_ini: config-time vars (${cc}, ${build_dir}, ${project.name})
 *   - ordinal:   runtime vars (${_target}, ${_deps}, ${_platform})
 */

/**
 * @brief Resolve runtime variables in a string
 *
 * This ONLY handles runtime variables (${_...} prefix) that were
 * preserved by zorya_ini. Config-time variables are already resolved.
 * 
 * Runtime variables:
 *   ${_target}    - Output file path
 *   ${_first_dep} - First dependency
 *   ${_all_deps}  - All dependencies (space-separated)
 *   ${_platform}  - linux/darwin/windows
 *   ${_arch}      - x86_64/aarch64/etc
 *   ${_nproc}     - Number of CPU cores
 */
static char* resolve_runtime_vars(Ordinal *ord, const char *str, OrdinalTargetInternal *target) {
    if (str == NULL) return NULL;
    if (strchr(str, '$') == NULL) return str_dup(str);
    
    /* Use simple string replacement for runtime vars */
    char *result = str_dup(str);
    if (result == NULL) return NULL;
    
    /* Helper macro for replacement */
    #define REPLACE_VAR(pattern, value) do { \
        if (value != NULL && strstr(result, pattern) != NULL) { \
            char *old = result; \
            size_t pat_len = strlen(pattern); \
            size_t val_len = strlen(value); \
            size_t count = 0; \
            for (char *p = result; (p = strstr(p, pattern)) != NULL; p += pat_len) count++; \
            if (count > 0) { \
                size_t new_len = strlen(result) - (count * pat_len) + (count * val_len) + 1; \
                char *new_str = malloc(new_len); \
                if (new_str != NULL) { \
                    char *dst = new_str; \
                    char *src = old; \
                    char *pos; \
                    while ((pos = strstr(src, pattern)) != NULL) { \
                        size_t chunk = (size_t)(pos - src); \
                        memcpy(dst, src, chunk); \
                        dst += chunk; \
                        memcpy(dst, value, val_len); \
                        dst += val_len; \
                        src = pos + pat_len; \
                    } \
                    strcpy(dst, src); \
                    result = new_str; \
                    free(old); \
                } \
            } \
        } \
    } while(0)
    
    /* Target-specific runtime vars */
    if (target != NULL) {
        const char *target_val = target->target_file ? target->target_file : target->name;
        REPLACE_VAR("${_target}", target_val);
        
        if (target->resolved_deps != NULL && target->resolved_dep_count > 0) {
            REPLACE_VAR("${_first_dep}", target->resolved_deps[0]);
            
            /* Build ${_all_deps} - join with spaces */
            if (strstr(result, "${_all_deps}") != NULL) {
                size_t total = 0;
                for (size_t i = 0; i < target->resolved_dep_count; i++) {
                    total += strlen(target->resolved_deps[i]) + 1;
                }
                char *all_deps = malloc(total);
                if (all_deps != NULL) {
                    char *p = all_deps;
                    for (size_t i = 0; i < target->resolved_dep_count; i++) {
                        size_t len = strlen(target->resolved_deps[i]);
                        memcpy(p, target->resolved_deps[i], len);
                        p += len;
                        *p++ = (i < target->resolved_dep_count - 1) ? ' ' : '\0';
                    }
                    REPLACE_VAR("${_all_deps}", all_deps);
                    free(all_deps);
                }
            }
        }
    }
    
    /* Global runtime vars */
    REPLACE_VAR("${_platform}", ord->var_platform);
    REPLACE_VAR("${_arch}", ord->var_arch);
    
    /* ${_nproc} needs conversion */
    if (strstr(result, "${_nproc}") != NULL) {
        char nproc_str[32];
        snprintf(nproc_str, sizeof(nproc_str), "%d", ord->var_nproc);
        REPLACE_VAR("${_nproc}", nproc_str);
    }
    
    if (ord->cwd != NULL) {
        REPLACE_VAR("${_cwd}", ord->cwd);
    } else {
        REPLACE_VAR("${_cwd}", ".");
    }
    
    if (ord->ordinal_dir != NULL) {
        REPLACE_VAR("${_ordinal_dir}", ord->ordinal_dir);
    } else {
        REPLACE_VAR("${_ordinal_dir}", ".");
    }
    
    #undef REPLACE_VAR
    
    return result;
}

/* ============================================================
 * DEPENDENCY RESOLUTION
 * ============================================================ */

static ordinal_error_t resolve_target_deps(Ordinal *ord, OrdinalTargetInternal *target) {
    if (target->dep_count == 0) return ORDINAL_OK;
    
    /* Free old resolved deps */
    if (target->resolved_deps != NULL) {
        for (size_t i = 0; i < target->resolved_dep_count; i++) {
            free(target->resolved_deps[i]);
        }
        free(target->resolved_deps);
        target->resolved_deps = NULL;
        target->resolved_dep_count = 0;
    }
    
    /* Temporary list */
    char **resolved = calloc(ORD_MAX_DEPS, sizeof(char*));
    if (resolved == NULL) return ORDINAL_ERROR_NOMEM;
    size_t resolved_count = 0;
    
    for (size_t i = 0; i < target->dep_count && resolved_count < ORD_MAX_DEPS; i++) {
        /* Resolve variables in dep */
        char *dep = resolve_runtime_vars(ord, target->deps[i], target);
        if (dep == NULL) continue;
        
        /* Check if it's a glob pattern */
        if (strchr(dep, '*') != NULL || strchr(dep, '?') != NULL) {
            size_t glob_count = 0;
            char *expanded = expand_glob(dep, &glob_count);
            if (expanded != NULL && *expanded != '\0') {
                /* Split by spaces */
                char *p = expanded;
                while (*p != '\0' && resolved_count < ORD_MAX_DEPS) {
                    while (*p == ' ') p++;
                    if (*p == '\0') break;
                    char *start = p;
                    while (*p != ' ' && *p != '\0') p++;
                    size_t len = (size_t)(p - start);
                    resolved[resolved_count] = malloc(len + 1);
                    if (resolved[resolved_count] != NULL) {
                        memcpy(resolved[resolved_count], start, len);
                        resolved[resolved_count][len] = '\0';
                        resolved_count++;
                    }
                }
            }
            free(expanded);
        } else {
            /* Check if it's another target name */
            void *idx = NULL;
            dagger_get(ord->target_map, dep, (uint32_t)strlen(dep), &idx);
            if (idx != NULL) {
                /* It's a target dependency */
                resolved[resolved_count++] = dep;
                dep = NULL;  /* Don't free */
            } else {
                /* Assume it's a file path */
                resolved[resolved_count++] = dep;
                dep = NULL;
            }
        }
        
        free(dep);
    }
    
    /* Shrink to fit */
    if (resolved_count > 0) {
        char **shrunk = realloc(resolved, resolved_count * sizeof(char*));
        target->resolved_deps = (shrunk != NULL) ? shrunk : resolved;
        target->resolved_dep_count = resolved_count;
    } else {
        free(resolved);
    }
    
    return ORDINAL_OK;
}

static ordinal_error_t resolve_command(Ordinal *ord, OrdinalTargetInternal *target) {
    if (target->command == NULL) return ORDINAL_OK;
    
    free(target->resolved_command);
    
    /* First resolve the target file path */
    if (target->target_file != NULL) {
        char *resolved_target = resolve_runtime_vars(ord, target->target_file, target);
        free(target->target_file);
        target->target_file = resolved_target;
    }
    
    /* Then resolve command */
    target->resolved_command = resolve_runtime_vars(ord, target->command, target);
    
    return ORDINAL_OK;
}

/* ============================================================
 * BUILD LOGIC
 * ============================================================ */

static bool needs_rebuild(Ordinal *ord, OrdinalTargetInternal *target) {
    /* Force rebuild? */
    if (ord->config.force) return true;
    
    /* Phony targets always rebuild */
    if (target->is_phony) return true;
    
    /* No target file means always rebuild */
    if (target->target_file == NULL) return true;
    
    /* Check if target exists */
    time_t target_mtime = get_mtime(target->target_file);
    if (target_mtime == 0) return true;  /* Target doesn't exist */
    
    /* Check if any dep is newer */
    for (size_t i = 0; i < target->resolved_dep_count; i++) {
        const char *dep = target->resolved_deps[i];
        
        /* Check if dep is a target */
        void *idx = NULL;
        dagger_get(ord->target_map, dep, (uint32_t)strlen(dep), &idx);
        if (idx != NULL) {
            /* Dependency is a target - check its status */
            size_t target_idx = (size_t)(uintptr_t)idx - 1;
            if (target_idx < ord->target_count) {
                OrdinalTargetInternal *dep_target = &ord->targets[target_idx];
                if (dep_target->status == ORDINAL_STATUS_REBUILT) {
                    return true;  /* Dep was rebuilt */
                }
            }
            continue;
        }
        
        /* Check file mtime */
        time_t dep_mtime = get_mtime(dep);
        if (dep_mtime > target_mtime) {
            if (ord->config.debug) {
                printf("[ordinal] %s: %s is newer than target\n", target->name, dep);
            }
            return true;
        }
    }
    
    return false;
}

static ordinal_error_t execute_command(Ordinal *ord, OrdinalTargetInternal *target) {
    if (target->resolved_command == NULL || *target->resolved_command == '\0') {
        return ORDINAL_OK;  /* No command to execute */
    }
    
    /* Print command if verbose or not silent */
    if (ord->config.verbose || !ord->config.silent) {
        printf("  %s\n", target->resolved_command);
    }
    
    /* Dry run - don't actually execute */
    if (ord->config.dry_run) {
        target->status = ORDINAL_STATUS_SKIPPED;
        return ORDINAL_OK;
    }
    
    /* Execute command */
    double start = get_time_ms();
    
    int ret = system(target->resolved_command);
    
    target->build_time_ms = get_time_ms() - start;
    
    if (ret != 0) {
        int exit_code = WEXITSTATUS(ret);
        snprintf(ord->error_msg, sizeof(ord->error_msg),
                 "Command failed with exit code %d: %s",
                 exit_code, target->resolved_command);
        target->status = ORDINAL_STATUS_FAILED;
        return ORDINAL_ERROR_COMMAND;
    }
    
    target->status = ORDINAL_STATUS_REBUILT;
    return ORDINAL_OK;
}

static ordinal_error_t build_target(Ordinal *ord, const char *name, int depth) {
    if (depth > ORD_MAX_RECURSION) {
        snprintf(ord->error_msg, sizeof(ord->error_msg),
                 "Maximum recursion depth exceeded (circular dependency?)");
        return ORDINAL_ERROR_CIRCULAR;
    }
    
    /* Find target */
    void *idx_ptr = NULL;
    dagger_get(ord->target_map, name, (uint32_t)strlen(name), &idx_ptr);
    if (idx_ptr == NULL) {
        /* Not a target - might be a file dependency, that's OK */
        return ORDINAL_OK;
    }
    
    size_t target_idx = (size_t)(uintptr_t)idx_ptr - 1;
    if (target_idx >= ord->target_count) {
        return ORDINAL_ERROR_NO_TARGET;
    }
    
    OrdinalTargetInternal *target = &ord->targets[target_idx];
    
    /* Cycle detection */
    if (target->in_stack) {
        snprintf(ord->error_msg, sizeof(ord->error_msg),
                 "Circular dependency detected: %s", name);
        return ORDINAL_ERROR_CIRCULAR;
    }
    
    /* Already processed? */
    if (target->visited) {
        return ORDINAL_OK;
    }
    
    target->in_stack = true;
    
    /* Progress callback */
    if (ord->progress_fn != NULL) {
        ord->progress_fn(name, ORDINAL_STATUS_PENDING, NULL, ord->progress_userdata);
    }
    
    /* Resolve dependencies */
    ordinal_error_t err = resolve_target_deps(ord, target);
    if (err != ORDINAL_OK) {
        target->in_stack = false;
        return err;
    }
    
    /* Build dependencies first */
    for (size_t i = 0; i < target->resolved_dep_count; i++) {
        const char *dep = target->resolved_deps[i];
        
        /* Check if it's a target */
        void *dep_idx = NULL;
        dagger_get(ord->target_map, dep, (uint32_t)strlen(dep), &dep_idx);
        if (dep_idx != NULL) {
            err = build_target(ord, dep, depth + 1);
            if (err != ORDINAL_OK && !ord->config.keep_going) {
                target->in_stack = false;
                target->status = ORDINAL_STATUS_FAILED;
                return err;
            }
        }
    }
    
    /* Resolve command (after deps are built) */
    err = resolve_command(ord, target);
    if (err != ORDINAL_OK) {
        target->in_stack = false;
        return err;
    }
    
    /* Check if rebuild needed */
    if (!needs_rebuild(ord, target)) {
        target->status = ORDINAL_STATUS_UP_TO_DATE;
        target->visited = true;
        target->in_stack = false;
        ord->result.targets_up_to_date++;
        
        if (ord->progress_fn != NULL) {
            ord->progress_fn(name, ORDINAL_STATUS_UP_TO_DATE, "up to date", ord->progress_userdata);
        }
        if (!ord->config.silent && ord->config.verbose) {
            printf("  [up-to-date] %s\n", name);
        }
        return ORDINAL_OK;
    }
    
    /* Progress callback */
    if (ord->progress_fn != NULL) {
        ord->progress_fn(name, ORDINAL_STATUS_BUILDING, NULL, ord->progress_userdata);
    }
    
    /* Execute build command */
    err = execute_command(ord, target);
    
    target->visited = true;
    target->in_stack = false;
    ord->result.targets_total++;
    
    if (err != ORDINAL_OK) {
        ord->result.targets_failed++;
        if (ord->progress_fn != NULL) {
            ord->progress_fn(name, ORDINAL_STATUS_FAILED, ord->error_msg, ord->progress_userdata);
        }
        return err;
    }
    
    if (target->status == ORDINAL_STATUS_REBUILT) {
        ord->result.targets_rebuilt++;
    } else if (target->status == ORDINAL_STATUS_SKIPPED) {
        ord->result.targets_skipped++;
    }
    
    if (ord->progress_fn != NULL) {
        ord->progress_fn(name, target->status, NULL, ord->progress_userdata);
    }
    
    return ORDINAL_OK;
}

/* ============================================================
 * EXECUTION API
 * ============================================================ */

ordinal_error_t ordinal_run(Ordinal *ord, const char *target) {
    if (ord == NULL) return ORDINAL_ERROR_NULLPTR;
    
    const char *target_name = target;
    if (target_name == NULL) {
        target_name = ord->default_target;
    }
    if (target_name == NULL && ord->target_count > 0) {
        target_name = ord->targets[0].name;
    }
    if (target_name == NULL) {
        snprintf(ord->error_msg, sizeof(ord->error_msg), "No target specified and no default target");
        return ORDINAL_ERROR_NO_TARGET;
    }
    
    /* Verify target exists before running */
    void *idx_ptr = NULL;
    dagger_get(ord->target_map, target_name, (uint32_t)strlen(target_name), &idx_ptr);
    if (idx_ptr == NULL) {
        snprintf(ord->error_msg, sizeof(ord->error_msg), "Target not found: %s", target_name);
        return ORDINAL_ERROR_NO_TARGET;
    }
    
    /* Reset state */
    memset(&ord->result, 0, sizeof(ord->result));
    for (size_t i = 0; i < ord->target_count; i++) {
        ord->targets[i].visited = false;
        ord->targets[i].in_stack = false;
        ord->targets[i].status = ORDINAL_STATUS_PENDING;
    }
    
    double start = get_time_ms();
    
    /* Change directory if requested */
    char *old_cwd = NULL;
    if (ord->config.directory != NULL) {
        char cwd[ORD_MAX_PATH_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            old_cwd = str_dup(cwd);
        }
        if (chdir(ord->config.directory) != 0) {
            snprintf(ord->error_msg, sizeof(ord->error_msg),
                     "Failed to change to directory: %s", ord->config.directory);
            free(old_cwd);
            return ORDINAL_ERROR_IO;
        }
    }
    
    /* Build */
    ordinal_error_t err = build_target(ord, target_name, 0);
    
    /* Restore directory */
    if (old_cwd != NULL) {
        chdir(old_cwd);
        free(old_cwd);
    }
    
    ord->result.total_time_ms = get_time_ms() - start;
    ord->result.success = (err == ORDINAL_OK);
    
    return err;
}

ordinal_error_t ordinal_run_many(
    Ordinal *ord,
    const char **targets,
    size_t count
) {
    if (ord == NULL || targets == NULL) return ORDINAL_ERROR_NULLPTR;
    
    ordinal_error_t last_err = ORDINAL_OK;
    for (size_t i = 0; i < count; i++) {
        ordinal_error_t err = ordinal_run(ord, targets[i]);
        if (err != ORDINAL_OK) {
            last_err = err;
            if (!ord->config.keep_going) {
                return err;
            }
        }
    }
    return last_err;
}

void ordinal_get_result(const Ordinal *ord, OrdinalResult *result) {
    if (ord == NULL || result == NULL) return;
    *result = ord->result;
}

/* ============================================================
 * INSPECTION
 * ============================================================ */

const char** ordinal_list_targets(const Ordinal *ord, size_t *count) {
    if (ord == NULL || count == NULL) return NULL;
    
    static const char *targets[ORD_MAX_TARGETS];
    size_t n = 0;
    
    for (size_t i = 0; i < ord->target_count && n < ORD_MAX_TARGETS; i++) {
        targets[n++] = ord->targets[i].name;
    }
    
    *count = n;
    return targets;
}

ordinal_error_t ordinal_get_target(
    const Ordinal *ord,
    const char *name,
    OrdinalTarget *target
) {
    if (ord == NULL || name == NULL || target == NULL) {
        return ORDINAL_ERROR_NULLPTR;
    }
    
    void *idx_ptr = NULL;
    dagger_get(ord->target_map, name, (uint32_t)strlen(name), &idx_ptr);
    if (idx_ptr == NULL) {
        return ORDINAL_ERROR_NO_TARGET;
    }
    
    size_t idx = (size_t)(uintptr_t)idx_ptr - 1;
    if (idx >= ord->target_count) {
        return ORDINAL_ERROR_NO_TARGET;
    }
    
    OrdinalTargetInternal *t = &ord->targets[idx];
    target->name = t->name;
    target->section = t->section;
    /* Return resolved deps if available, otherwise raw deps */
    if (t->resolved_deps != NULL && t->resolved_dep_count > 0) {
        target->deps = (const char**)t->resolved_deps;
        target->dep_count = t->resolved_dep_count;
    } else {
        target->deps = (const char**)t->deps;
        target->dep_count = t->dep_count;
    }
    target->command = t->resolved_command ? t->resolved_command : t->command;
    target->status = t->status;
    target->build_time_ms = t->build_time_ms;
    
    return ORDINAL_OK;
}

const char* ordinal_get_project_name(const Ordinal *ord) {
    return ord ? ord->project_name : NULL;
}

const char* ordinal_get_project_version(const Ordinal *ord) {
    return ord ? ord->project_version : NULL;
}

const char* ordinal_get_var(const Ordinal *ord, const char *key) {
    if (ord == NULL || key == NULL) return NULL;
    
    /* Try direct lookup first */
    const char *val = zorya_ini_get(ord->ini, key);
    if (val != NULL) return val;
    
    /* Try with env. prefix (for [env] section values) */
    char env_key[280];
    snprintf(env_key, sizeof(env_key), "env.%s", key);
    val = zorya_ini_get(ord->ini, env_key);
    if (val != NULL) return val;
    
    /* Try with project. prefix */
    char proj_key[280];
    snprintf(proj_key, sizeof(proj_key), "project.%s", key);
    return zorya_ini_get(ord->ini, proj_key);
}

/* ============================================================
 * ERROR HANDLING
 * ============================================================ */

const char* ordinal_strerror(ordinal_error_t err) {
    switch (err) {
        case ORDINAL_OK:            return "Success";
        case ORDINAL_ERROR_NULLPTR: return "Null pointer";
        case ORDINAL_ERROR_NOMEM:   return "Out of memory";
        case ORDINAL_ERROR_IO:      return "I/O error";
        case ORDINAL_ERROR_SYNTAX:  return "Syntax error";
        case ORDINAL_ERROR_NO_TARGET: return "Target not found";
        case ORDINAL_ERROR_CIRCULAR: return "Circular dependency";
        case ORDINAL_ERROR_COMMAND: return "Command failed";
        case ORDINAL_ERROR_DEP:     return "Dependency error";
        case ORDINAL_ERROR_GLOB:    return "Glob pattern error";
        default:                    return "Unknown error";
    }
}

const char* ordinal_last_error(const Ordinal *ord) {
    if (ord == NULL) return "NULL ordinal context";
    return ord->error_msg[0] ? ord->error_msg : "No error";
}

/* ============================================================
 * UTILITIES
 * ============================================================ */

void ordinal_print_deps(const Ordinal *ord, const char *target) {
    if (ord == NULL) return;
    
    printf("Dependency tree:\n");
    
    size_t start = 0, end = ord->target_count;
    if (target != NULL) {
        void *idx = NULL;
        dagger_get(ord->target_map, target, (uint32_t)strlen(target), &idx);
        if (idx != NULL) {
            start = (size_t)(uintptr_t)idx - 1;
            end = start + 1;
        }
    }
    
    for (size_t i = start; i < end; i++) {
        OrdinalTargetInternal *t = &ord->targets[i];
        printf("  %s:\n", t->name);
        if (t->dep_count > 0) {
            for (size_t j = 0; j < t->dep_count; j++) {
                printf("    - %s\n", t->deps[j]);
            }
        } else {
            printf("    (no dependencies)\n");
        }
    }
}

void ordinal_print_summary(const Ordinal *ord) {
    if (ord == NULL) return;
    
    printf("\n");
    printf("Build Summary:\n");
    printf("  Targets total:     %zu\n", ord->result.targets_total);
    printf("  Targets rebuilt:   %zu\n", ord->result.targets_rebuilt);
    printf("  Targets up-to-date:%zu\n", ord->result.targets_up_to_date);
    printf("  Targets failed:    %zu\n", ord->result.targets_failed);
    printf("  Targets skipped:   %zu\n", ord->result.targets_skipped);
    printf("  Total time:        %.2f ms\n", ord->result.total_time_ms);
    printf("  Status:            %s\n", ord->result.success ? "SUCCESS" : "FAILED");
}
