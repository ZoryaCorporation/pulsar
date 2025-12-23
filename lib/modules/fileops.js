/**
 * @file fileops.ts
 * @brief Pulsar FileOps Module - Native File Operations
 *
 * Comprehensive file I/O operations including:
 * - Reading/writing files
 * - Directory operations
 * - File watching (inotify)
 * - Glob patterns
 * - Memory mapping
 * - Symlinks & permissions
 *
 * @author Anthony Taliento
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * @example
 * ```typescript
 * import { fileops } from '@aspect/pulsar';
 *
 * // Read/write
 * const data = fileops.readFile('/path/to/file');
 * fileops.writeFile('/path/to/output', data);
 *
 * // Watch
 * const watcher = fileops.watch('/path/to/dir');
 * const events = watcher.poll(1000);
 * ```
 */
import { createRequire } from 'module';
import { join as pathJoin, dirname as pathDirname } from 'path';
import { fileURLToPath } from 'url';
const __dirname = pathDirname(fileURLToPath(import.meta.url));
const require = createRequire(import.meta.url);
/* Load native module */
const NATIVE_PATH = pathJoin(__dirname, '..', 'native', 'pulsar_fileops.node');
const native = require(NATIVE_PATH);
/* ============================================================
 * Types
 * ============================================================ */
export var FileType;
(function (FileType) {
    FileType[FileType["Unknown"] = 0] = "Unknown";
    FileType[FileType["File"] = 1] = "File";
    FileType[FileType["Directory"] = 2] = "Directory";
    FileType[FileType["Symlink"] = 3] = "Symlink";
    FileType[FileType["Fifo"] = 4] = "Fifo";
    FileType[FileType["Socket"] = 5] = "Socket";
    FileType[FileType["CharDevice"] = 6] = "CharDevice";
    FileType[FileType["BlockDevice"] = 7] = "BlockDevice";
})(FileType || (FileType = {}));
/* ============================================================
 * File Operations
 * ============================================================ */
/**
 * Read entire file into buffer
 */
export function readFile(path) {
    return native.readFile(path);
}
/**
 * Read file as UTF-8 text
 */
export function readText(path) {
    return native.readFile(path).toString('utf-8');
}
/**
 * Write buffer to file
 */
export function writeFile(path, data) {
    const buf = typeof data === 'string' ? Buffer.from(data, 'utf-8') : data;
    native.writeFile(path, buf);
}
/**
 * Append to file
 */
export function appendFile(path, data) {
    const buf = typeof data === 'string' ? Buffer.from(data, 'utf-8') : data;
    native.appendFile(path, buf);
}
/**
 * Copy file
 */
export function copyFile(src, dst) {
    native.copyFile(src, dst);
}
/**
 * Move/rename file
 */
export function moveFile(src, dst) {
    native.moveFile(src, dst);
}
/**
 * Remove file
 */
export function remove(path) {
    native.remove(path);
}
/**
 * Remove directory and contents recursively
 */
export function removeRecursive(path) {
    native.removeRecursive(path);
}
/* ============================================================
 * Stat Operations
 * ============================================================ */
/**
 * Get file/directory stats
 */
export function stat(path) {
    const st = native.stat(path);
    return {
        ...st,
        atime: new Date(st.atime * 1000),
        mtime: new Date(st.mtime * 1000),
        ctime: new Date(st.ctime * 1000),
    };
}
/**
 * Get stats without following symlinks
 */
export function lstat(path) {
    const st = native.lstat(path);
    return {
        ...st,
        atime: new Date(st.atime * 1000),
        mtime: new Date(st.mtime * 1000),
        ctime: new Date(st.ctime * 1000),
    };
}
/**
 * Check if path exists
 */
export function exists(path) {
    return native.exists(path);
}
/**
 * Check if path is a file
 */
export function isFile(path) {
    return native.isFile(path);
}
/**
 * Check if path is a directory
 */
export function isDirectory(path) {
    return native.isDirectory(path);
}
/**
 * Check if path is a symlink
 */
export function isSymlink(path) {
    return native.isSymlink(path);
}
/**
 * Get file size in bytes
 */
export function fileSize(path) {
    return native.fileSize(path);
}
/* ============================================================
 * Directory Operations
 * ============================================================ */
/**
 * Create directory
 * @param path Directory path
 * @param recursive Create parent directories if needed
 */
export function mkdir(path, recursive = false) {
    native.mkdir(path, recursive);
}
/**
 * Create directory with parents (alias for mkdir(path, true))
 */
