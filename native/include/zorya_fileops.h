/**
 * @file zorya_fileops.h
 * @brief Zorya FileOps - Comprehensive cross-platform file I/O library
 *
 * @author Anthony Taliento
 * @date 2025-12-22
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * DESCRIPTION:
 *   Industrial-strength file operations library providing:
 *   - High-performance file I/O with streaming support
 *   - Memory-mapped file access
 *   - Directory traversal and manipulation
 *   - File watching (inotify/FSEvents/ReadDirectoryChangesW)
 *   - Batch operations with hash-based deduplication
 *   - Atomic file operations
 *   - Large file support (64-bit offsets)
 *
 * DESIGN:
 *   Platform-agnostic API with optimized backends per OS.
 *   C99 compliant for maximum portability.
 */

#ifndef ZORYA_FILEOPS_H
#define ZORYA_FILEOPS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Standard Includes
 * ============================================================ */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ============================================================
 * Version Information
 * ============================================================ */

#define ZORYA_FILEOPS_VERSION_MAJOR 1
#define ZORYA_FILEOPS_VERSION_MINOR 0
#define ZORYA_FILEOPS_VERSION_PATCH 0
#define ZORYA_FILEOPS_VERSION "1.0.0"

/* ============================================================
 * Platform Detection
 * ============================================================ */

#if defined(_WIN32) || defined(_WIN64)
    #define ZFO_PLATFORM_WINDOWS 1
    #define ZFO_PLATFORM_POSIX 0
#elif defined(__linux__)
    #define ZFO_PLATFORM_LINUX 1
    #define ZFO_PLATFORM_POSIX 1
#elif defined(__APPLE__)
    #define ZFO_PLATFORM_MACOS 1
    #define ZFO_PLATFORM_POSIX 1
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #define ZFO_PLATFORM_BSD 1
    #define ZFO_PLATFORM_POSIX 1
#else
    #define ZFO_PLATFORM_POSIX 1
#endif

/* ============================================================
 * Type Definitions
 * ============================================================ */

/** File size and offset type (64-bit for large file support) */
typedef int64_t zfo_off_t;

/** File handle (opaque) */
typedef struct zfo_file zfo_file_t;

/** Directory handle (opaque) */
typedef struct zfo_dir zfo_dir_t;

/** Watch handle (opaque) */
typedef struct zfo_watch zfo_watch_t;

/** Memory map handle (opaque) */
typedef struct zfo_mmap zfo_mmap_t;

/* ============================================================
 * Error Codes
 * ============================================================ */

typedef enum {
    ZFO_OK = 0,
    ZFO_ERR_INVALID_ARG = -1,
    ZFO_ERR_NOT_FOUND = -2,
    ZFO_ERR_PERMISSION = -3,
    ZFO_ERR_EXISTS = -4,
    ZFO_ERR_NOT_EMPTY = -5,
    ZFO_ERR_IS_DIR = -6,
    ZFO_ERR_NOT_DIR = -7,
    ZFO_ERR_IO = -8,
    ZFO_ERR_NO_SPACE = -9,
    ZFO_ERR_TOO_MANY_OPEN = -10,
    ZFO_ERR_NAME_TOO_LONG = -11,
    ZFO_ERR_BUSY = -12,
    ZFO_ERR_LOOP = -13,           /* Symlink loop */
    ZFO_ERR_CROSS_DEVICE = -14,   /* Can't move across filesystems */
    ZFO_ERR_NO_MEMORY = -15,
    ZFO_ERR_UNSUPPORTED = -16,
    ZFO_ERR_TIMEOUT = -17,
    ZFO_ERR_INTERRUPTED = -18,
    ZFO_ERR_UNKNOWN = -99
} zfo_error_t;

/* ============================================================
 * File Types
 * ============================================================ */

typedef enum {
    ZFO_TYPE_UNKNOWN = 0,
    ZFO_TYPE_FILE = 1,
    ZFO_TYPE_DIR = 2,
    ZFO_TYPE_SYMLINK = 3,
    ZFO_TYPE_FIFO = 4,      /* Named pipe */
    ZFO_TYPE_SOCKET = 5,
    ZFO_TYPE_BLOCK = 6,     /* Block device */
    ZFO_TYPE_CHAR = 7       /* Character device */
} zfo_file_type_t;

/* ============================================================
 * File Open Flags
 * ============================================================ */

