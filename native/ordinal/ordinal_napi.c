/**
 * @file zorya_ordinal_napi.c
 * @brief N-API bindings for Ordinal Build System
 *
 * @author Anthony Taliento
 * @date 2025-12-21
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * DESCRIPTION:
 *   Exposes Ordinal build system to Node.js/Electron via N-API.
 *   Provides native-speed task execution with DAG dependency resolution.
 *
 * EXPORTS:
 *   - Ordinal class (main interface)
 *   - version string
 *   - detectPlatform()
 *   - detectArch()
 *   - detectNproc()
 *
 * USAGE:
 *   const { Ordinal } = require('zorya-ordinal');
 *   
 *   const ord = new Ordinal();
 *   ord.load('./Ordinal');
 *   ord.run('build');
 *   console.log(ord.getResult());
 *   ord.close();
 *
 * THREAD SAFETY:
 *   Each Ordinal instance is independent. Not thread-safe within instance.
 */

#include <node_api.h>
#include <stdlib.h>
#include <string.h>
#include "ordinal.h"

/* ============================================================
 * N-API HELPER MACROS
 * ============================================================ */

#define NAPI_CALL(env, call)                                          \
    do {                                                              \
        napi_status status = (call);                                  \
        if (status != napi_ok) {                                      \
            const napi_extended_error_info *error_info = NULL;        \
            napi_get_last_error_info((env), &error_info);             \
            const char *msg = error_info->error_message;              \
            napi_throw_error((env), NULL, msg ? msg : "N-API error"); \
            return NULL;                                              \
        }                                                             \
    } while (0)

#define DECLARE_NAPI_METHOD(name, func)                               \
    { name, 0, func, 0, 0, 0, napi_default, 0 }

/* ============================================================
 * ORDINAL WRAPPER STRUCTURE
 * ============================================================ */

typedef struct {
    Ordinal *ord;
    napi_env env;
    napi_ref progress_callback;
    napi_ref output_callback;
} OrdinalWrapper;

/* ============================================================
 * DESTRUCTOR
 * ============================================================ */

static void ordinal_destructor(napi_env env, void *data, void *hint) {
    (void)env;
    (void)hint;
    
    OrdinalWrapper *wrapper = (OrdinalWrapper *)data;
    if (wrapper != NULL) {
        if (wrapper->ord != NULL) {
            ordinal_free(wrapper->ord);
        }
        free(wrapper);
    }
}

/* ============================================================
 * CONSTRUCTOR
 * ============================================================ */

/**
 * @brief Create new Ordinal instance
 * 
 * JavaScript: const ord = new Ordinal();
 */
static napi_value OrdinalNew(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    /* Create wrapper */
    OrdinalWrapper *wrapper = calloc(1, sizeof(OrdinalWrapper));
    if (wrapper == NULL) {
        napi_throw_error(env, NULL, "Failed to allocate Ordinal wrapper");
        return NULL;
    }
    
    /* Create native Ordinal */
    wrapper->ord = ordinal_new();
    if (wrapper->ord == NULL) {
        free(wrapper);
        napi_throw_error(env, NULL, "Failed to create Ordinal context");
        return NULL;
    }
    
    wrapper->env = env;
    
    /* Wrap native object */
    NAPI_CALL(env, napi_wrap(env, this_arg, wrapper, ordinal_destructor, NULL, NULL));
    
    return this_arg;
}

/* ============================================================
 * LOAD - Load Ordinal file
 * ============================================================ */

/**
 * @brief Load Ordinal file
 * 
 * JavaScript: ord.load('./Ordinal') or ord.load() for auto-detect
 */
static napi_value OrdinalLoad(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &this_arg, NULL));
    
    /* Get wrapper */
    OrdinalWrapper *wrapper;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper->ord == NULL) {
        napi_throw_error(env, NULL, "Ordinal instance is closed");
        return NULL;
    }
    
    /* Get filepath (optional) */
    const char *filepath = NULL;
    char path_buf[4096];
    
    if (argc >= 1) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, args[0], &type));
        
        if (type == napi_string) {
            size_t len;
            NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], path_buf, sizeof(path_buf), &len));
            filepath = path_buf;
        }
    }
    
    /* Load file */
    ordinal_error_t err = ordinal_load(wrapper->ord, filepath);
    
    if (err != ORDINAL_OK) {
        const char *error_msg = ordinal_last_error(wrapper->ord);
        napi_throw_error(env, NULL, error_msg ? error_msg : ordinal_strerror(err));
        return NULL;
    }
    
    /* Return this for chaining */
    return this_arg;
}