export function mkdirp(path) {
    native.mkdir(path, true);
}
/**
 * Read directory contents
 */
export function readdir(path) {
    return native.readdir(path);
}
/**
 * Read directory with file type info
 */
export function readdirWithTypes(path) {
    return native.readdirWithTypes(path);
}
/* ============================================================
 * Path Operations
 * ============================================================ */
/**
 * Get basename of path
 */
export function basename(path) {
    return native.basename(path);
}
/**
 * Get directory name of path
 */
export function dirname(path) {
    return native.dirname(path);
}
/**
 * Get file extension
 */
export function extname(path) {
    return native.extname(path);
}
/**
 * Join path segments
 */
export function joinPath(base, ...paths) {
    let result = base;
    for (const p of paths) {
        result = native.join(result, p);
    }
    return result;
}
/**
 * Normalize path (resolve . and ..)
 */
export function normalize(path) {
    return native.normalize(path);
}
/**
 * Resolve to absolute path
 */
export function resolve(path) {
    return native.resolve(path);
}
/**
 * Check if path is absolute
 */
export function isAbsolute(path) {
    return native.isAbsolute(path);
}
/* ============================================================
 * Symlink Operations
 * ============================================================ */
/**
 * Create symbolic link
 */
export function symlink(target, linkpath) {
    native.symlink(target, linkpath);
}
/**
 * Read symlink target
 */
export function readlink(path) {
    return native.readlink(path);
}
/* ============================================================
 * System Paths
 * ============================================================ */
/**
 * Get temp directory
 */
export function tmpdir() {
    return native.tmpdir();
}
/**
 * Get home directory
 */
export function homedir() {
    return native.homedir();
}
/**
 * Get current working directory
 */
export function cwd() {
    return native.cwd();
}
/**
 * Change working directory
 */
export function chdir(path) {
    native.chdir(path);
}
/* ============================================================
 * Permissions
 * ============================================================ */
/**
 * Change file mode
 */
export function chmod(path, mode) {
    native.chmod(path, mode);
}
/**
 * Change file owner
 */
export function chown(path, uid, gid) {
    native.chown(path, uid, gid);
}
/* ============================================================
 * Glob
 * ============================================================ */
/**
 * Find files matching glob pattern
 */
export function glob(pattern) {
    return native.glob(pattern);
}
/* ============================================================
 * File Watcher
 * ============================================================ */
/**
 * File system watcher using inotify
 */
export class Watcher {
    watches = new Map();
    initialized = false;
    constructor() {
        native.watchInit();
        this.initialized = true;
    }
    /**
     * Add path to watch
     */
    add(path) {
        if (!this.initialized)
            throw new Error('Watcher closed');
        const wd = native.watchAdd(path);
        this.watches.set(wd, path);
        return wd;
    }
    /**
     * Remove watch
     */
    remove(wd) {
        if (!this.initialized)
            throw new Error('Watcher closed');
        native.watchRemove(wd);
        this.watches.delete(wd);
    }
    /**
     * Poll for events
     * @param timeout Timeout in ms (0 = non-blocking)
     */
    poll(timeout = 0) {
        if (!this.initialized)
            throw new Error('Watcher closed');
        return native.watchPoll(timeout);
    }
    /**
     * Get path for watch descriptor
     */
    getPath(wd) {
        return this.watches.get(wd);
    }
    /**
     * Close watcher
     */
    close() {
        if (this.initialized) {
            native.watchClose();
            this.watches.clear();
            this.initialized = false;
        }
    }
}
/**
 * Create file watcher
 */
export function watch(path) {
    const watcher = new Watcher();
    if (path)
        watcher.add(path);
    return watcher;
}
/**
 * Get version
 */
export function version() {
    return native.version();
}
export default {
    readFile,
    readText,
    writeFile,
    appendFile,
    copyFile,
    moveFile,
    remove,
    removeRecursive,
    stat,
    lstat,
    exists,
    isFile,
    isDirectory,
    isSymlink,
    fileSize,
    mkdir,
    mkdirp,
    readdir,
    readdirWithTypes,
    basename,
    dirname,
    extname,
    joinPath,
    normalize,
    resolve,
    isAbsolute,
    symlink,
    readlink,
    tmpdir,
    homedir,
    cwd,
    chdir,
    chmod,
    chown,
    glob,
    Watcher,
    watch,
    version,
    FileType,
};
//# sourceMappingURL=fileops.js.map