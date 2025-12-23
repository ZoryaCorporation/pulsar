/**
 * @file ordinal.h
 * @brief Ordinal Build System - Makefile Replacement Built on ZORYA-INI
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
 * ============================================================================
 * PHILOSOPHY
 * ============================================================================
 *
 * Same dependency model as Make. Less bullshit.
 *
 * - No tabs required
 * - No cryptic sigils ($@, $<, $^)
 * - Named variables with clear interpolation
 * - Section-based organization
 * - Parallel execution support
 * - Human-readable, AI-parseable
 *
 * ============================================================================
 * ORDINAL FILE FORMAT
 * ============================================================================
 *
 * ```ini
 * # Ordinal - replaces Makefile
 *
 * [project]
 * name = my_project
 * version = 1.0.0
 *
 * [env]
 * cc = ${env:CC:-gcc}
 * cflags = -std=c99 | -Wall | -Wextra
 *
 * [build]
 * target = bin/${name}
 * deps = src/[asterisk].c
 * command = ${cc} ${cflags} -o ${target} ${deps}
 *
 * [clean]
 * command = rm -rf build/ bin/
 *
 * [test]
 * deps = build
 * command = ./bin/test_${name}
 * ```
 *
 * ============================================================================
 * SECTION TYPES
 * ============================================================================
 *
 * [project]  - Project metadata (name, version)
 * [env]      - Environment/variables
 * [build]    - Default build target
 * [target]   - Named build target (e.g., [build:debug], [build:release])
 * [clean]    - Cleanup target
 * [test]     - Test target
 * [install]  - Installation target
 * [*]        - Any other section is a custom target
 *
 * ============================================================================
 * RESERVED VARIABLES
 * ============================================================================
 *
 * ${_target}      - Current target being built (replaces $@)
 * ${_first_dep}   - First dependency (replaces $<)
 * ${_all_deps}    - All dependencies (replaces $^)
 * ${_platform}    - Platform: linux, darwin, windows
 * ${_arch}        - Architecture: x86_64, aarch64, arm, etc.
 * ${_nproc}       - Number of CPU cores
 * ${_cwd}         - Current working directory
 * ${_ordinal_dir} - Directory containing Ordinal file
 *
 * ============================================================================
 * USAGE
 * ============================================================================
 *
 * ```c
 * #include <zorya/ordinal.h>
 *
 * Ordinal *ord = ordinal_new();
 * ordinal_load(ord, "Ordinal");
 * ordinal_run(ord, "build");  // Run [build] target
 * ordinal_free(ord);
 * ```
 *
 * CLI:
 *   ordinal              # Run default target
 *   ordinal clean        # Run [clean] target
 *   ordinal -j8 build    # Parallel build with 8 jobs
 *   ordinal -v test      # Verbose mode
 *   ordinal -n build     # Dry-run (show commands without executing)
 */

#ifndef ORDINAL_H_
#define ORDINAL_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * VERSION
 * ============================================================ */

#define ORDINAL_VERSION_MAJOR 0
#define ORDINAL_VERSION_MINOR 1
#define ORDINAL_VERSION_PATCH 0
#define ORDINAL_VERSION_STRING "0.1.0"

/* ============================================================
 * ERROR CODES
 * ============================================================ */

typedef enum {
    ORDINAL_OK = 0,                /**< Success */
    ORDINAL_ERROR_NULLPTR = -1,    /**< NULL pointer argument */
    ORDINAL_ERROR_NOMEM = -2,      /**< Memory allocation failed */
    ORDINAL_ERROR_IO = -3,         /**< File I/O error */
    ORDINAL_ERROR_SYNTAX = -4,     /**< Syntax error in Ordinal file */
    ORDINAL_ERROR_NO_TARGET = -5,  /**< Target not found */
    ORDINAL_ERROR_CIRCULAR = -6,   /**< Circular dependency detected */
    ORDINAL_ERROR_COMMAND = -7,    /**< Command execution failed */
    ORDINAL_ERROR_DEP = -8,        /**< Dependency resolution failed */
    ORDINAL_ERROR_GLOB = -9,       /**< Glob pattern error */
} ordinal_error_t;

/* ============================================================
 * BUILD STATUS
 * ============================================================ */