typedef enum {
    ZFO_OPEN_READ       = 0x01,     /* Open for reading */
    ZFO_OPEN_WRITE      = 0x02,     /* Open for writing */
    ZFO_OPEN_APPEND     = 0x04,     /* Append mode */
    ZFO_OPEN_CREATE     = 0x08,     /* Create if not exists */
    ZFO_OPEN_TRUNCATE   = 0x10,     /* Truncate if exists */
    ZFO_OPEN_EXCLUSIVE  = 0x20,     /* Fail if exists (with CREATE) */
    ZFO_OPEN_SYNC       = 0x40,     /* Synchronous I/O */
    ZFO_OPEN_DIRECT     = 0x80,     /* Direct I/O (bypass cache) */
    ZFO_OPEN_TEMP       = 0x100,    /* Temporary file (delete on close) */
    ZFO_OPEN_NOFOLLOW   = 0x200     /* Don't follow symlinks */
} zfo_open_flags_t;

/* ============================================================
 * File Watch Events
 * ============================================================ */

typedef enum {
    ZFO_EVENT_CREATE    = 0x01,     /* File/dir created */
    ZFO_EVENT_DELETE    = 0x02,     /* File/dir deleted */
    ZFO_EVENT_MODIFY    = 0x04,     /* File modified */
    ZFO_EVENT_RENAME    = 0x08,     /* File/dir renamed */
    ZFO_EVENT_ATTRIB    = 0x10,     /* Attributes changed */
    ZFO_EVENT_OPEN      = 0x20,     /* File opened */
    ZFO_EVENT_CLOSE     = 0x40,     /* File closed */
    ZFO_EVENT_MOVE_FROM = 0x80,     /* Moved out of watched dir */
    ZFO_EVENT_MOVE_TO   = 0x100,    /* Moved into watched dir */
    ZFO_EVENT_OVERFLOW  = 0x200,    /* Event queue overflowed */
    ZFO_EVENT_ERROR     = 0x400,    /* Watch error occurred */
    ZFO_EVENT_ALL       = 0x7FF     /* All events */
} zfo_watch_event_t;

/* ============================================================
 * File Information
 * ============================================================ */

typedef struct {
    zfo_file_type_t type;           /* File type */
    zfo_off_t size;                 /* Size in bytes */
    uint32_t mode;                  /* Permission bits */
    uint32_t uid;                   /* Owner user ID */
    uint32_t gid;                   /* Owner group ID */
    uint64_t inode;                 /* Inode number */
    uint64_t dev;                   /* Device ID */
    uint32_t nlink;                 /* Number of hard links */
    time_t atime;                   /* Last access time */
    time_t mtime;                   /* Last modification time */
    time_t ctime;                   /* Last status change time */
    time_t btime;                   /* Birth/creation time (if available) */
    uint64_t blocks;                /* Number of 512-byte blocks allocated */
    uint32_t blksize;               /* Preferred I/O block size */
} zfo_stat_t;

/* ============================================================
 * Directory Entry
 * ============================================================ */

typedef struct {
    char name[256];                 /* Entry name (not full path) */
    zfo_file_type_t type;           /* Entry type */
    uint64_t inode;                 /* Inode (if available) */
} zfo_dirent_t;

/* ============================================================
 * Watch Event Data
 * ============================================================ */

typedef struct {
    zfo_watch_event_t event;        /* Event type */
    char path[4096];                /* Affected path */
    char old_path[4096];            /* Old path (for renames) */
    uint32_t cookie;                /* Cookie for pairing move events */
    bool is_dir;                    /* Is directory? */
} zfo_watch_data_t;

/* ============================================================
 * Copy/Move Options
 * ============================================================ */

typedef struct {
    bool overwrite;                 /* Overwrite existing files */
    bool preserve_mode;             /* Preserve permissions */
    bool preserve_times;            /* Preserve timestamps */
    bool preserve_owner;            /* Preserve owner (requires root) */
    bool follow_symlinks;           /* Follow symlinks (copy target) */
    bool recursive;                 /* Recursive for directories */
    bool atomic;                    /* Use atomic rename */
    size_t buffer_size;             /* I/O buffer size (0 = default) */
} zfo_copy_options_t;

/* ============================================================
 * Walk/Traverse Callback
 * ============================================================ */