/* ============================================================
 * LOAD_STRING - Load from string buffer
 * ============================================================ */

/**
 * @brief Load Ordinal from string
 * 
 * JavaScript: ord.loadString(ordinalContent)
 */
static napi_value OrdinalLoadString(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &this_arg, NULL));
    
    if (argc < 1) {
        napi_throw_error(env, NULL, "loadString requires content argument");
        return NULL;
    }
    
    /* Get wrapper */
    OrdinalWrapper *wrapper;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper->ord == NULL) {
        napi_throw_error(env, NULL, "Ordinal instance is closed");
        return NULL;
    }
    
    /* Get string content */
    size_t len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], NULL, 0, &len));
    
    char *content = malloc(len + 1);
    if (content == NULL) {
        napi_throw_error(env, NULL, "Failed to allocate buffer");
        return NULL;
    }
    
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], content, len + 1, &len));
    
    /* Load content */
    ordinal_error_t err = ordinal_load_buffer(wrapper->ord, content, len);
    free(content);
    
    if (err != ORDINAL_OK) {
        const char *error_msg = ordinal_last_error(wrapper->ord);
        napi_throw_error(env, NULL, error_msg ? error_msg : ordinal_strerror(err));
        return NULL;
    }
    
    return this_arg;
}

/* ============================================================
 * CONFIGURE - Set build options
 * ============================================================ */

/**
 * @brief Configure build options
 * 
 * JavaScript: ord.configure({ verbose: true, dryRun: true, jobs: 4 })
 */
static napi_value OrdinalConfigure(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &this_arg, NULL));
    
    /* Get wrapper */
    OrdinalWrapper *wrapper;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper->ord == NULL) {
        napi_throw_error(env, NULL, "Ordinal instance is closed");
        return NULL;
    }
    
    OrdinalConfig config = ORDINAL_CONFIG_DEFAULT;
    
    if (argc >= 1) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, args[0], &type));
        
        if (type == napi_object) {
            napi_value val;
            bool has_prop;
            
            /* jobs */
            NAPI_CALL(env, napi_has_named_property(env, args[0], "jobs", &has_prop));
            if (has_prop) {
                NAPI_CALL(env, napi_get_named_property(env, args[0], "jobs", &val));
                int32_t jobs;
                NAPI_CALL(env, napi_get_value_int32(env, val, &jobs));
                config.jobs = jobs;
            }
            
            /* verbose */
            NAPI_CALL(env, napi_has_named_property(env, args[0], "verbose", &has_prop));
            if (has_prop) {
                NAPI_CALL(env, napi_get_named_property(env, args[0], "verbose", &val));
                NAPI_CALL(env, napi_get_value_bool(env, val, &config.verbose));
            }
            
            /* dryRun */
            NAPI_CALL(env, napi_has_named_property(env, args[0], "dryRun", &has_prop));
            if (has_prop) {
                NAPI_CALL(env, napi_get_named_property(env, args[0], "dryRun", &val));
                NAPI_CALL(env, napi_get_value_bool(env, val, &config.dry_run));
            }
            
            /* keepGoing */
            NAPI_CALL(env, napi_has_named_property(env, args[0], "keepGoing", &has_prop));
            if (has_prop) {
                NAPI_CALL(env, napi_get_named_property(env, args[0], "keepGoing", &val));
                NAPI_CALL(env, napi_get_value_bool(env, val, &config.keep_going));
            }
            
            /* silent */
            NAPI_CALL(env, napi_has_named_property(env, args[0], "silent", &has_prop));
            if (has_prop) {
                NAPI_CALL(env, napi_get_named_property(env, args[0], "silent", &val));
                NAPI_CALL(env, napi_get_value_bool(env, val, &config.silent));
            }
            
            /* force */
            NAPI_CALL(env, napi_has_named_property(env, args[0], "force", &has_prop));
            if (has_prop) {
                NAPI_CALL(env, napi_get_named_property(env, args[0], "force", &val));
                NAPI_CALL(env, napi_get_value_bool(env, val, &config.force));
            }
            
            /* debug */
            NAPI_CALL(env, napi_has_named_property(env, args[0], "debug", &has_prop));
            if (has_prop) {
                NAPI_CALL(env, napi_get_named_property(env, args[0], "debug", &val));
                NAPI_CALL(env, napi_get_value_bool(env, val, &config.debug));
            }
        }
    }
    
    ordinal_configure(wrapper->ord, &config);
    return this_arg;
}