typedef enum {
    ORDINAL_STATUS_PENDING = 0,    /**< Not yet processed */
    ORDINAL_STATUS_BUILDING,       /**< Currently building */
    ORDINAL_STATUS_UP_TO_DATE,     /**< Already up to date */
    ORDINAL_STATUS_REBUILT,        /**< Successfully rebuilt */
    ORDINAL_STATUS_FAILED,         /**< Build failed */
    ORDINAL_STATUS_SKIPPED,        /**< Skipped (dry-run or condition) */
} ordinal_status_t;

/* ============================================================
 * CONFIGURATION
 * ============================================================ */

/**
 * @brief Build configuration options
 */
typedef struct {
    int jobs;                      /**< Parallel jobs (0 = auto-detect) */
    bool verbose;                  /**< Print commands before executing */
    bool dry_run;                  /**< Show commands without executing */
    bool keep_going;               /**< Continue on errors */
    bool silent;                   /**< Suppress output */
    bool force;                    /**< Force rebuild all targets */
    bool debug;                    /**< Debug output */
    const char *directory;         /**< Change to directory before build */
} OrdinalConfig;

/**
 * @brief Default configuration
 */
#define ORDINAL_CONFIG_DEFAULT { \
    .jobs = 1,                    \
    .verbose = false,             \
    .dry_run = false,             \
    .keep_going = false,          \
    .silent = false,              \
    .force = false,               \
    .debug = false,               \
    .directory = NULL             \
}

/* ============================================================
 * CALLBACK TYPES
 * ============================================================ */

/**
 * @brief Progress callback for build status
 *
 * @param target      Target name being processed
 * @param status      Current status
 * @param message     Status message (may be NULL)
 * @param userdata    User context
 */
typedef void (*ordinal_progress_fn)(
    const char *target,
    ordinal_status_t status,
    const char *message,
    void *userdata
);

/**
 * @brief Command output callback
 *
 * @param target      Target being built
 * @param output      Command output line
 * @param is_stderr   True if from stderr
 * @param userdata    User context
 */
typedef void (*ordinal_output_fn)(
    const char *target,
    const char *output,
    bool is_stderr,
    void *userdata
);

/* ============================================================
 * DATA STRUCTURES
 * ============================================================ */

/**
 * @brief Forward declaration
 */
typedef struct Ordinal Ordinal;

/**
 * @brief Target information (read-only view)
 */
typedef struct {
    const char *name;              /**< Target name */
    const char *section;           /**< INI section name */
    const char **deps;             /**< Dependency list */
    size_t dep_count;              /**< Number of dependencies */
    const char *command;           /**< Command to execute */
    ordinal_status_t status;       /**< Current status */
    double build_time_ms;          /**< Build time in milliseconds */
} OrdinalTarget;

/**
 * @brief Build result summary
 */
typedef struct {
    size_t targets_total;          /**< Total targets processed */
    size_t targets_rebuilt;        /**< Targets that were rebuilt */
    size_t targets_up_to_date;     /**< Targets already up to date */
    size_t targets_failed;         /**< Targets that failed */
    size_t targets_skipped;        /**< Targets skipped */
    double total_time_ms;          /**< Total build time */
    bool success;                  /**< Overall success */
} OrdinalResult;

/* ============================================================
 * LIFECYCLE
 * ============================================================ */

/**
 * @brief Create new Ordinal context
 *
 * @return New context or NULL on failure
 */
Ordinal* ordinal_new(void);

/**
 * @brief Free Ordinal context
 *
 * @param ord Context to free (NULL safe)
 */
void ordinal_free(Ordinal *ord);

/* ============================================================
 * CONFIGURATION
 * ============================================================ */

/**
 * @brief Set build configuration
 *
 * @param ord    Ordinal context
 * @param config Configuration to apply
 * @return ORDINAL_OK on success
 */
ordinal_error_t ordinal_configure(Ordinal *ord, const OrdinalConfig *config);

/**
 * @brief Set progress callback
 *
 * @param ord       Ordinal context
 * @param callback  Progress callback function
 * @param userdata  User context passed to callback
 */
void ordinal_set_progress_callback(
    Ordinal *ord,
    ordinal_progress_fn callback,
    void *userdata
);

/**
 * @brief Set output callback
 *
 * @param ord       Ordinal context
 * @param callback  Output callback function
 * @param userdata  User context passed to callback
 */
void ordinal_set_output_callback(
    Ordinal *ord,
    ordinal_output_fn callback,
    void *userdata
);