/**
 * Callback for directory traversal
 * @param path Full path to entry
 * @param stat Entry statistics
 * @param depth Depth from starting directory
 * @param userdata User-provided context
 * @return 0 to continue, non-zero to stop
 */
typedef int (*zfo_walk_callback_t)(
    const char* path,
    const zfo_stat_t* stat,
    int depth,
    void* userdata
);

/**
 * Callback for file watch events
 * @param data Event data
 * @param userdata User-provided context
 */
typedef void (*zfo_watch_callback_t)(
    const zfo_watch_data_t* data,
    void* userdata
);

/* ============================================================
 * Initialization / Shutdown
 * ============================================================ */

/**
 * Initialize the file operations library
 * Must be called before any other function
 * @return ZFO_OK on success
 */
int zfo_init(void);

/**
 * Shutdown the library and release resources
 */
void zfo_shutdown(void);

/**
 * Get library version string
 */
const char* zfo_version(void);

/**
 * Get error message for error code
 */
const char* zfo_strerror(int error);

/* ============================================================
 * Basic File Operations
 * ============================================================ */

/**
 * Open a file
 * @param path File path
 * @param flags Open flags (ZFO_OPEN_*)
 * @param mode Permission mode for created files (e.g., 0644)
 * @return File handle or NULL on error
 */
zfo_file_t* zfo_open(const char* path, int flags, uint32_t mode);

/**
 * Close a file
 */
int zfo_close(zfo_file_t* file);

/**
 * Read from file
 * @param file File handle
 * @param buf Buffer to read into
 * @param size Maximum bytes to read
 * @return Bytes read, 0 on EOF, negative on error
 */
zfo_off_t zfo_read(zfo_file_t* file, void* buf, size_t size);

/**
 * Write to file
 * @param file File handle
 * @param buf Buffer to write from
 * @param size Bytes to write
 * @return Bytes written, negative on error
 */
zfo_off_t zfo_write(zfo_file_t* file, const void* buf, size_t size);

/**
 * Seek to position
 * @param file File handle
 * @param offset Offset
 * @param whence SEEK_SET, SEEK_CUR, or SEEK_END
 * @return New position, negative on error
 */
zfo_off_t zfo_seek(zfo_file_t* file, zfo_off_t offset, int whence);

/**
 * Get current position
 */
zfo_off_t zfo_tell(zfo_file_t* file);

/**
 * Sync file to disk
 */
int zfo_sync(zfo_file_t* file);

/**
 * Truncate/extend file to size
 */
int zfo_truncate(zfo_file_t* file, zfo_off_t size);

/* ============================================================
 * Convenience I/O Functions
 * ============================================================ */

/**
 * Read entire file into buffer
 * @param path File path
 * @param out_buf Output buffer (caller frees)
 * @param out_size Output size
 * @return ZFO_OK on success
 */
int zfo_read_file(const char* path, uint8_t** out_buf, size_t* out_size);

/**
 * Write buffer to file (creates/truncates)
 */
int zfo_write_file(const char* path, const void* buf, size_t size);

/**
 * Append buffer to file
 */
int zfo_append_file(const char* path, const void* buf, size_t size);

/**
 * Read file as text (null-terminated)
 */
int zfo_read_text(const char* path, char** out_str);

/**
 * Write text to file
 */
int zfo_write_text(const char* path, const char* str);

/* ============================================================
 * File Information
 * ============================================================ */

/**
 * Get file statistics
 */
int zfo_stat(const char* path, zfo_stat_t* stat);

/**
 * Get file statistics (don't follow symlinks)
 */
int zfo_lstat(const char* path, zfo_stat_t* stat);

/**
 * Get file statistics from open handle
 */
int zfo_fstat(zfo_file_t* file, zfo_stat_t* stat);

/**
 * Check if path exists
 */
bool zfo_exists(const char* path);

/**
 * Check if path is a file
 */
bool zfo_is_file(const char* path);

/**
 * Check if path is a directory
 */
bool zfo_is_dir(const char* path);

/**
 * Check if path is a symlink
 */
bool zfo_is_symlink(const char* path);

/**
 * Get file size
 */
zfo_off_t zfo_size(const char* path);

/* ============================================================
 * File Manipulation
 * ============================================================ */

/**
 * Copy file or directory
 */
int zfo_copy(const char* src, const char* dst, const zfo_copy_options_t* opts);

