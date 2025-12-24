# FileOps Module Guide

> **Native File Operations at Native Speed**  
> Read, write, watch, and manipulate files with maximum efficiency

The FileOps module provides comprehensive file system operations built on native code. From simple reads to file watching with inotify, it delivers 5-10x speedup over Node.js built-in `fs` module for common operations. FileOps is ideal for applications that require high-performance file I/O covering areas that are ordinarily challenging in a javaScript environment. In this guide we will cover the FileOps API, features and usage patterns. 

Some noteworthy features of the FileOps module brings previously unavailable capabilities to Node.js: mkdir, path manipulation, globbing, and inotify-based file watching on Linux. These features enable developers to build robust file-centric applications with ease. Currently there is a big lack of high-performance file system libraries for Node.js, and FileOps aims to fill that gap by providing a native solution that is both fast and reliable. There are some implementations of file based I/O which leverage libuv directly. It has always been a challenging task building a application that requires robust file system operations in Node.js. Developers often resort to using child processes, different languages like Rust, C++ or Go to work around these limitaitons. This of course means managing multiple runtimes, build systems, and deployment targets. Maintaining a mature, multi-language codebase is a significant burden for any development team. 

We feel your pain, we understand the challenges, and we are here to help.

Below is a comprehensive guide to using the FileOps module effectively in your Node.js applications.

---

## Overview

| Category | Operations |
|----------|------------|
| **File I/O** | read, write, append, copy, move, remove |
| **Stats** | stat, lstat, exists, isFile, isDirectory |
| **Directories** | mkdir, readdir, glob |
| **Paths** | join, resolve, basename, dirname, extname |
| **System** | tmpdir, homedir, cwd, chdir |
| **Watching** | inotify-based file watching |
| **Permissions** | chmod, chown, symlink, readlink |

---

## Quick Start

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Read a file
const data = fileops.readFile('/etc/hostname');
console.log(data.toString());

// Write a file
fileops.writeFile('/tmp/hello.txt', Buffer.from('Hello, Pulsar!'));

// Check if exists
if (fileops.exists('/tmp/hello.txt')) {
  console.log('File created!');
}
```

---

## Reading Files

### Read as Buffer

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Read entire file into buffer
const buffer = fileops.readFile('/path/to/file');

console.log(buffer.length); // Size in bytes
console.log(buffer);        // <Buffer ...>
```

### Read as Text

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Read directly as UTF-8 text
const text = fileops.readText('/path/to/file.txt');

console.log(text); // String content
```

### Reading Large Files

```typescript
// FileOps handles large files efficiently
const largeFile = fileops.readFile('/path/to/large-file.bin');
console.log(`Read ${largeFile.length} bytes`);
```

---

## Writing Files

### Write Buffer

```typescript
import { fileops } from '@zoryacorporation/pulsar';

const data = Buffer.from([0x48, 0x65, 0x6c, 0x6c, 0x6f]);
fileops.writeFile('/tmp/binary.bin', data);
```

### Write Text

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Strings are automatically converted to UTF-8
fileops.writeFile('/tmp/hello.txt', 'Hello, World!');

// Multi-line content
const content = `
Line 1
Line 2
Line 3
`;
fileops.writeFile('/tmp/multiline.txt', content);
```

### Append to File

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Append data to existing file
fileops.appendFile('/tmp/log.txt', 'New log entry\n');
fileops.appendFile('/tmp/log.txt', 'Another entry\n');
```

---

## File Operations

### Copy Files

```typescript
import { fileops } from '@zoryacorporation/pulsar';

fileops.copyFile('/source/file.txt', '/destination/file.txt');
```

### Move/Rename Files

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Move file (works across filesystems)
fileops.moveFile('/tmp/old-name.txt', '/tmp/new-name.txt');

// Move to different directory
fileops.moveFile('/tmp/file.txt', '/home/user/file.txt');
```

