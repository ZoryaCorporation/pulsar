/**
 * @file zorya_fileops.c
 * @brief Zorya FileOps - POSIX Implementation
 *
 * @author Anthony Taliento
 * @date 2025-12-22
 * @version 1.0.0
 *
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 */

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include "zorya_fileops.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <limits.h>
#include <fnmatch.h>
#include <glob.h>
#include <utime.h>

#ifdef __linux__
    #include <sys/inotify.h>
    #include <poll.h>
#elif defined(__APPLE__)
    #include <sys/event.h>
    #include <CoreServices/CoreServices.h>
#endif

/* ============================================================
 * Internal Structures
 * ============================================================ */

struct zfo_file {
    int fd;
    char path[PATH_MAX];
    int flags;
};

struct zfo_dir {
    DIR* handle;
    char path[PATH_MAX];
    zfo_dirent_t current;
};

struct zfo_mmap {
    void* ptr;
    size_t size;
    int fd;
    bool owns_fd;
};

#ifdef __linux__
struct zfo_watch {
    int inotify_fd;
    struct {
        int wd;
        char path[PATH_MAX];
        int events;
        bool recursive;
    } watches[256];
    int watch_count;
};
#elif defined(__APPLE__)
struct zfo_watch {
    int kq;
    struct {
        int fd;
        char path[PATH_MAX];
        int events;
    } watches[256];
    int watch_count;
};
#endif

/* ============================================================
 * Error Mapping
 * ============================================================ */

static int errno_to_zfo(int err) {
    switch (err) {
        case 0:           return ZFO_OK;
        case EINVAL:      return ZFO_ERR_INVALID_ARG;
        case ENOENT:      return ZFO_ERR_NOT_FOUND;
        case EACCES:
        case EPERM:       return ZFO_ERR_PERMISSION;
        case EEXIST:      return ZFO_ERR_EXISTS;
        case ENOTEMPTY:   return ZFO_ERR_NOT_EMPTY;
        case EISDIR:      return ZFO_ERR_IS_DIR;
        case ENOTDIR:     return ZFO_ERR_NOT_DIR;
        case EIO:         return ZFO_ERR_IO;
        case ENOSPC:      return ZFO_ERR_NO_SPACE;
        case EMFILE:
        case ENFILE:      return ZFO_ERR_TOO_MANY_OPEN;
        case ENAMETOOLONG: return ZFO_ERR_NAME_TOO_LONG;
        case EBUSY:       return ZFO_ERR_BUSY;
        case ELOOP:       return ZFO_ERR_LOOP;
        case EXDEV:       return ZFO_ERR_CROSS_DEVICE;
        case ENOMEM:      return ZFO_ERR_NO_MEMORY;
        case ENOTSUP:     return ZFO_ERR_UNSUPPORTED;
#if ENOTSUP != EOPNOTSUPP
        case EOPNOTSUPP:  return ZFO_ERR_UNSUPPORTED;
#endif
        case ETIMEDOUT:   return ZFO_ERR_TIMEOUT;
        case EINTR:       return ZFO_ERR_INTERRUPTED;
        default:          return ZFO_ERR_UNKNOWN;
    }
}

static const char* error_messages[] = {
    [0] = "Success",
    [1] = "Invalid argument",
    [2] = "No such file or directory",
    [3] = "Permission denied",
    [4] = "File exists",
    [5] = "Directory not empty",
    [6] = "Is a directory",
    [7] = "Not a directory",
    [8] = "I/O error",
    [9] = "No space left on device",
    [10] = "Too many open files",
    [11] = "Filename too long",
    [12] = "Device or resource busy",
    [13] = "Symbolic link loop",
    [14] = "Cross-device link",
    [15] = "Out of memory",
    [16] = "Operation not supported",
    [17] = "Operation timed out",
    [18] = "Interrupted system call",
};

/* ============================================================
 * Initialization
 * ============================================================ */

int zfo_init(void) {
    return ZFO_OK;
}

void zfo_shutdown(void) {
    /* Nothing to clean up for POSIX */
}

const char* zfo_version(void) {
    return ZORYA_FILEOPS_VERSION;
}

const char* zfo_strerror(int error) {
    int idx = -error;
    if (idx >= 0 && idx < 19) {
        return error_messages[idx];
    }
    return "Unknown error";
}

/* ============================================================
 * File Type Detection
 * ============================================================ */

static zfo_file_type_t mode_to_type(mode_t mode) {
    if (S_ISREG(mode))  return ZFO_TYPE_FILE;
    if (S_ISDIR(mode))  return ZFO_TYPE_DIR;
    if (S_ISLNK(mode))  return ZFO_TYPE_SYMLINK;
    if (S_ISFIFO(mode)) return ZFO_TYPE_FIFO;
    if (S_ISSOCK(mode)) return ZFO_TYPE_SOCKET;
    if (S_ISBLK(mode))  return ZFO_TYPE_BLOCK;
    if (S_ISCHR(mode))  return ZFO_TYPE_CHAR;
    return ZFO_TYPE_UNKNOWN;
}

/* ============================================================
 * Basic File Operations
 * ============================================================ */

