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

export enum FileType {
  Unknown = 0,
  File = 1,
  Directory = 2,
  Symlink = 3,
  Fifo = 4,
  Socket = 5,
  CharDevice = 6,
  BlockDevice = 7,
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

/* ============================================================
 * File Operations
 * ============================================================ */

/**
 * Read entire file into buffer
 */
export function readFile(path: string): Buffer {
  return native.readFile(path);
}

/**
 * Read file as UTF-8 text
 */
export function readText(path: string): string {
  return native.readFile(path).toString('utf-8');
}

/**
 * Write buffer to file
 */
export function writeFile(path: string, data: Buffer | string): void {
  const buf = typeof data === 'string' ? Buffer.from(data, 'utf-8') : data;
  native.writeFile(path, buf);
}

/**
 * Append to file
 */
export function appendFile(path: string, data: Buffer | string): void {
  const buf = typeof data === 'string' ? Buffer.from(data, 'utf-8') : data;
  native.appendFile(path, buf);
}

/**
 * Copy file
 */
export function copyFile(src: string, dst: string): void {
  native.copyFile(src, dst);
}

/**
 * Move/rename file
 */
export function moveFile(src: string, dst: string): void {
  native.moveFile(src, dst);
}

/**
 * Remove file
 */
export function remove(path: string): void {
  native.remove(path);
}

/**
 * Remove directory and contents recursively
 */
export function removeRecursive(path: string): void {
  native.removeRecursive(path);
}

/* ============================================================
 * Stat Operations
 * ============================================================ */

/**
 * Get file/directory stats
 */
export function stat(path: string): StatResult {
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
export function lstat(path: string): StatResult {
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
export function exists(path: string): boolean {
  return native.exists(path);
}

/**
 * Check if path is a file
 */
export function isFile(path: string): boolean {
  return native.isFile(path);
}

/**
 * Check if path is a directory
 */
export function isDirectory(path: string): boolean {
  return native.isDirectory(path);
}

/**
 * Check if path is a symlink
 */
export function isSymlink(path: string): boolean {
  return native.isSymlink(path);
}

/**
 * Get file size in bytes
 */
export function fileSize(path: string): number {
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
export function mkdir(path: string, recursive: boolean = false): void {
  native.mkdir(path, recursive);
}

/**
 * Create directory with parents (alias for mkdir(path, true))
 */
export function mkdirp(path: string): void {
  native.mkdir(path, true);
}

/**
 * Read directory contents
 */
export function readdir(path: string): string[] {
  return native.readdir(path);
}

/**
 * Read directory with file type info
 */
export function readdirWithTypes(path: string): DirEntry[] {
  return native.readdirWithTypes(path);
}

/* ============================================================
 * Path Operations
 * ============================================================ */

/**
 * Get basename of path
 */
export function basename(path: string): string {
  return native.basename(path);
}

/**
 * Get directory name of path
 */
export function dirname(path: string): string {
  return native.dirname(path);
}

/**
 * Get file extension
 */
export function extname(path: string): string {
  return native.extname(path);
}

/**
 * Join path segments
 */
export function joinPath(base: string, ...paths: string[]): string {
  let result = base;
  for (const p of paths) {
    result = native.join(result, p);
  }
  return result;
}

/**
 * Normalize path (resolve . and ..)
 */
export function normalize(path: string): string {
  return native.normalize(path);
}

/**
 * Resolve to absolute path
 */
export function resolve(path: string): string {
  return native.resolve(path);
}

/**
 * Check if path is absolute
 */
export function isAbsolute(path: string): boolean {
  return native.isAbsolute(path);
}

/* ============================================================
 * Symlink Operations
 * ============================================================ */

/**
 * Create symbolic link
 */
export function symlink(target: string, linkpath: string): void {
  native.symlink(target, linkpath);
}

/**
 * Read symlink target
 */
export function readlink(path: string): string {
  return native.readlink(path);
}

/* ============================================================
 * System Paths
 * ============================================================ */

/**
 * Get temp directory
 */
export function tmpdir(): string {
  return native.tmpdir();
}

/**
 * Get home directory
 */
export function homedir(): string {
  return native.homedir();
}

/**
 * Get current working directory
 */
export function cwd(): string {
  return native.cwd();
}

/**
 * Change working directory
 */
export function chdir(path: string): void {
  native.chdir(path);
}

/* ============================================================
 * Permissions
 * ============================================================ */

/**
 * Change file mode
 */
export function chmod(path: string, mode: number): void {
  native.chmod(path, mode);
}

/**
 * Change file owner
 */
export function chown(path: string, uid: number, gid: number): void {
  native.chown(path, uid, gid);
}

/* ============================================================
 * Glob
 * ============================================================ */

/**
 * Find files matching glob pattern
 */
export function glob(pattern: string): string[] {
  return native.glob(pattern);
}

/* ============================================================
 * File Watcher
 * ============================================================ */

/**
 * File system watcher using inotify
 */
export class Watcher {
  private watches = new Map<number, string>();
  private initialized = false;

  constructor() {
    native.watchInit();
    this.initialized = true;
  }

  /**
   * Add path to watch
   */
  add(path: string): number {
    if (!this.initialized) throw new Error('Watcher closed');
    const wd = native.watchAdd(path);
    this.watches.set(wd, path);
    return wd;
  }

  /**
   * Remove watch
   */
  remove(wd: number): void {
    if (!this.initialized) throw new Error('Watcher closed');
    native.watchRemove(wd);
    this.watches.delete(wd);
  }

  /**
   * Poll for events
   * @param timeout Timeout in ms (0 = non-blocking)
   */
  poll(timeout: number = 0): WatchEvent[] {
    if (!this.initialized) throw new Error('Watcher closed');
    return native.watchPoll(timeout);
  }

  /**
   * Get path for watch descriptor
   */
  getPath(wd: number): string | undefined {
    return this.watches.get(wd);
  }

  /**
   * Close watcher
   */
  close(): void {
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
export function watch(path?: string): Watcher {
  const watcher = new Watcher();
  if (path) watcher.add(path);
  return watcher;
}

/**
 * Get version
 */
export function version(): string {
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