### Delete Files

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Remove single file
fileops.remove('/tmp/file.txt');

// Remove directory and all contents
fileops.removeRecursive('/tmp/my-directory');
```

---

## File Information

### Basic Stat

```typescript
import { fileops } from '@zoryacorporation/pulsar';

const stat = fileops.stat('/path/to/file');

console.log({
  size: stat.size,           // File size in bytes
  isFile: stat.isFile,       // true if regular file
  isDirectory: stat.isDirectory,
  isSymlink: stat.isSymlink,
  mode: stat.mode,           // Unix permissions
  uid: stat.uid,             // Owner user ID
  gid: stat.gid,             // Owner group ID
  atime: stat.atime,         // Last access time (Date)
  mtime: stat.mtime,         // Last modification time (Date)
  ctime: stat.ctime,         // Last status change time (Date)
  ino: stat.ino,             // Inode number
  nlink: stat.nlink          // Number of hard links
});
```

### Stat Without Following Symlinks

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// lstat doesn't follow symlinks
const linkStat = fileops.lstat('/path/to/symlink');
console.log(linkStat.isSymlink); // true
```

### Quick Checks

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Check existence
if (fileops.exists('/path/to/file')) {
  // File exists
}

// Check type
if (fileops.isFile('/path/to/file')) {
  // It's a regular file
}

if (fileops.isDirectory('/path/to/dir')) {
  // It's a directory
}

if (fileops.isSymlink('/path/to/link')) {
  // It's a symbolic link
}

// Get file size directly
const size = fileops.fileSize('/path/to/file');
```

---

## Directory Operations

### Create Directories

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Create single directory
fileops.mkdir('/tmp/new-dir');

// Create with parents (like mkdir -p)
fileops.mkdir('/tmp/a/b/c/d', true);

// Or use the convenient alias
fileops.mkdirp('/tmp/x/y/z');
```

### List Directory Contents

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Get names only
const names = fileops.readdir('/path/to/dir');
console.log(names); // ['file1.txt', 'file2.txt', 'subdir']

// Get names with type information
const entries = fileops.readdirWithTypes('/path/to/dir');
for (const entry of entries) {
  console.log({
    name: entry.name,
    isFile: entry.isFile,
    isDirectory: entry.isDirectory,
    isSymlink: entry.isSymlink,
    type: entry.type
  });
}
```

### Glob Patterns

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Find files matching pattern
const txtFiles = fileops.glob('/path/to/dir/*.txt');
const allJs = fileops.glob('/project/**/*.js');
const configs = fileops.glob('/etc/*.conf');

console.log(txtFiles); // ['/path/to/dir/a.txt', '/path/to/dir/b.txt']
```

---

## Path Operations

### Components

```typescript
import { fileops } from '@zoryacorporation/pulsar';

const path = '/home/user/documents/report.pdf';

console.log(fileops.basename(path)); // 'report.pdf'
console.log(fileops.dirname(path));  // '/home/user/documents'
console.log(fileops.extname(path));  // '.pdf'
```

### Joining Paths

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Join path segments
const full = fileops.joinPath('/home', 'user', 'documents', 'file.txt');
console.log(full); // '/home/user/documents/file.txt'
```

### Normalization

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Normalize path (resolve . and ..)
const messy = '/home/user/../user/./documents//file.txt';
const clean = fileops.normalize(messy);
console.log(clean); // '/home/user/documents/file.txt'
```

### Absolute Paths

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Resolve to absolute path
const relative = './config/app.json';
const absolute = fileops.resolve(relative);
console.log(absolute); // '/current/working/dir/config/app.json'

// Check if path is absolute
console.log(fileops.isAbsolute('/etc/hosts')); // true
console.log(fileops.isAbsolute('./file.txt')); // false
```

---

## System Paths

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Temporary directory
console.log(fileops.tmpdir()); // '/tmp' or equivalent

// Home directory
console.log(fileops.homedir()); // '/home/user'

// Current working directory
console.log(fileops.cwd()); // '/current/directory'

// Change working directory
fileops.chdir('/new/directory');
```