/**
 * Move/rename file or directory
 */
int zfo_move(const char* src, const char* dst, const zfo_copy_options_t* opts);

/**
 * Delete file
 */
int zfo_remove(const char* path);

/**
 * Delete directory (must be empty)
 */
int zfo_rmdir(const char* path);

/**
 * Delete file or directory recursively
 */
int zfo_remove_all(const char* path);

/**
 * Create symbolic link
 */
int zfo_symlink(const char* target, const char* link_path);

/**
 * Create hard link
 */
int zfo_link(const char* target, const char* link_path);

/**
 * Read symbolic link target
 */
int zfo_readlink(const char* path, char* buf, size_t size);

/**
 * Change file permissions
 */
int zfo_chmod(const char* path, uint32_t mode);

/**
 * Change file owner (requires root)
 */
int zfo_chown(const char* path, uint32_t uid, uint32_t gid);

/**
 * Set file timestamps
 */
int zfo_utime(const char* path, time_t atime, time_t mtime);

/**
 * Touch file (create or update mtime)
 */
int zfo_touch(const char* path);

/* ============================================================
 * Directory Operations
 * ============================================================ */

/**
 * Create directory
 */
int zfo_mkdir(const char* path, uint32_t mode);

/**
 * Create directory and all parent directories
 */
int zfo_mkdir_p(const char* path, uint32_t mode);

/**
 * Open directory for reading
 */
zfo_dir_t* zfo_opendir(const char* path);

/**
 * Read next directory entry
 * @return Entry or NULL when done
 */
zfo_dirent_t* zfo_readdir(zfo_dir_t* dir);

/**
 * Rewind directory to beginning
 */
void zfo_rewinddir(zfo_dir_t* dir);

/**
 * Close directory
 */
int zfo_closedir(zfo_dir_t* dir);

/**
 * Walk directory tree
 * @param path Starting path
 * @param callback Called for each entry
 * @param max_depth Maximum depth (-1 for unlimited)
 * @param userdata Passed to callback
 */
int zfo_walk(
    const char* path,
    zfo_walk_callback_t callback,
    int max_depth,
    void* userdata
);

/**
 * List directory contents (convenience)
 * @param path Directory path
 * @param out_entries Output array (caller frees)
 * @param out_count Number of entries
 */
int zfo_listdir(const char* path, zfo_dirent_t** out_entries, size_t* out_count);

/* ============================================================
 * Path Utilities
 * ============================================================ */

/**
 * Get absolute path
 */
int zfo_realpath(const char* path, char* resolved, size_t size);

/**
 * Get directory name component
 */
int zfo_dirname(const char* path, char* buf, size_t size);

/**
 * Get filename component
 */
int zfo_basename(const char* path, char* buf, size_t size);

/**
 * Get file extension (without dot)
 */
int zfo_extname(const char* path, char* buf, size_t size);

/**
 * Join path components
 */
int zfo_join(char* buf, size_t size, const char* base, const char* path);

/**
 * Normalize path (resolve . and ..)
 */
int zfo_normalize(const char* path, char* buf, size_t size);

/**
 * Check if path is absolute
 */
bool zfo_is_absolute(const char* path);

/**
 * Get current working directory
 */
int zfo_getcwd(char* buf, size_t size);

/**
 * Change current working directory
 */
int zfo_chdir(const char* path);

/**
 * Get temporary directory path
 */
int zfo_tmpdir(char* buf, size_t size);

/**
 * Create temporary file
 * @param prefix Filename prefix
 * @param out_path Output path (must be 4096 bytes)
 * @return File handle
 */
zfo_file_t* zfo_tmpfile(const char* prefix, char* out_path);

/* ============================================================
 * Memory Mapping
 * ============================================================ */

typedef enum {
    ZFO_MMAP_READ   = 0x01,     /* Read access */
    ZFO_MMAP_WRITE  = 0x02,     /* Write access */
    ZFO_MMAP_EXEC   = 0x04,     /* Execute access */
    ZFO_MMAP_SHARED = 0x08,     /* Changes visible to other processes */
    ZFO_MMAP_PRIVATE = 0x10    /* Private copy-on-write */
} zfo_mmap_flags_t;

/**
 * Memory-map a file
 * @param path File path
 * @param offset Offset into file
 * @param length Length to map (0 = entire file)
 * @param flags Mapping flags
 * @return Map handle or NULL
 */
