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
export declare enum FileType {
    Unknown = 0,
    File = 1,
    Directory = 2,
    Symlink = 3,
    Fifo = 4,
    Socket = 5,
    CharDevice = 6,
    BlockDevice = 7
}
export interface StatResult {
    size: number;
    atime: Date;
    mtime: Date;
    ctime: Date;
    mode: number;
    uid: number;
    gid: number;
    ino: number;
    nlink: number;
    type: FileType;
    isFile: boolean;
    isDirectory: boolean;
    isSymlink: boolean;
}
export interface DirEntry {
    name: string;
    type: FileType;
    isFile: boolean;
    isDirectory: boolean;
    isSymlink: boolean;
}
export interface WatchEvent {
    event: number;
    path: string;
    oldPath: string;
    isDir: boolean;
    cookie: number;
    isCreate: boolean;
    isDelete: boolean;
    isModify: boolean;
    isMove: boolean;
}
/**
 * Read entire file into buffer
 */
export declare function readFile(path: string): Buffer;
/**
 * Read file as UTF-8 text
 */
export declare function readText(path: string): string;
/**
 * Write buffer to file
 */
export declare function writeFile(path: string, data: Buffer | string): void;
/**
 * Append to file
 */
export declare function appendFile(path: string, data: Buffer | string): void;
/**
 * Copy file
 */
export declare function copyFile(src: string, dst: string): void;
/**
 * Move/rename file
 */
export declare function moveFile(src: string, dst: string): void;
/**
 * Remove file
 */
export declare function remove(path: string): void;
/**
 * Remove directory and contents recursively
 */
export declare function removeRecursive(path: string): void;
/**
 * Get file/directory stats
 */
export declare function stat(path: string): StatResult;
/**
 * Get stats without following symlinks
 */
export declare function lstat(path: string): StatResult;
/**
 * Check if path exists
 */
export declare function exists(path: string): boolean;
/**
 * Check if path is a file
 */
export declare function isFile(path: string): boolean;
/**
 * Check if path is a directory
 */
export declare function isDirectory(path: string): boolean;
/**
 * Check if path is a symlink
 */
export declare function isSymlink(path: string): boolean;
/**
 * Get file size in bytes
 */
export declare function fileSize(path: string): number;
/**
 * Create directory
 * @param path Directory path
 * @param recursive Create parent directories if needed
 */
export declare function mkdir(path: string, recursive?: boolean): void;
/**
 * Create directory with parents (alias for mkdir(path, true))
 */
export declare function mkdirp(path: string): void;
/**
 * Read directory contents
 */
export declare function readdir(path: string): string[];
/**
 * Read directory with file type info
 */
export declare function readdirWithTypes(path: string): DirEntry[];
/**
 * Get basename of path
 */
export declare function basename(path: string): string;
/**
 * Get directory name of path
 */
export declare function dirname(path: string): string;
/**
 * Get file extension
 */
export declare function extname(path: string): string;
/**
 * Join path segments
 */
export declare function joinPath(base: string, ...paths: string[]): string;
/**
 * Normalize path (resolve . and ..)
 */
export declare function normalize(path: string): string;
/**
 * Resolve to absolute path
 */
export declare function resolve(path: string): string;
/**
 * Check if path is absolute
 */
export declare function isAbsolute(path: string): boolean;
/**
 * Create symbolic link
 */
export declare function symlink(target: string, linkpath: string): void;
/**
 * Read symlink target
 */
export declare function readlink(path: string): string;
/**
 * Get temp directory
 */
export declare function tmpdir(): string;
/**
 * Get home directory
 */
export declare function homedir(): string;
/**
 * Get current working directory
 */
export declare function cwd(): string;
/**
 * Change working directory
 */
export declare function chdir(path: string): void;
/**
 * Change file mode
 */
export declare function chmod(path: string, mode: number): void;
/**
 * Change file owner
 */
export declare function chown(path: string, uid: number, gid: number): void;
/**
 * Find files matching glob pattern
 */
export declare function glob(pattern: string): string[];
/**
 * File system watcher using inotify
 */
export declare class Watcher {
    private watches;
    private initialized;
    constructor();
    /**
     * Add path to watch
     */
    add(path: string): number;
    /**
     * Remove watch
     */
    remove(wd: number): void;
    /**
     * Poll for events
     * @param timeout Timeout in ms (0 = non-blocking)
     */
    poll(timeout?: number): WatchEvent[];
    /**
     * Get path for watch descriptor
     */
    getPath(wd: number): string | undefined;
    /**
     * Close watcher
     */
    close(): void;
}
/**
 * Create file watcher
 */
export declare function watch(path?: string): Watcher;
/**
 * Get version
 */
export declare function version(): string;
declare const _default: {
    readFile: typeof readFile;
    readText: typeof readText;
    writeFile: typeof writeFile;
    appendFile: typeof appendFile;
    copyFile: typeof copyFile;
    moveFile: typeof moveFile;
    remove: typeof remove;
    removeRecursive: typeof removeRecursive;
    stat: typeof stat;
    lstat: typeof lstat;
    exists: typeof exists;
    isFile: typeof isFile;
    isDirectory: typeof isDirectory;
    isSymlink: typeof isSymlink;
    fileSize: typeof fileSize;
    mkdir: typeof mkdir;
    mkdirp: typeof mkdirp;
    readdir: typeof readdir;
    readdirWithTypes: typeof readdirWithTypes;
    basename: typeof basename;
    dirname: typeof dirname;
    extname: typeof extname;
    joinPath: typeof joinPath;
    normalize: typeof normalize;
    resolve: typeof resolve;
    isAbsolute: typeof isAbsolute;
    symlink: typeof symlink;
    readlink: typeof readlink;
    tmpdir: typeof tmpdir;
    homedir: typeof homedir;
    cwd: typeof cwd;
    chdir: typeof chdir;
    chmod: typeof chmod;
    chown: typeof chown;
    glob: typeof glob;
    Watcher: typeof Watcher;
    watch: typeof watch;
    version: typeof version;
    FileType: typeof FileType;
};
export default _default;
//# sourceMappingURL=fileops.d.ts.map