---

## Symbolic Links

### Create Symlinks

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Create symbolic link
fileops.symlink('/path/to/target', '/path/to/link');
```

### Read Symlink Target

```typescript
import { fileops } from '@zoryacorporation/pulsar';

const target = fileops.readlink('/path/to/symlink');
console.log(target); // '/path/to/target'
```

---

## Permissions

### Change Mode

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Make file executable
fileops.chmod('/path/to/script.sh', 0o755);

// Read-only
fileops.chmod('/path/to/file', 0o444);

// Standard file
fileops.chmod('/path/to/file', 0o644);
```

### Change Owner

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Change owner (requires appropriate permissions)
fileops.chown('/path/to/file', uid, gid);
```

---

## File Watching

FileOps provides inotify-based file watching on Linux for real-time file system notifications.

### Basic Watching

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Create watcher
const watcher = fileops.watch('/path/to/watch');

// Or create watcher and add paths manually
const watcher2 = fileops.watch();
watcher2.add('/path/to/dir1');
watcher2.add('/path/to/dir2');
```

### Polling for Events

```typescript
import { fileops } from '@zoryacorporation/pulsar';

const watcher = fileops.watch('/path/to/dir');

// Non-blocking poll
const events = watcher.poll(0);

// Blocking poll with timeout (ms)
const eventsWithTimeout = watcher.poll(1000);

for (const event of events) {
  console.log({
    path: event.path,
    isCreate: event.isCreate,
    isDelete: event.isDelete,
    isModify: event.isModify,
    isMove: event.isMove,
    isDir: event.isDir,
    oldPath: event.oldPath  // For moves
  });
}
```

### Watch Loop

```typescript
import { fileops } from '@zoryacorporation/pulsar';

const watcher = fileops.watch('/path/to/watch');

// Process events in a loop
function watchLoop() {
  const events = watcher.poll(100); // 100ms timeout
  
  for (const event of events) {
    if (event.isCreate) {
      console.log(`Created: ${event.path}`);
    } else if (event.isDelete) {
      console.log(`Deleted: ${event.path}`);
    } else if (event.isModify) {
      console.log(`Modified: ${event.path}`);
    } else if (event.isMove) {
      console.log(`Moved: ${event.oldPath} → ${event.path}`);
    }
  }
  
  // Continue watching
  setTimeout(watchLoop, 0);
}

watchLoop();

// Clean up when done
// watcher.close();
```

### Watcher Class

```typescript
import { fileops } from '@zoryacorporation/pulsar';

const watcher = new fileops.Watcher();

// Add multiple paths
const wd1 = watcher.add('/path/to/dir1');
const wd2 = watcher.add('/path/to/dir2');

// Get path for watch descriptor
console.log(watcher.getPath(wd1)); // '/path/to/dir1'

// Remove a watch
watcher.remove(wd1);

// Close watcher
watcher.close();
```

---

## Common Use Cases

### Config File Loader

```typescript
import { fileops } from '@zoryacorporation/pulsar';

function loadConfig(path: string): object {
  if (!fileops.exists(path)) {
    throw new Error(`Config not found: ${path}`);
  }
  
  const text = fileops.readText(path);
  
  if (path.endsWith('.json')) {
    return JSON.parse(text);
  }
  
  throw new Error(`Unsupported config format: ${path}`);
}
```

### Safe File Writing

```typescript
import { fileops } from '@zoryacorporation/pulsar';

function safeWrite(path: string, data: string): void {
  const tempPath = `${path}.tmp`;
  const backupPath = `${path}.bak`;
  
  // Write to temp file
  fileops.writeFile(tempPath, data);
  
  // Backup existing file if present
  if (fileops.exists(path)) {
    if (fileops.exists(backupPath)) {
      fileops.remove(backupPath);
    }
    fileops.moveFile(path, backupPath);
  }
  
  // Move temp to final location
  fileops.moveFile(tempPath, path);
}
```