zfo_file_t* zfo_open(const char* path, int flags, uint32_t mode) {
    if (!path) return NULL;

    int oflags = 0;

    if ((flags & ZFO_OPEN_READ) && (flags & ZFO_OPEN_WRITE)) {
        oflags |= O_RDWR;
    } else if (flags & ZFO_OPEN_WRITE) {
        oflags |= O_WRONLY;
    } else {
        oflags |= O_RDONLY;
    }

    if (flags & ZFO_OPEN_APPEND)    oflags |= O_APPEND;
    if (flags & ZFO_OPEN_CREATE)    oflags |= O_CREAT;
    if (flags & ZFO_OPEN_TRUNCATE)  oflags |= O_TRUNC;
    if (flags & ZFO_OPEN_EXCLUSIVE) oflags |= O_EXCL;
    if (flags & ZFO_OPEN_SYNC)      oflags |= O_SYNC;
#ifdef O_DIRECT
    if (flags & ZFO_OPEN_DIRECT)    oflags |= O_DIRECT;
#endif
#ifdef O_NOFOLLOW
    if (flags & ZFO_OPEN_NOFOLLOW)  oflags |= O_NOFOLLOW;
#endif

    int fd = open(path, oflags, mode);
    if (fd < 0) return NULL;

    zfo_file_t* file = calloc(1, sizeof(zfo_file_t));
    if (!file) {
        close(fd);
        return NULL;
    }

    file->fd = fd;
    file->flags = flags;
    strncpy(file->path, path, sizeof(file->path) - 1);

    return file;
}

int zfo_close(zfo_file_t* file) {
    if (!file) return ZFO_ERR_INVALID_ARG;

    int ret = close(file->fd);
    int err = errno;
    free(file);

    return ret == 0 ? ZFO_OK : errno_to_zfo(err);
}

zfo_off_t zfo_read(zfo_file_t* file, void* buf, size_t size) {
    if (!file || !buf) return ZFO_ERR_INVALID_ARG;

    ssize_t n = read(file->fd, buf, size);
    if (n < 0) return errno_to_zfo(errno);
    return n;
}

zfo_off_t zfo_write(zfo_file_t* file, const void* buf, size_t size) {
    if (!file || !buf) return ZFO_ERR_INVALID_ARG;

    ssize_t n = write(file->fd, buf, size);
    if (n < 0) return errno_to_zfo(errno);
    return n;
}

zfo_off_t zfo_seek(zfo_file_t* file, zfo_off_t offset, int whence) {
    if (!file) return ZFO_ERR_INVALID_ARG;

    off_t pos = lseek(file->fd, offset, whence);
    if (pos < 0) return errno_to_zfo(errno);
    return pos;
}

zfo_off_t zfo_tell(zfo_file_t* file) {
    if (!file) return ZFO_ERR_INVALID_ARG;
    return lseek(file->fd, 0, SEEK_CUR);
}