/* ============================================================
 * RUN - Execute target
 * ============================================================ */

/**
 * @brief Run a target
 * 
 * JavaScript: ord.run('build') or ord.run() for default
 */
static napi_value OrdinalRun(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &this_arg, NULL));
    
    /* Get wrapper */
    OrdinalWrapper *wrapper;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper->ord == NULL) {
        napi_throw_error(env, NULL, "Ordinal instance is closed");
        return NULL;
    }
    
    /* Get target name (optional) */
    const char *target = NULL;
    char target_buf[256];
    
    if (argc >= 1) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, args[0], &type));
        
        if (type == napi_string) {
            size_t len;
            NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], target_buf, sizeof(target_buf), &len));
            target = target_buf;
        }
    }
    
    /* Run target */
    ordinal_error_t err = ordinal_run(wrapper->ord, target);
    
    /* Return result object */
    napi_value result;
    NAPI_CALL(env, napi_create_object(env, &result));
    
    /* success */
    napi_value success_val;
    NAPI_CALL(env, napi_get_boolean(env, err == ORDINAL_OK, &success_val));
    NAPI_CALL(env, napi_set_named_property(env, result, "success", success_val));
    
    /* error */
    if (err != ORDINAL_OK) {
        napi_value error_val;
        const char *error_msg = ordinal_last_error(wrapper->ord);
        NAPI_CALL(env, napi_create_string_utf8(env, error_msg ? error_msg : ordinal_strerror(err), 
                                                NAPI_AUTO_LENGTH, &error_val));
        NAPI_CALL(env, napi_set_named_property(env, result, "error", error_val));
    }
    
    /* Get detailed result */
    OrdinalResult ord_result;
    ordinal_get_result(wrapper->ord, &ord_result);
    
    napi_value stats;
    NAPI_CALL(env, napi_create_object(env, &stats));
    
    napi_value val;
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)ord_result.targets_total, &val));
    NAPI_CALL(env, napi_set_named_property(env, stats, "targetsTotal", val));
    
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)ord_result.targets_rebuilt, &val));
    NAPI_CALL(env, napi_set_named_property(env, stats, "targetsRebuilt", val));
    
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)ord_result.targets_up_to_date, &val));
    NAPI_CALL(env, napi_set_named_property(env, stats, "targetsUpToDate", val));
    
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)ord_result.targets_failed, &val));
    NAPI_CALL(env, napi_set_named_property(env, stats, "targetsFailed", val));
    
    NAPI_CALL(env, napi_create_uint32(env, (uint32_t)ord_result.targets_skipped, &val));
    NAPI_CALL(env, napi_set_named_property(env, stats, "targetsSkipped", val));
    
    NAPI_CALL(env, napi_create_double(env, ord_result.total_time_ms, &val));
    NAPI_CALL(env, napi_set_named_property(env, stats, "totalTimeMs", val));
    
    NAPI_CALL(env, napi_set_named_property(env, result, "stats", stats));
    
    return result;
}

/* ============================================================
 * LIST_TARGETS - Get available targets
 * ============================================================ */

/**
 * @brief Get list of targets
 * 
 * JavaScript: ord.listTargets() // ['build', 'clean', 'test']
 */
static napi_value OrdinalListTargets(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    /* Get wrapper */
    OrdinalWrapper *wrapper;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper->ord == NULL) {
        napi_throw_error(env, NULL, "Ordinal instance is closed");
        return NULL;
    }
    
    /* Get targets */
    size_t count = 0;
    const char **targets = ordinal_list_targets(wrapper->ord, &count);
    
    /* Create array */
    napi_value result;
    NAPI_CALL(env, napi_create_array_with_length(env, count, &result));
    
    for (size_t i = 0; i < count; i++) {
        napi_value str;
        NAPI_CALL(env, napi_create_string_utf8(env, targets[i], NAPI_AUTO_LENGTH, &str));
        NAPI_CALL(env, napi_set_element(env, result, (uint32_t)i, str));
    }
    
    return result;
}

/* ============================================================
 * GET_TARGET - Get target info
 * ============================================================ */

/**
 * @brief Get target information
 * 
 * JavaScript: ord.getTarget('build') // { name, deps, command, status }
 */