### Directory Tree Walker

```typescript
import { fileops } from '@zoryacorporation/pulsar';

function* walkDir(dir: string): Generator<string> {
  const entries = fileops.readdirWithTypes(dir);
  
  for (const entry of entries) {
    const fullPath = fileops.joinPath(dir, entry.name);
    
    if (entry.isDirectory) {
      yield* walkDir(fullPath);
    } else if (entry.isFile) {
      yield fullPath;
    }
  }
}

// Usage
for (const file of walkDir('/path/to/project')) {
  console.log(file);
}
```

### Find Files by Extension

```typescript
import { fileops } from '@zoryacorporation/pulsar';

function findByExtension(dir: string, ext: string): string[] {
  const results: string[] = [];
  
  function search(current: string) {
    const entries = fileops.readdirWithTypes(current);
    
    for (const entry of entries) {
      const fullPath = fileops.joinPath(current, entry.name);
      
      if (entry.isDirectory) {
        search(fullPath);
      } else if (entry.isFile && entry.name.endsWith(ext)) {
        results.push(fullPath);
      }
    }
  }
  
  search(dir);
  return results;
}

// Find all TypeScript files
const tsFiles = findByExtension('/path/to/project', '.ts');
```

### Hot Reload Watcher

```typescript
import { fileops } from '@zoryacorporation/pulsar';

class HotReloader {
  private watcher: fileops.Watcher;
  private callbacks = new Map<string, () => void>();
  
  constructor() {
    this.watcher = new fileops.Watcher();
    this.startPolling();
  }
  
  watch(path: string, callback: () => void): void {
    this.watcher.add(path);
    this.callbacks.set(path, callback);
  }
  
  private startPolling(): void {
    const poll = () => {
      const events = this.watcher.poll(100);
      
      for (const event of events) {
        if (event.isModify) {
          const callback = this.callbacks.get(event.path);
          if (callback) {
            console.log(`Reloading: ${event.path}`);
            callback();
          }
        }
      }
      
      setTimeout(poll, 0);
    };
    
    poll();
  }
  
  close(): void {
    this.watcher.close();
  }
}

// Usage
const reloader = new HotReloader();
reloader.watch('/path/to/config.json', () => {
  // Reload config
});
```

### Backup Utility

```typescript
import { fileops, compress } from '@zoryacorporation/pulsar';

function backup(sourceDir: string, backupDir: string): void {
  const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
  const backupPath = fileops.joinPath(backupDir, `backup-${timestamp}`);
  
  fileops.mkdirp(backupPath);
  
  function copyRecursive(src: string, dst: string) {
    const entries = fileops.readdirWithTypes(src);
    
    for (const entry of entries) {
      const srcPath = fileops.joinPath(src, entry.name);
      const dstPath = fileops.joinPath(dst, entry.name);
      
      if (entry.isDirectory) {
        fileops.mkdir(dstPath);
        copyRecursive(srcPath, dstPath);
      } else if (entry.isFile) {
        // Compress while copying
        const data = fileops.readFile(srcPath);
        const compressed = compress.zstd.compress(data, 9);
        fileops.writeFile(`${dstPath}.zst`, compressed);
      }
    }
  }
  
  copyRecursive(sourceDir, backupPath);
  console.log(`Backup created: ${backupPath}`);
}
```

---

## FileType Enum

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// FileType values
fileops.FileType.Unknown     // 0
fileops.FileType.File        // 1
fileops.FileType.Directory   // 2
fileops.FileType.Symlink     // 3
fileops.FileType.Fifo        // 4
fileops.FileType.Socket      // 5
fileops.FileType.CharDevice  // 6
fileops.FileType.BlockDevice // 7
```

---

## Error Handling

```typescript
import { fileops } from '@zoryacorporation/pulsar';