int zfo_sync(zfo_file_t* file) {
    if (!file) return ZFO_ERR_INVALID_ARG;
    return fsync(file->fd) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_truncate(zfo_file_t* file, zfo_off_t size) {
    if (!file) return ZFO_ERR_INVALID_ARG;
    return ftruncate(file->fd, size) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

/* ============================================================
 * Convenience I/O
 * ============================================================ */

int zfo_read_file(const char* path, uint8_t** out_buf, size_t* out_size) {
    if (!path || !out_buf || !out_size) return ZFO_ERR_INVALID_ARG;

    struct stat st;
    if (stat(path, &st) != 0) return errno_to_zfo(errno);
    if (!S_ISREG(st.st_mode)) return ZFO_ERR_IS_DIR;

    int fd = open(path, O_RDONLY);
    if (fd < 0) return errno_to_zfo(errno);

    size_t size = st.st_size;
    uint8_t* buf = malloc(size + 1);
    if (!buf) {
        close(fd);
        return ZFO_ERR_NO_MEMORY;
    }

    size_t total = 0;
    while (total < size) {
        ssize_t n = read(fd, buf + total, size - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            free(buf);
            close(fd);
            return errno_to_zfo(errno);
        }
        if (n == 0) break;
        total += n;
    }

    buf[total] = 0;  /* Null terminate for text use */
    close(fd);

    *out_buf = buf;
    *out_size = total;
    return ZFO_OK;
}

int zfo_write_file(const char* path, const void* buf, size_t size) {
    if (!path || !buf) return ZFO_ERR_INVALID_ARG;

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return errno_to_zfo(errno);

    size_t total = 0;
    while (total < size) {
        ssize_t n = write(fd, (const uint8_t*)buf + total, size - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            close(fd);
            return errno_to_zfo(errno);
        }
        total += n;
    }

    close(fd);
    return ZFO_OK;
}

int zfo_append_file(const char* path, const void* buf, size_t size) {
    if (!path || !buf) return ZFO_ERR_INVALID_ARG;

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return errno_to_zfo(errno);

    size_t total = 0;
    while (total < size) {
        ssize_t n = write(fd, (const uint8_t*)buf + total, size - total);
        if (n < 0) {
            if (errno == EINTR) continue;
            close(fd);
            return errno_to_zfo(errno);
        }
        total += n;
    }

    close(fd);
    return ZFO_OK;
}

int zfo_read_text(const char* path, char** out_str) {
    size_t size;
    return zfo_read_file(path, (uint8_t**)out_str, &size);
}

int zfo_write_text(const char* path, const char* str) {
    return zfo_write_file(path, str, strlen(str));
}

/* ============================================================
 * File Information
 * ============================================================ */

static void stat_to_zfo(const struct stat* st, zfo_stat_t* zst) {
    zst->type = mode_to_type(st->st_mode);
    zst->size = st->st_size;
    zst->mode = st->st_mode & 07777;
    zst->uid = st->st_uid;
    zst->gid = st->st_gid;
    zst->inode = st->st_ino;
    zst->dev = st->st_dev;
    zst->nlink = st->st_nlink;
    zst->atime = st->st_atime;
    zst->mtime = st->st_mtime;
    zst->ctime = st->st_ctime;
#ifdef __APPLE__
    zst->btime = st->st_birthtime;
#else
    zst->btime = 0;
#endif
    zst->blocks = st->st_blocks;
    zst->blksize = st->st_blksize;
}

int zfo_stat(const char* path, zfo_stat_t* zst) {
    if (!path || !zst) return ZFO_ERR_INVALID_ARG;

    struct stat st;
    if (stat(path, &st) != 0) return errno_to_zfo(errno);

    stat_to_zfo(&st, zst);
    return ZFO_OK;
}

int zfo_lstat(const char* path, zfo_stat_t* zst) {
    if (!path || !zst) return ZFO_ERR_INVALID_ARG;

    struct stat st;
    if (lstat(path, &st) != 0) return errno_to_zfo(errno);

    stat_to_zfo(&st, zst);
    return ZFO_OK;
}

int zfo_fstat(zfo_file_t* file, zfo_stat_t* zst) {
    if (!file || !zst) return ZFO_ERR_INVALID_ARG;

    struct stat st;
    if (fstat(file->fd, &st) != 0) return errno_to_zfo(errno);

    stat_to_zfo(&st, zst);
    return ZFO_OK;
}

bool zfo_exists(const char* path) {
    if (!path) return false;
    return access(path, F_OK) == 0;
}

bool zfo_is_file(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

bool zfo_is_dir(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

bool zfo_is_symlink(const char* path) {
    struct stat st;
    return lstat(path, &st) == 0 && S_ISLNK(st.st_mode);
}

zfo_off_t zfo_size(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return st.st_size;
}

/* ============================================================
 * File Manipulation
 * ============================================================ */

int zfo_copy(const char* src, const char* dst, const zfo_copy_options_t* opts) {
    if (!src || !dst) return ZFO_ERR_INVALID_ARG;

    zfo_copy_options_t default_opts = {
        .overwrite = false,
        .preserve_mode = true,
        .preserve_times = true,
        .preserve_owner = false,
        .follow_symlinks = true,
        .recursive = true,
        .atomic = false,
        .buffer_size = 64 * 1024
    };

    if (!opts) opts = &default_opts;
    size_t bufsize = opts->buffer_size > 0 ? opts->buffer_size : 64 * 1024;

    struct stat st;
    int (*stat_fn)(const char*, struct stat*) = opts->follow_symlinks ? stat : lstat;

    if (stat_fn(src, &st) != 0) return errno_to_zfo(errno);

    /* Handle directories */
    if (S_ISDIR(st.st_mode)) {
        if (!opts->recursive) return ZFO_ERR_IS_DIR;

        if (mkdir(dst, st.st_mode) != 0 && errno != EEXIST) {
            return errno_to_zfo(errno);
        }

        DIR* dir = opendir(src);
        if (!dir) return errno_to_zfo(errno);

        struct dirent* entry;
        int ret = ZFO_OK;

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char src_path[PATH_MAX], dst_path[PATH_MAX];
            snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

            ret = zfo_copy(src_path, dst_path, opts);
            if (ret != ZFO_OK) break;
        }

        closedir(dir);
        return ret;
    }

    /* Handle symlinks */
    if (S_ISLNK(st.st_mode) && !opts->follow_symlinks) {
        char link_target[PATH_MAX];
        ssize_t len = readlink(src, link_target, sizeof(link_target) - 1);
        if (len < 0) return errno_to_zfo(errno);
        link_target[len] = 0;

        if (symlink(link_target, dst) != 0) {
            if (errno == EEXIST && opts->overwrite) {
                unlink(dst);
                if (symlink(link_target, dst) != 0) return errno_to_zfo(errno);
            } else {
                return errno_to_zfo(errno);
            }
        }
        return ZFO_OK;
    }

    /* Regular file copy */
    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0) return errno_to_zfo(errno);

    int dst_flags = O_WRONLY | O_CREAT | O_TRUNC;
    if (!opts->overwrite) dst_flags |= O_EXCL;

    int dst_fd = open(dst, dst_flags, st.st_mode);
    if (dst_fd < 0) {
        int err = errno;
        close(src_fd);
        return errno_to_zfo(err);
    }

    char* buf = malloc(bufsize);
    if (!buf) {
        close(src_fd);
        close(dst_fd);
        return ZFO_ERR_NO_MEMORY;
    }

    ssize_t n;
    int ret = ZFO_OK;

    while ((n = read(src_fd, buf, bufsize)) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(dst_fd, buf + written, n - written);
            if (w < 0) {
                if (errno == EINTR) continue;
                ret = errno_to_zfo(errno);
                goto cleanup;
            }
            written += w;
        }
    }

    if (n < 0) ret = errno_to_zfo(errno);

    /* Preserve attributes */
    if (ret == ZFO_OK) {
        if (opts->preserve_mode) fchmod(dst_fd, st.st_mode);
        if (opts->preserve_owner) fchown(dst_fd, st.st_uid, st.st_gid);
    }

cleanup:
    free(buf);
    close(src_fd);
    close(dst_fd);

    if (ret == ZFO_OK && opts->preserve_times) {
        struct utimbuf times = { .actime = st.st_atime, .modtime = st.st_mtime };
        utime(dst, &times);
    }

    return ret;
}

int zfo_move(const char* src, const char* dst, const zfo_copy_options_t* opts) {
    if (!src || !dst) return ZFO_ERR_INVALID_ARG;

    /* Try rename first (same filesystem) */
    if (rename(src, dst) == 0) return ZFO_OK;

    if (errno != EXDEV) return errno_to_zfo(errno);

    /* Cross-device: copy then delete */
    int ret = zfo_copy(src, dst, opts);
    if (ret != ZFO_OK) return ret;

    return zfo_remove_all(src);
}

int zfo_remove(const char* path) {
    if (!path) return ZFO_ERR_INVALID_ARG;
    return unlink(path) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_rmdir(const char* path) {
    if (!path) return ZFO_ERR_INVALID_ARG;
    return rmdir(path) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_remove_all(const char* path) {
    if (!path) return ZFO_ERR_INVALID_ARG;

    struct stat st;
    if (lstat(path, &st) != 0) {
        return errno == ENOENT ? ZFO_OK : errno_to_zfo(errno);
    }

    if (!S_ISDIR(st.st_mode)) {
        return unlink(path) == 0 ? ZFO_OK : errno_to_zfo(errno);
    }

    DIR* dir = opendir(path);
    if (!dir) return errno_to_zfo(errno);

    struct dirent* entry;
    int ret = ZFO_OK;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char child[PATH_MAX];
        snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);

        ret = zfo_remove_all(child);
        if (ret != ZFO_OK) break;
    }

    closedir(dir);

    if (ret == ZFO_OK) {
        ret = rmdir(path) == 0 ? ZFO_OK : errno_to_zfo(errno);
    }

    return ret;
}

int zfo_symlink(const char* target, const char* link_path) {
    if (!target || !link_path) return ZFO_ERR_INVALID_ARG;
    return symlink(target, link_path) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_link(const char* target, const char* link_path) {
    if (!target || !link_path) return ZFO_ERR_INVALID_ARG;
    return link(target, link_path) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_readlink(const char* path, char* buf, size_t size) {
    if (!path || !buf || size == 0) return ZFO_ERR_INVALID_ARG;

    ssize_t n = readlink(path, buf, size - 1);
    if (n < 0) return errno_to_zfo(errno);

    buf[n] = 0;
    return ZFO_OK;
}

int zfo_chmod(const char* path, uint32_t mode) {
    if (!path) return ZFO_ERR_INVALID_ARG;
    return chmod(path, mode) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_chown(const char* path, uint32_t uid, uint32_t gid) {
    if (!path) return ZFO_ERR_INVALID_ARG;
    return chown(path, uid, gid) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_utime(const char* path, time_t atime, time_t mtime) {
    if (!path) return ZFO_ERR_INVALID_ARG;
    struct utimbuf times = { .actime = atime, .modtime = mtime };
    return utime(path, &times) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_touch(const char* path) {
    if (!path) return ZFO_ERR_INVALID_ARG;

    int fd = open(path, O_WRONLY | O_CREAT | O_NOCTTY, 0644);
    if (fd < 0 && errno != EISDIR) return errno_to_zfo(errno);
    if (fd >= 0) close(fd);

    return utime(path, NULL) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

/* ============================================================
 * Directory Operations
 * ============================================================ */

int zfo_mkdir(const char* path, uint32_t mode) {
    if (!path) return ZFO_ERR_INVALID_ARG;
    return mkdir(path, mode) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_mkdir_p(const char* path, uint32_t mode) {
    if (!path) return ZFO_ERR_INVALID_ARG;

    char tmp[PATH_MAX];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;

    size_t len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;

    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
                return errno_to_zfo(errno);
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
        return errno_to_zfo(errno);
    }

    return ZFO_OK;
}

zfo_dir_t* zfo_opendir(const char* path) {
    if (!path) return NULL;

    DIR* handle = opendir(path);
    if (!handle) return NULL;

    zfo_dir_t* dir = calloc(1, sizeof(zfo_dir_t));
    if (!dir) {
        closedir(handle);
        return NULL;
    }

    dir->handle = handle;
    strncpy(dir->path, path, sizeof(dir->path) - 1);

    return dir;
}

zfo_dirent_t* zfo_readdir(zfo_dir_t* dir) {
    if (!dir) return NULL;

    struct dirent* entry = readdir(dir->handle);
    if (!entry) return NULL;

    size_t name_len = strlen(entry->d_name);
    if (name_len >= sizeof(dir->current.name)) {
        name_len = sizeof(dir->current.name) - 1;
    }
    memcpy(dir->current.name, entry->d_name, name_len);
    dir->current.name[name_len] = '\0';

#ifdef _DIRENT_HAVE_D_TYPE
    switch (entry->d_type) {
        case DT_REG:  dir->current.type = ZFO_TYPE_FILE; break;
        case DT_DIR:  dir->current.type = ZFO_TYPE_DIR; break;
        case DT_LNK:  dir->current.type = ZFO_TYPE_SYMLINK; break;
        case DT_FIFO: dir->current.type = ZFO_TYPE_FIFO; break;
        case DT_SOCK: dir->current.type = ZFO_TYPE_SOCKET; break;
        case DT_BLK:  dir->current.type = ZFO_TYPE_BLOCK; break;
        case DT_CHR:  dir->current.type = ZFO_TYPE_CHAR; break;
        default:      dir->current.type = ZFO_TYPE_UNKNOWN; break;
    }
#else
    dir->current.type = ZFO_TYPE_UNKNOWN;
#endif

#ifdef _DIRENT_HAVE_D_INO
    dir->current.inode = entry->d_ino;
#else
    dir->current.inode = 0;
#endif

    return &dir->current;
}

void zfo_rewinddir(zfo_dir_t* dir) {
    if (dir) rewinddir(dir->handle);
}

int zfo_closedir(zfo_dir_t* dir) {
    if (!dir) return ZFO_ERR_INVALID_ARG;
    closedir(dir->handle);
    free(dir);
    return ZFO_OK;
}

int zfo_walk(const char* path, zfo_walk_callback_t callback, int max_depth, void* userdata) {
    if (!path || !callback) return ZFO_ERR_INVALID_ARG;

    zfo_stat_t st;
    int ret = zfo_stat(path, &st);
    if (ret != ZFO_OK) return ret;

    if (callback(path, &st, 0, userdata) != 0) return ZFO_OK;

    if (st.type != ZFO_TYPE_DIR) return ZFO_OK;
    if (max_depth == 0) return ZFO_OK;

    DIR* dir = opendir(path);
    if (!dir) return errno_to_zfo(errno);

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char child[PATH_MAX];
        snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);

        ret = zfo_stat(child, &st);
        if (ret != ZFO_OK) continue;

        if (callback(child, &st, 1, userdata) != 0) break;

        if (st.type == ZFO_TYPE_DIR && max_depth != 0) {
            /* Recursively walk subdirectories */
            zfo_walk(child, callback, max_depth > 0 ? max_depth - 1 : -1, userdata);
        }
    }

    closedir(dir);
    return ZFO_OK;
}

int zfo_listdir(const char* path, zfo_dirent_t** out_entries, size_t* out_count) {
    if (!path || !out_entries || !out_count) return ZFO_ERR_INVALID_ARG;

    DIR* dir = opendir(path);
    if (!dir) return errno_to_zfo(errno);

    /* Count entries first */
    size_t count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }

    zfo_dirent_t* entries = calloc(count, sizeof(zfo_dirent_t));
    if (!entries) {
        closedir(dir);
        return ZFO_ERR_NO_MEMORY;
    }

    rewinddir(dir);
    size_t i = 0;
    while ((entry = readdir(dir)) != NULL && i < count) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t name_len = strlen(entry->d_name);
        if (name_len >= sizeof(entries[i].name)) {
            name_len = sizeof(entries[i].name) - 1;
        }
        memcpy(entries[i].name, entry->d_name, name_len);
        entries[i].name[name_len] = '\0';

#ifdef _DIRENT_HAVE_D_TYPE
        switch (entry->d_type) {
            case DT_REG:  entries[i].type = ZFO_TYPE_FILE; break;
            case DT_DIR:  entries[i].type = ZFO_TYPE_DIR; break;
            case DT_LNK:  entries[i].type = ZFO_TYPE_SYMLINK; break;
            default:      entries[i].type = ZFO_TYPE_UNKNOWN; break;
        }
#endif

        i++;
    }

    closedir(dir);

    *out_entries = entries;
    *out_count = i;
    return ZFO_OK;
}

/* ============================================================
 * Path Utilities
 * ============================================================ */

int zfo_realpath(const char* path, char* resolved, size_t size) {
    if (!path || !resolved || size == 0) return ZFO_ERR_INVALID_ARG;

    char* result = realpath(path, NULL);
    if (!result) return errno_to_zfo(errno);

    strncpy(resolved, result, size - 1);
    resolved[size - 1] = 0;
    free(result);

    return ZFO_OK;
}

int zfo_dirname(const char* path, char* buf, size_t size) {
    if (!path || !buf || size == 0) return ZFO_ERR_INVALID_ARG;

    char tmp[PATH_MAX];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;

    char* last_slash = strrchr(tmp, '/');
    if (!last_slash) {
        strncpy(buf, ".", size - 1);
    } else if (last_slash == tmp) {
        strncpy(buf, "/", size - 1);
    } else {
        *last_slash = 0;
        strncpy(buf, tmp, size - 1);
    }

    buf[size - 1] = 0;
    return ZFO_OK;
}

int zfo_basename(const char* path, char* buf, size_t size) {
    if (!path || !buf || size == 0) return ZFO_ERR_INVALID_ARG;

    /* Handle trailing slashes by working on a copy */
    char tmp[4096];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;
    
    /* Strip trailing slashes */
    size_t len = strlen(tmp);
    while (len > 1 && tmp[len - 1] == '/') {
        tmp[--len] = 0;
    }

    const char* last_slash = strrchr(tmp, '/');
    const char* name = last_slash ? last_slash + 1 : tmp;

    strncpy(buf, name, size - 1);
    buf[size - 1] = 0;
    return ZFO_OK;
}

int zfo_extname(const char* path, char* buf, size_t size) {
    if (!path || !buf || size == 0) return ZFO_ERR_INVALID_ARG;

    const char* last_slash = strrchr(path, '/');
    const char* name = last_slash ? last_slash + 1 : path;
    const char* dot = strrchr(name, '.');

    if (!dot || dot == name) {
        buf[0] = 0;
    } else {
        /* Include the dot in the extension */
        strncpy(buf, dot, size - 1);
        buf[size - 1] = 0;
    }

    return ZFO_OK;
}

int zfo_join(char* buf, size_t size, const char* base, const char* path) {
    if (!buf || size == 0 || !base || !path) return ZFO_ERR_INVALID_ARG;

    if (path[0] == '/') {
        strncpy(buf, path, size - 1);
    } else {
        size_t base_len = strlen(base);
        bool needs_slash = base_len > 0 && base[base_len - 1] != '/';

        snprintf(buf, size, "%s%s%s", base, needs_slash ? "/" : "", path);
    }

    buf[size - 1] = 0;
    return ZFO_OK;
}

int zfo_normalize(const char* path, char* buf, size_t size) {
    if (!path || !buf || size == 0) return ZFO_ERR_INVALID_ARG;

    /* Handle . and .. without requiring path to exist */
    char tmp[4096];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;

    /* Split path into components */
    char* components[256];
    int count = 0;
    bool is_absolute = (tmp[0] == '/');
    
    char* saveptr = NULL;
    char* token = strtok_r(tmp, "/", &saveptr);
    while (token && count < 256) {
        if (strcmp(token, ".") == 0) {
            /* Skip "." */
        } else if (strcmp(token, "..") == 0) {
            /* Go up one level if possible */
            if (count > 0 && strcmp(components[count - 1], "..") != 0) {
                count--;
            } else if (!is_absolute) {
                components[count++] = "..";
            }
        } else {
            components[count++] = token;
        }
        token = strtok_r(NULL, "/", &saveptr);
    }

    /* Rebuild path */
    buf[0] = 0;
    size_t pos = 0;
    
    if (is_absolute) {
        buf[pos++] = '/';
        buf[pos] = 0;
    }

    for (int i = 0; i < count; i++) {
        if (i > 0) {
            if (pos < size - 1) buf[pos++] = '/';
        }
        size_t clen = strlen(components[i]);
        size_t copy_len = (pos + clen < size - 1) ? clen : (size - 1 - pos);
        memcpy(buf + pos, components[i], copy_len);
        pos += copy_len;
        buf[pos] = 0;
    }

    if (pos == 0) {
        buf[0] = '.';
        buf[1] = 0;
    }

    return ZFO_OK;
}

bool zfo_is_absolute(const char* path) {
    return path && path[0] == '/';
}

int zfo_getcwd(char* buf, size_t size) {
    if (!buf || size == 0) return ZFO_ERR_INVALID_ARG;
    return getcwd(buf, size) ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_chdir(const char* path) {
    if (!path) return ZFO_ERR_INVALID_ARG;
    return chdir(path) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_tmpdir(char* buf, size_t size) {
    if (!buf || size == 0) return ZFO_ERR_INVALID_ARG;

    const char* tmp = getenv("TMPDIR");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = getenv("TEMP");
    if (!tmp) tmp = "/tmp";

    strncpy(buf, tmp, size - 1);
    buf[size - 1] = 0;
    return ZFO_OK;
}

zfo_file_t* zfo_tmpfile(const char* prefix, char* out_path) {
    char template[PATH_MAX];
    char tmpdir_buf[PATH_MAX];
    const char* safe_prefix = prefix ? prefix : "zfo";

    zfo_tmpdir(tmpdir_buf, sizeof(tmpdir_buf));
    
    /* Ensure we don't overflow template buffer */
    size_t tmpdir_len = strlen(tmpdir_buf);
    size_t prefix_len = strlen(safe_prefix);
    if (tmpdir_len + prefix_len + 8 >= sizeof(template)) {
        /* Truncate tmpdir if needed */
        tmpdir_buf[sizeof(template) - prefix_len - 9] = '\0';
    }
    snprintf(template, sizeof(template), "%s/%sXXXXXX", tmpdir_buf, safe_prefix);

    int fd = mkstemp(template);
    if (fd < 0) return NULL;

    if (out_path) {
        strncpy(out_path, template, PATH_MAX - 1);
        out_path[PATH_MAX - 1] = 0;
    }

    zfo_file_t* file = calloc(1, sizeof(zfo_file_t));
    if (!file) {
        close(fd);
        unlink(template);
        return NULL;
    }

    file->fd = fd;
    size_t tmpl_len = strlen(template);
    if (tmpl_len >= sizeof(file->path)) {
        tmpl_len = sizeof(file->path) - 1;
    }
    memcpy(file->path, template, tmpl_len);
    file->path[tmpl_len] = '\0';

    return file;
}

/* ============================================================
 * Memory Mapping
 * ============================================================ */

zfo_mmap_t* zfo_mmap(const char* path, zfo_off_t offset, size_t length, int flags) {
    if (!path) return NULL;

    int oflags = O_RDONLY;
    if (flags & ZFO_MMAP_WRITE) oflags = O_RDWR;

    int fd = open(path, oflags);
    if (fd < 0) return NULL;

    zfo_mmap_t* map = calloc(1, sizeof(zfo_mmap_t));
    if (!map) {
        close(fd);
        return NULL;
    }

    map->fd = fd;
    map->owns_fd = true;

    /* Get file size if length not specified */
    if (length == 0) {
        struct stat st;
        if (fstat(fd, &st) != 0) {
            close(fd);
            free(map);
            return NULL;
        }
        length = st.st_size - offset;
    }

    int prot = 0;
    if (flags & ZFO_MMAP_READ)  prot |= PROT_READ;
    if (flags & ZFO_MMAP_WRITE) prot |= PROT_WRITE;
    if (flags & ZFO_MMAP_EXEC)  prot |= PROT_EXEC;

    int mflags = MAP_FILE;
    if (flags & ZFO_MMAP_SHARED) mflags |= MAP_SHARED;
    else mflags |= MAP_PRIVATE;

    void* ptr = mmap(NULL, length, prot, mflags, fd, offset);
    if (ptr == MAP_FAILED) {
        close(fd);
        free(map);
        return NULL;
    }

    map->ptr = ptr;
    map->size = length;

    return map;
}

zfo_mmap_t* zfo_mmap_file(zfo_file_t* file, zfo_off_t offset, size_t length, int flags) {
    if (!file) return NULL;

    zfo_mmap_t* map = calloc(1, sizeof(zfo_mmap_t));
    if (!map) return NULL;

    map->fd = file->fd;
    map->owns_fd = false;

    if (length == 0) {
        struct stat st;
        if (fstat(file->fd, &st) != 0) {
            free(map);
            return NULL;
        }
        length = st.st_size - offset;
    }

    int prot = 0;
    if (flags & ZFO_MMAP_READ)  prot |= PROT_READ;
    if (flags & ZFO_MMAP_WRITE) prot |= PROT_WRITE;
    if (flags & ZFO_MMAP_EXEC)  prot |= PROT_EXEC;

    int mflags = MAP_FILE;
    if (flags & ZFO_MMAP_SHARED) mflags |= MAP_SHARED;
    else mflags |= MAP_PRIVATE;

    void* ptr = mmap(NULL, length, prot, mflags, file->fd, offset);
    if (ptr == MAP_FAILED) {
        free(map);
        return NULL;
    }

    map->ptr = ptr;
    map->size = length;

    return map;
}

void* zfo_mmap_ptr(zfo_mmap_t* map) {
    return map ? map->ptr : NULL;
}

size_t zfo_mmap_size(zfo_mmap_t* map) {
    return map ? map->size : 0;
}

int zfo_mmap_sync(zfo_mmap_t* map) {
    if (!map) return ZFO_ERR_INVALID_ARG;
    return msync(map->ptr, map->size, MS_SYNC) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_mmap_close(zfo_mmap_t* map) {
    if (!map) return ZFO_ERR_INVALID_ARG;

    int ret = ZFO_OK;
    if (munmap(map->ptr, map->size) != 0) {
        ret = errno_to_zfo(errno);
    }

    if (map->owns_fd && map->fd >= 0) {
        close(map->fd);
    }

    free(map);
    return ret;
}

/* ============================================================
 * File Watching (Linux inotify)
 * ============================================================ */

#ifdef __linux__

zfo_watch_t* zfo_watch_create(void) {
    int fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (fd < 0) return NULL;

    zfo_watch_t* watch = calloc(1, sizeof(zfo_watch_t));
    if (!watch) {
        close(fd);
        return NULL;
    }

    watch->inotify_fd = fd;
    return watch;
}

int zfo_watch_add(zfo_watch_t* watch, const char* path, int events, bool recursive) {
    if (!watch || !path) return ZFO_ERR_INVALID_ARG;
    if (watch->watch_count >= 256) return ZFO_ERR_TOO_MANY_OPEN;

    uint32_t mask = 0;
    if (events & ZFO_EVENT_CREATE)    mask |= IN_CREATE;
    if (events & ZFO_EVENT_DELETE)    mask |= IN_DELETE | IN_DELETE_SELF;
    if (events & ZFO_EVENT_MODIFY)    mask |= IN_MODIFY;
    if (events & ZFO_EVENT_RENAME)    mask |= IN_MOVE;
    if (events & ZFO_EVENT_ATTRIB)    mask |= IN_ATTRIB;
    if (events & ZFO_EVENT_OPEN)      mask |= IN_OPEN;
    if (events & ZFO_EVENT_CLOSE)     mask |= IN_CLOSE;
    if (events & ZFO_EVENT_MOVE_FROM) mask |= IN_MOVED_FROM;
    if (events & ZFO_EVENT_MOVE_TO)   mask |= IN_MOVED_TO;

    int wd = inotify_add_watch(watch->inotify_fd, path, mask);
    if (wd < 0) return errno_to_zfo(errno);

    int idx = watch->watch_count++;
    watch->watches[idx].wd = wd;
    strncpy(watch->watches[idx].path, path, PATH_MAX - 1);
    watch->watches[idx].events = events;
    watch->watches[idx].recursive = recursive;

    /* Add subdirectories if recursive */
    if (recursive) {
        DIR* dir = opendir(path);
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] == '.') continue;
                if (entry->d_type != DT_DIR) continue;

                char subpath[PATH_MAX];
                snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
                zfo_watch_add(watch, subpath, events, true);
            }
            closedir(dir);
        }
    }

    return wd;
}

int zfo_watch_remove(zfo_watch_t* watch, int wd) {
    if (!watch) return ZFO_ERR_INVALID_ARG;
    return inotify_rm_watch(watch->inotify_fd, wd) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

static const char* find_watch_path(zfo_watch_t* watch, int wd) {
    for (int i = 0; i < watch->watch_count; i++) {
        if (watch->watches[i].wd == wd) {
            return watch->watches[i].path;
        }
    }
    return "";
}

int zfo_watch_poll(zfo_watch_t* watch, zfo_watch_callback_t callback, void* userdata) {
    if (!watch || !callback) return ZFO_ERR_INVALID_ARG;

    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    int count = 0;

    ssize_t len = read(watch->inotify_fd, buf, sizeof(buf));
    if (len <= 0) return 0;

    const char* ptr = buf;
    while (ptr < buf + len) {
        const struct inotify_event* event = (const struct inotify_event*)ptr;

        zfo_watch_data_t data = {0};
        const char* base_path = find_watch_path(watch, event->wd);

        if (event->len > 0) {
            snprintf(data.path, sizeof(data.path), "%s/%s", base_path, event->name);
        } else {
            strncpy(data.path, base_path, sizeof(data.path) - 1);
        }

        data.cookie = event->cookie;
        data.is_dir = event->mask & IN_ISDIR;

        if (event->mask & IN_CREATE)     data.event = ZFO_EVENT_CREATE;
        else if (event->mask & IN_DELETE) data.event = ZFO_EVENT_DELETE;
        else if (event->mask & IN_MODIFY) data.event = ZFO_EVENT_MODIFY;
        else if (event->mask & IN_MOVED_FROM) data.event = ZFO_EVENT_MOVE_FROM;
        else if (event->mask & IN_MOVED_TO)   data.event = ZFO_EVENT_MOVE_TO;
        else if (event->mask & IN_ATTRIB) data.event = ZFO_EVENT_ATTRIB;
        else if (event->mask & IN_OPEN)   data.event = ZFO_EVENT_OPEN;
        else if (event->mask & IN_CLOSE)  data.event = ZFO_EVENT_CLOSE;
        else if (event->mask & IN_Q_OVERFLOW) data.event = ZFO_EVENT_OVERFLOW;

        callback(&data, userdata);
        count++;

        ptr += sizeof(struct inotify_event) + event->len;
    }

    return count;
}

int zfo_watch_wait(zfo_watch_t* watch, zfo_watch_callback_t callback, void* userdata, int timeout_ms) {
    if (!watch || !callback) return ZFO_ERR_INVALID_ARG;

    struct pollfd pfd = { .fd = watch->inotify_fd, .events = POLLIN };

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret < 0) return errno_to_zfo(errno);
    if (ret == 0) return ZFO_ERR_TIMEOUT;

    return zfo_watch_poll(watch, callback, userdata);
}

int zfo_watch_fd(zfo_watch_t* watch) {
    return watch ? watch->inotify_fd : -1;
}

void zfo_watch_destroy(zfo_watch_t* watch) {
    if (!watch) return;

    for (int i = 0; i < watch->watch_count; i++) {
        inotify_rm_watch(watch->inotify_fd, watch->watches[i].wd);
    }

    close(watch->inotify_fd);
    free(watch);
}

#endif /* __linux__ */

/* ============================================================
 * File Locking
 * ============================================================ */

int zfo_lock(zfo_file_t* file, zfo_off_t offset, zfo_off_t length, int flags) {
    if (!file) return ZFO_ERR_INVALID_ARG;

    struct flock fl = {
        .l_whence = SEEK_SET,
        .l_start = offset,
        .l_len = length,
        .l_type = (flags & ZFO_LOCK_SHARED) ? F_RDLCK : F_WRLCK
    };

    int cmd = (flags & ZFO_LOCK_NONBLOCK) ? F_SETLK : F_SETLKW;
    return fcntl(file->fd, cmd, &fl) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

int zfo_unlock(zfo_file_t* file, zfo_off_t offset, zfo_off_t length) {
    if (!file) return ZFO_ERR_INVALID_ARG;

    struct flock fl = {
        .l_whence = SEEK_SET,
        .l_start = offset,
        .l_len = length,
        .l_type = F_UNLCK
    };

    return fcntl(file->fd, F_SETLK, &fl) == 0 ? ZFO_OK : errno_to_zfo(errno);
}

/* ============================================================
 * Atomic Operations
 * ============================================================ */

int zfo_atomic_write(const char* path, const void* buf, size_t size) {
    if (!path || !buf) return ZFO_ERR_INVALID_ARG;

    char tmppath[PATH_MAX];
    snprintf(tmppath, sizeof(tmppath), "%s.XXXXXX", path);

    int fd = mkstemp(tmppath);
    if (fd < 0) return errno_to_zfo(errno);

    size_t written = 0;
    while (written < size) {
        ssize_t n = write(fd, (const uint8_t*)buf + written, size - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            close(fd);
            unlink(tmppath);
            return errno_to_zfo(errno);
        }
        written += n;
    }

    fsync(fd);
    close(fd);

    if (rename(tmppath, path) != 0) {
        int err = errno;
        unlink(tmppath);
        return errno_to_zfo(err);
    }

    return ZFO_OK;
}

int zfo_atomic_update(const char* path, int (*callback)(zfo_file_t*, void*), void* userdata) {
    if (!path || !callback) return ZFO_ERR_INVALID_ARG;

    char tmppath[PATH_MAX];
    zfo_file_t* file = zfo_tmpfile("atomic", tmppath);
    if (!file) return ZFO_ERR_IO;

    int ret = callback(file, userdata);
    if (ret != 0) {
        zfo_close(file);
        unlink(tmppath);
        return ret;
    }

    zfo_sync(file);
    zfo_close(file);

    if (rename(tmppath, path) != 0) {
        int err = errno;
        unlink(tmppath);
        return errno_to_zfo(err);
    }

    return ZFO_OK;
}

/* ============================================================
 * Glob/Pattern Matching
 * ============================================================ */

int zfo_glob(const char* pattern, char*** out_paths, size_t* out_count) {
    if (!pattern || !out_paths || !out_count) return ZFO_ERR_INVALID_ARG;

    glob_t gl;
    int ret = glob(pattern, GLOB_TILDE | GLOB_BRACE, NULL, &gl);

    if (ret == GLOB_NOMATCH) {
        *out_paths = NULL;
        *out_count = 0;
        return ZFO_OK;
    }

    if (ret != 0) return ZFO_ERR_IO;

    char** paths = malloc(gl.gl_pathc * sizeof(char*));
    if (!paths) {
        globfree(&gl);
        return ZFO_ERR_NO_MEMORY;
    }

    for (size_t i = 0; i < gl.gl_pathc; i++) {
        paths[i] = strdup(gl.gl_pathv[i]);
    }

    *out_paths = paths;
    *out_count = gl.gl_pathc;

    globfree(&gl);
    return ZFO_OK;
}

void zfo_glob_free(char** paths, size_t count) {
    if (!paths) return;
    for (size_t i = 0; i < count; i++) {
        free(paths[i]);
    }
    free(paths);
}

bool zfo_match(const char* pattern, const char* filename) {
    if (!pattern || !filename) return false;
    return fnmatch(pattern, filename, FNM_PATHNAME) == 0;
}

/* ============================================================
 * Disk Space
 * ============================================================ */

int zfo_diskspace(const char* path, zfo_space_t* space) {
    if (!path || !space) return ZFO_ERR_INVALID_ARG;

    struct statvfs st;
    if (statvfs(path, &st) != 0) return errno_to_zfo(errno);

    space->total = (uint64_t)st.f_blocks * st.f_frsize;
    space->free = (uint64_t)st.f_bfree * st.f_frsize;
    space->available = (uint64_t)st.f_bavail * st.f_frsize;

    return ZFO_OK;
}