static napi_value OrdinalGetTarget(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &this_arg, NULL));
    
    if (argc < 1) {
        napi_throw_error(env, NULL, "getTarget requires target name");
        return NULL;
    }
    
    /* Get wrapper */
    OrdinalWrapper *wrapper;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper->ord == NULL) {
        napi_throw_error(env, NULL, "Ordinal instance is closed");
        return NULL;
    }
    
    /* Get target name */
    char name[256];
    size_t len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], name, sizeof(name), &len));
    
    /* Get target info */
    OrdinalTarget target;
    ordinal_error_t err = ordinal_get_target(wrapper->ord, name, &target);
    
    if (err != ORDINAL_OK) {
        napi_value undefined;
        NAPI_CALL(env, napi_get_undefined(env, &undefined));
        return undefined;
    }
    
    /* Create result object */
    napi_value result;
    NAPI_CALL(env, napi_create_object(env, &result));
    
    napi_value val;
    
    /* name */
    NAPI_CALL(env, napi_create_string_utf8(env, target.name, NAPI_AUTO_LENGTH, &val));
    NAPI_CALL(env, napi_set_named_property(env, result, "name", val));
    
    /* section */
    if (target.section != NULL) {
        NAPI_CALL(env, napi_create_string_utf8(env, target.section, NAPI_AUTO_LENGTH, &val));
        NAPI_CALL(env, napi_set_named_property(env, result, "section", val));
    }
    
    /* command */
    if (target.command != NULL) {
        NAPI_CALL(env, napi_create_string_utf8(env, target.command, NAPI_AUTO_LENGTH, &val));
        NAPI_CALL(env, napi_set_named_property(env, result, "command", val));
    }
    
    /* deps */
    napi_value deps_arr;
    NAPI_CALL(env, napi_create_array_with_length(env, target.dep_count, &deps_arr));
    for (size_t i = 0; i < target.dep_count; i++) {
        NAPI_CALL(env, napi_create_string_utf8(env, target.deps[i], NAPI_AUTO_LENGTH, &val));
        NAPI_CALL(env, napi_set_element(env, deps_arr, (uint32_t)i, val));
    }
    NAPI_CALL(env, napi_set_named_property(env, result, "deps", deps_arr));
    
    /* status */
    const char *status_str;
    switch (target.status) {
        case ORDINAL_STATUS_PENDING: status_str = "pending"; break;
        case ORDINAL_STATUS_BUILDING: status_str = "building"; break;
        case ORDINAL_STATUS_UP_TO_DATE: status_str = "up-to-date"; break;
        case ORDINAL_STATUS_REBUILT: status_str = "rebuilt"; break;
        case ORDINAL_STATUS_FAILED: status_str = "failed"; break;
        case ORDINAL_STATUS_SKIPPED: status_str = "skipped"; break;
        default: status_str = "unknown"; break;
    }
    NAPI_CALL(env, napi_create_string_utf8(env, status_str, NAPI_AUTO_LENGTH, &val));
    NAPI_CALL(env, napi_set_named_property(env, result, "status", val));
    
    /* buildTimeMs */
    NAPI_CALL(env, napi_create_double(env, target.build_time_ms, &val));
    NAPI_CALL(env, napi_set_named_property(env, result, "buildTimeMs", val));
    
    return result;
}

/* ============================================================
 * GET_VAR - Get variable value
 * ============================================================ */

/**
 * @brief Get variable value
 * 
 * JavaScript: ord.getVar('cc') // 'gcc'
 */
static napi_value OrdinalGetVar(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &this_arg, NULL));
    
    if (argc < 1) {
        napi_throw_error(env, NULL, "getVar requires variable name");
        return NULL;
    }
    
    /* Get wrapper */
    OrdinalWrapper *wrapper;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper->ord == NULL) {
        napi_throw_error(env, NULL, "Ordinal instance is closed");
        return NULL;
    }
    
    /* Get variable name */
    char name[256];
    size_t len;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], name, sizeof(name), &len));
    
    /* Get value */
    const char *value = ordinal_get_var(wrapper->ord, name);
    
    if (value == NULL) {
        napi_value undefined;
        NAPI_CALL(env, napi_get_undefined(env, &undefined));
        return undefined;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &result));
    return result;
}

/* ============================================================
 * PROJECT INFO
 * ============================================================ */

/**
 * @brief Get project name
 */
static napi_value OrdinalGetProjectName(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    OrdinalWrapper *wrapper;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper->ord == NULL) {
        napi_throw_error(env, NULL, "Ordinal instance is closed");
        return NULL;
    }
    
    const char *name = ordinal_get_project_name(wrapper->ord);
    
    if (name == NULL) {
        napi_value undefined;
        NAPI_CALL(env, napi_get_undefined(env, &undefined));
        return undefined;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &result));
    return result;
}

/**
 * @brief Get project version
 */