/* ============================================================
 * LOADING
 * ============================================================ */

/**
 * @brief Load Ordinal file
 *
 * Searches for: Ordinal, Ordinal.ini, ordinal, ordinal.ini
 * in current directory or specified path.
 *
 * @param ord      Ordinal context
 * @param filepath Path to Ordinal file (NULL for auto-detect)
 * @return ORDINAL_OK on success
 */
ordinal_error_t ordinal_load(Ordinal *ord, const char *filepath);

/**
 * @brief Load Ordinal from string buffer
 *
 * @param ord  Ordinal context
 * @param data Buffer containing Ordinal text
 * @param len  Buffer length
 * @return ORDINAL_OK on success
 */
ordinal_error_t ordinal_load_buffer(Ordinal *ord, const char *data, size_t len);

/* ============================================================
 * EXECUTION
 * ============================================================ */

/**
 * @brief Run a target
 *
 * Builds the specified target and all its dependencies.
 * If target is NULL, runs the default target (first [build] section).
 *
 * @param ord    Ordinal context
 * @param target Target name (NULL for default)
 * @return ORDINAL_OK on success
 */
ordinal_error_t ordinal_run(Ordinal *ord, const char *target);

/**
 * @brief Run multiple targets
 *
 * @param ord     Ordinal context
 * @param targets Array of target names
 * @param count   Number of targets
 * @return ORDINAL_OK on success
 */
ordinal_error_t ordinal_run_many(
    Ordinal *ord,
    const char **targets,
    size_t count
);

/**
 * @brief Get build result after run
 *
 * @param ord    Ordinal context
 * @param result Output: build result
 */
void ordinal_get_result(const Ordinal *ord, OrdinalResult *result);

/* ============================================================
 * INSPECTION
 * ============================================================ */

/**
 * @brief Get list of available targets
 *
 * @param ord   Ordinal context
 * @param count Output: number of targets
 * @return Array of target names (do not free)
 */
const char** ordinal_list_targets(const Ordinal *ord, size_t *count);

/**
 * @brief Get target information
 *
 * @param ord    Ordinal context
 * @param name   Target name
 * @param target Output: target info
 * @return ORDINAL_OK if found
 */
ordinal_error_t ordinal_get_target(
    const Ordinal *ord,
    const char *name,
    OrdinalTarget *target
);

/**
 * @brief Get project name
 *
 * @param ord Ordinal context
 * @return Project name or NULL
 */
const char* ordinal_get_project_name(const Ordinal *ord);

/**
 * @brief Get project version
 *
 * @param ord Ordinal context
 * @return Project version or NULL
 */
const char* ordinal_get_project_version(const Ordinal *ord);

/**
 * @brief Get variable value
 *
 * @param ord Ordinal context
 * @param key Variable name (supports ${} syntax)
 * @return Variable value or NULL
 */
const char* ordinal_get_var(const Ordinal *ord, const char *key);

/* ============================================================
 * ERROR HANDLING
 * ============================================================ */

/**
 * @brief Get error message for error code
 *
 * @param err Error code
 * @return Static error message
 */
const char* ordinal_strerror(ordinal_error_t err);

/**
 * @brief Get last error message with details
 *
 * @param ord Ordinal context
 * @return Error message (valid until next operation)
 */
const char* ordinal_last_error(const Ordinal *ord);

/* ============================================================
 * UTILITIES
 * ============================================================ */

/**
 * @brief Print target dependency tree
 *
 * @param ord    Ordinal context
 * @param target Target name (NULL for all)
 */
void ordinal_print_deps(const Ordinal *ord, const char *target);

/**
 * @brief Print build summary
 *
 * @param ord Ordinal context
 */
void ordinal_print_summary(const Ordinal *ord);

/**
 * @brief Detect number of CPU cores
 *
 * @return Number of cores (minimum 1)
 */
int ordinal_detect_nproc(void);

/**
 * @brief Detect platform name
 *
 * @return "linux", "darwin", "windows", or "unknown"
 */
const char* ordinal_detect_platform(void);

/**
 * @brief Detect architecture
 *
 * @return "x86_64", "aarch64", "arm", etc.
 */
const char* ordinal_detect_arch(void);

#ifdef __cplusplus
}
#endif

#endif /* ORDINAL_H_ */