function safeRead(path: string): Buffer | null {
  try {
    return fileops.readFile(path);
  } catch (error) {
    console.error(`Failed to read ${path}:`, error);
    return null;
  }
}

function ensureDirectory(path: string): boolean {
  try {
    if (!fileops.exists(path)) {
      fileops.mkdirp(path);
    }
    return true;
  } catch (error) {
    console.error(`Failed to create ${path}:`, error);
    return false;
  }
}
```

---

## Performance Tips

### 1. Read Once, Process Multiple Times

```typescript
// ❌ Reading file multiple times
const size = fileops.fileSize('/path/to/file');
const data = fileops.readFile('/path/to/file');

// ✅ Read once
const data = fileops.readFile('/path/to/file');
const size = data.length;
```

### 2. Use Glob for Patterns

```typescript
// ❌ Filtering manually
const all = fileops.readdir('/path');
const filtered = all.filter(f => f.endsWith('.txt'));

// ✅ Use glob
const filtered = fileops.glob('/path/*.txt');
```

### 3. Batch Directory Operations

```typescript
// ❌ Many separate calls
for (const name of names) {
  fileops.mkdir(`/path/${name}`);
}

// ✅ Use mkdirp for nested
fileops.mkdirp('/path/a/b/c/d');
```

---

## API Reference

### File I/O

| Function | Description |
|----------|-------------|
| `readFile(path)` | Read file as Buffer |
| `readText(path)` | Read file as UTF-8 string |
| `writeFile(path, data)` | Write Buffer or string |
| `appendFile(path, data)` | Append to file |
| `copyFile(src, dst)` | Copy file |
| `moveFile(src, dst)` | Move/rename file |
| `remove(path)` | Delete file |
| `removeRecursive(path)` | Delete directory recursively |

### Stats

| Function | Description |
|----------|-------------|
| `stat(path)` | Get file stats |
| `lstat(path)` | Get stats (no symlink follow) |
| `exists(path)` | Check if exists |
| `isFile(path)` | Check if regular file |
| `isDirectory(path)` | Check if directory |
| `isSymlink(path)` | Check if symlink |
| `fileSize(path)` | Get size in bytes |

### Directories

| Function | Description |
|----------|-------------|
| `mkdir(path, recursive?)` | Create directory |
| `mkdirp(path)` | Create with parents |
| `readdir(path)` | List contents (names only) |
| `readdirWithTypes(path)` | List contents with types |
| `glob(pattern)` | Find matching files |

### Paths

| Function | Description |
|----------|-------------|
| `basename(path)` | Get filename |
| `dirname(path)` | Get directory |
| `extname(path)` | Get extension |
| `joinPath(base, ...paths)` | Join segments |
| `normalize(path)` | Clean up path |
| `resolve(path)` | Get absolute path |
| `isAbsolute(path)` | Check if absolute |

### System

| Function | Description |
|----------|-------------|
| `tmpdir()` | Get temp directory |
| `homedir()` | Get home directory |
| `cwd()` | Get current directory |
| `chdir(path)` | Change directory |

### Symlinks & Permissions

| Function | Description |
|----------|-------------|
| `symlink(target, link)` | Create symlink |
| `readlink(path)` | Read symlink target |
| `chmod(path, mode)` | Change permissions |
| `chown(path, uid, gid)` | Change owner |

### Watcher

| Method | Description |
|--------|-------------|
| `watch(path?)` | Create watcher |
| `Watcher.add(path)` | Add path to watch |
| `Watcher.remove(wd)` | Remove watch |
| `Watcher.poll(timeout)` | Get events |
| `Watcher.getPath(wd)` | Get path for descriptor |
| `Watcher.close()` | Close watcher |

---

## Next Steps

- [Compress Module](./compress.md) - Compress files you read
- [Hash Module](./hash.md) - Hash file contents
- [INI Module](./ini.md) - Parse configuration files