zfo_mmap_t* zfo_mmap(const char* path, zfo_off_t offset, size_t length, int flags);

/**
 * Memory-map from open file handle
 */
zfo_mmap_t* zfo_mmap_file(zfo_file_t* file, zfo_off_t offset, size_t length, int flags);

/**
 * Get mapped memory pointer
 */
void* zfo_mmap_ptr(zfo_mmap_t* map);

/**
 * Get mapped length
 */
size_t zfo_mmap_size(zfo_mmap_t* map);

/**
 * Sync mapped changes to disk
 */
int zfo_mmap_sync(zfo_mmap_t* map);

/**
 * Unmap memory
 */
int zfo_mmap_close(zfo_mmap_t* map);

/* ============================================================
 * File Watching
 * ============================================================ */

/**
 * Create a file watcher
 * @return Watch handle or NULL
 */
zfo_watch_t* zfo_watch_create(void);

/**
 * Add path to watch
 * @param watch Watch handle
 * @param path Path to watch
 * @param events Events to watch for (ZFO_EVENT_*)
 * @param recursive Watch subdirectories
 * @return Watch descriptor (>= 0) or error
 */
int zfo_watch_add(zfo_watch_t* watch, const char* path, int events, bool recursive);

/**
 * Remove path from watch
 */
int zfo_watch_remove(zfo_watch_t* watch, int wd);

/**
 * Poll for events (non-blocking)
 * @param watch Watch handle
 * @param callback Called for each event
 * @param userdata Passed to callback
 * @return Number of events processed
 */
int zfo_watch_poll(zfo_watch_t* watch, zfo_watch_callback_t callback, void* userdata);

/**
 * Wait for events (blocking)
 * @param timeout_ms Timeout in milliseconds (-1 = forever)
 */
int zfo_watch_wait(zfo_watch_t* watch, zfo_watch_callback_t callback, void* userdata, int timeout_ms);

/**
 * Get file descriptor for select/poll/epoll integration
 */
int zfo_watch_fd(zfo_watch_t* watch);

/**
 * Destroy watcher
 */
void zfo_watch_destroy(zfo_watch_t* watch);

/* ============================================================
 * Locking
 * ============================================================ */

typedef enum {
    ZFO_LOCK_SHARED    = 0x01,  /* Shared/read lock */
    ZFO_LOCK_EXCLUSIVE = 0x02,  /* Exclusive/write lock */
    ZFO_LOCK_NONBLOCK  = 0x04   /* Don't block if can't acquire */
} zfo_lock_flags_t;

/**
 * Lock file region
 */
int zfo_lock(zfo_file_t* file, zfo_off_t offset, zfo_off_t length, int flags);

/**
 * Unlock file region
 */
int zfo_unlock(zfo_file_t* file, zfo_off_t offset, zfo_off_t length);

/* ============================================================
 * Atomic Operations
 * ============================================================ */

/**
 * Atomically replace file (write to temp, then rename)
 */
int zfo_atomic_write(const char* path, const void* buf, size_t size);

/**
 * Atomically replace file with callback
 * @param path Target path
 * @param callback Called with temp file handle
 * @param userdata Passed to callback
 */
int zfo_atomic_update(const char* path, int (*callback)(zfo_file_t*, void*), void* userdata);

/* ============================================================
 * Glob/Pattern Matching
 * ============================================================ */

/**
 * Find files matching glob pattern
 * @param pattern Glob pattern (e.g., "*.txt", "src/[star][star]/[star].c")
 * @param out_paths Output array of paths (caller frees each and array)
 * @param out_count Number of matches
 */
int zfo_glob(const char* pattern, char*** out_paths, size_t* out_count);

/**
 * Free glob results
 */
void zfo_glob_free(char** paths, size_t count);

/**
 * Check if filename matches pattern
 */
bool zfo_match(const char* pattern, const char* filename);

/* ============================================================
 * Disk Space
 * ============================================================ */

typedef struct {
    uint64_t total;         /* Total space in bytes */
    uint64_t free;          /* Free space in bytes */
    uint64_t available;     /* Available to non-root in bytes */
} zfo_space_t;

/**
 * Get disk space information
 */
int zfo_diskspace(const char* path, zfo_space_t* space);

#ifdef __cplusplus
}
#endif

#endif /* ZORYA_FILEOPS_H */