static napi_value OrdinalGetProjectVersion(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    OrdinalWrapper *wrapper;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper->ord == NULL) {
        napi_throw_error(env, NULL, "Ordinal instance is closed");
        return NULL;
    }
    
    const char *version = ordinal_get_project_version(wrapper->ord);
    
    if (version == NULL) {
        napi_value undefined;
        NAPI_CALL(env, napi_get_undefined(env, &undefined));
        return undefined;
    }
    
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, version, NAPI_AUTO_LENGTH, &result));
    return result;
}

/* ============================================================
 * CLOSE - Free resources
 * ============================================================ */

/**
 * @brief Close and free resources
 */
static napi_value OrdinalClose(napi_env env, napi_callback_info info) {
    napi_value this_arg;
    NAPI_CALL(env, napi_get_cb_info(env, info, NULL, NULL, &this_arg, NULL));
    
    OrdinalWrapper *wrapper;
    NAPI_CALL(env, napi_unwrap(env, this_arg, (void **)&wrapper));
    
    if (wrapper->ord != NULL) {
        ordinal_free(wrapper->ord);
        wrapper->ord = NULL;
    }
    
    napi_value undefined;
    NAPI_CALL(env, napi_get_undefined(env, &undefined));
    return undefined;
}

/* ============================================================
 * UTILITY FUNCTIONS
 * ============================================================ */

/**
 * @brief Detect platform
 */
static napi_value DetectPlatform(napi_env env, napi_callback_info info) {
    (void)info;
    
    const char *platform = ordinal_detect_platform();
    
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, platform, NAPI_AUTO_LENGTH, &result));
    return result;
}

/**
 * @brief Detect architecture
 */
static napi_value DetectArch(napi_env env, napi_callback_info info) {
    (void)info;
    
    const char *arch = ordinal_detect_arch();
    
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, arch, NAPI_AUTO_LENGTH, &result));
    return result;
}

/**
 * @brief Detect number of processors
 */
static napi_value DetectNproc(napi_env env, napi_callback_info info) {
    (void)info;
    
    int nproc = ordinal_detect_nproc();
    
    napi_value result;
    NAPI_CALL(env, napi_create_int32(env, nproc, &result));
    return result;
}

/* ============================================================
 * MODULE INITIALIZATION
 * ============================================================ */

static napi_value Init(napi_env env, napi_value exports) {
    /* Define Ordinal class */
    napi_property_descriptor class_props[] = {
        DECLARE_NAPI_METHOD("load", OrdinalLoad),
        DECLARE_NAPI_METHOD("loadString", OrdinalLoadString),
        DECLARE_NAPI_METHOD("configure", OrdinalConfigure),
        DECLARE_NAPI_METHOD("run", OrdinalRun),
        DECLARE_NAPI_METHOD("listTargets", OrdinalListTargets),
        DECLARE_NAPI_METHOD("getTarget", OrdinalGetTarget),
        DECLARE_NAPI_METHOD("getVar", OrdinalGetVar),
        DECLARE_NAPI_METHOD("getProjectName", OrdinalGetProjectName),
        DECLARE_NAPI_METHOD("getProjectVersion", OrdinalGetProjectVersion),
        DECLARE_NAPI_METHOD("close", OrdinalClose),
        DECLARE_NAPI_METHOD("free", OrdinalClose),
        DECLARE_NAPI_METHOD("dispose", OrdinalClose),
    };
    
    napi_value ordinal_class;
    NAPI_CALL(env, napi_define_class(
        env,
        "Ordinal",
        NAPI_AUTO_LENGTH,
        OrdinalNew,
        NULL,
        sizeof(class_props) / sizeof(class_props[0]),
        class_props,
        &ordinal_class
    ));
    
    NAPI_CALL(env, napi_set_named_property(env, exports, "Ordinal", ordinal_class));
    
    /* Export utility functions */
    napi_property_descriptor util_props[] = {
        DECLARE_NAPI_METHOD("detectPlatform", DetectPlatform),
        DECLARE_NAPI_METHOD("detectArch", DetectArch),
        DECLARE_NAPI_METHOD("detectNproc", DetectNproc),
    };
    
    NAPI_CALL(env, napi_define_properties(env, exports, 
        sizeof(util_props) / sizeof(util_props[0]), util_props));
    
    /* Export version */
    napi_value version;
    NAPI_CALL(env, napi_create_string_utf8(env, ORDINAL_VERSION_STRING, NAPI_AUTO_LENGTH, &version));
    NAPI_CALL(env, napi_set_named_property(env, exports, "version", version));
    
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
