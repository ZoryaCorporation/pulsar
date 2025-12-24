# Getting Started with Pulsar

> **From Zero to High-Performance in 5 Minutes**  
> A complete guide to accelerating your Node.js applications

Welcome to Pulsar! This guide will take you from installation to building real-world high-performance applications using Pulsar's native modules.

---

## What is Pulsar?

Pulsar is a collection of native Node.js modules that provide 10-100x performance improvements for common operations:

| Module | Purpose | Speedup |
|--------|---------|---------|
| **weave** | String manipulation | 50x+ |
| **compress** | Zstd/LZ4 compression | N/A (not in JS) |
| **fileops** | File operations | 5-10x |
| **hash** | Non-crypto hashing | 56x |
| **dagger** | Hash tables | 7x |
| **pcm** | Bit manipulation | Hardware speed |
| **ini** | Config parsing | 10x+ |

All modules expose clean TypeScript APIs while running native C code underneath.

---

## Installation

### Requirements

- Node.js 18.0.0 or later
- Linux or macOS (x64 or arm64)
- C compiler (gcc or clang)
- Python 3 (for node-gyp)

### Install from npm

```bash
npm install @zoryacorporation/pulsar
```

That's it! The native modules are pre-compiled for common platforms.

### Build from Source

If you need to build from source:

```bash
git clone https://github.com/ZoryaCorporation/pulsar.git
cd pulsar
npm install
npm run build
npm test
```

---

## Your First Pulsar Program

Let's write a simple program that demonstrates Pulsar's capabilities:

```typescript
import { weave, compress, fileops, hash, dagger, ini } from '@zoryacorporation/pulsar';

// 1. String Building with Weave
const builder = weave.Weave.from('Hello');
builder.append(', Pulsar!');
console.log(builder.toString()); // "Hello, Pulsar!"

// 2. Fast Hashing
const h = hash.nxhString('hello world');
console.log(`Hash: ${h.toString(16)}`);

// 3. Compression
const data = Buffer.from('Hello!'.repeat(1000));
const compressed = compress.zstd.compress(data, 3);
console.log(`Compressed: ${data.length} → ${compressed.length} bytes`);

// 4. File Operations
const hostname = fileops.readText('/etc/hostname');
console.log(`Hostname: ${hostname.trim()}`);

// 5. Hash Table
const table = new dagger.DaggerTable();
table.set('name', 'Pulsar').set('version', '1.0.0');
console.log(`Name: ${table.get('name')}`);

console.log('Pulsar is ready!');
```

Run it:

```bash
npx tsx example.ts
```

---

## Module Overview

### weave - String Primitives

Four powerful string data structures:

```typescript
import { weave } from '@zoryacorporation/pulsar';

// Weave: Mutable string builder
const builder = weave.Weave.from('Hello');
builder.append(' World').replace('World', 'Pulsar');

// Cord: Rope for large documents
const doc = weave.Cord.create();
doc.append('Chapter 1\n').append('Content...\n');

// Tablet: String interning
const tablet = weave.Tablet.create();
const id = tablet.intern('repeated string'); // Same ID each time

// Arena: Temporary allocations
const arena = weave.Arena.create(4096);
const offset = arena.allocString('temporary');
arena.reset(); // Free all at once
```

[→ Weave Guide](./weave.md)

### compress - Compression

Zstandard and LZ4 compression:

```typescript
import { compress } from '@zoryacorporation/pulsar';

// Zstandard (best ratio)
const compressed = compress.zstd.compress(data, 3);
const original = compress.zstd.decompress(compressed);

// LZ4 (fastest)
const fast = compress.lz4.compress(data);
const restored = compress.lz4.decompress(fast);
```

[→ Compress Guide](./compress.md)

### fileops - File Operations

Native file I/O:

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Read/write
const data = fileops.readFile('/path/to/file');
fileops.writeFile('/path/to/output', data);

// Directory operations
const files = fileops.readdir('/path/to/dir');
fileops.mkdirp('/path/to/nested/dir');

// Glob patterns
const jsFiles = fileops.glob('/project/**/*.js');

// File watching
const watcher = fileops.watch('/path/to/watch');
const events = watcher.poll(1000);
```

[→ FileOps Guide](./fileops.md)

### hash - NXH Hashing

Fast non-cryptographic hashing:

```typescript
import { hash } from '@zoryacorporation/pulsar';

// Hash strings
const h = hash.nxhString('hello world');

// Hash buffers
const bufHash = hash.nxh64(buffer);

// Combine hashes
const combined = hash.nxhCombine(hash1, hash2);
```

[→ Hash Guide](./hash.md)

### dagger - Hash Tables

Robin Hood hash table:

```typescript
import { dagger } from '@zoryacorporation/pulsar';

const table = new dagger.DaggerTable();

table.set('key', 'value');
const value = table.get('key');

for (const [k, v] of table) {
  console.log(k, v);
}
```

[→ Dagger Guide](./dagger.md)

### pcm - Bit Manipulation

Hardware-accelerated operations:

```typescript
import { pcm } from '@zoryacorporation/pulsar';

const bits = pcm.popcount32(0xFF);        // Count set bits
const aligned = pcm.alignUp(100, 64);     // Align to boundary
const next = pcm.nextPowerOf2_32(100);    // Next power of 2
const swapped = pcm.bswap32(0x12345678);  // Endian swap
```

[→ PCM Guide](./pcm.md)

### ini - Configuration

INI file parsing:

```typescript
import { ini } from '@zoryacorporation/pulsar';

const config = ini.INIDocument.load('config.ini');

const host = config.get('server.host');
const port = config.getInt('server.port');
const debug = config.getBool('app.debug');

config.free();
```

[→ INI Guide](./ini.md)

---

## Real-World Example: File Processing Pipeline

Let's build a practical file processing pipeline using multiple Pulsar modules:

```typescript
import { weave, compress, fileops, hash } from '@zoryacorporation/pulsar';

interface ProcessedFile {
  path: string;
  originalSize: number;
  compressedSize: number;
  checksum: string;
}

class FileProcessor {
  private processed: ProcessedFile[] = [];
  private log = weave.Cord.create();
  
  async processDirectory(dir: string, outputDir: string): Promise<void> {
    this.logLine(`Processing directory: ${dir}`);
    
    // Ensure output directory exists
    fileops.mkdirp(outputDir);
    
    // Find all files
    const files = this.findFiles(dir);
    this.logLine(`Found ${files.length} files`);
    
    for (const file of files) {
      this.processFile(file, outputDir);
    }
    
    // Write summary
    this.writeSummary(outputDir);
  }
  
  private findFiles(dir: string): string[] {
    const results: string[] = [];
    
    const entries = fileops.readdirWithTypes(dir);
    for (const entry of entries) {
      const fullPath = fileops.joinPath(dir, entry.name);
      
      if (entry.isDirectory) {
        results.push(...this.findFiles(fullPath));
      } else if (entry.isFile) {
        results.push(fullPath);
      }
    }
    
    return results;
  }
  
  private processFile(inputPath: string, outputDir: string): void {
    const filename = fileops.basename(inputPath);
    const outputPath = fileops.joinPath(outputDir, `${filename}.zst`);
    
    // Read file
    const data = fileops.readFile(inputPath);
    
    // Calculate checksum
    const checksum = hash.nxh64(data).toString(16);
    
    // Compress
    const compressed = compress.zstd.compress(data, 9);
    
    // Write compressed file
    fileops.writeFile(outputPath, compressed);
    
    // Record stats
    const result: ProcessedFile = {
      path: inputPath,
      originalSize: data.length,
      compressedSize: compressed.length,
      checksum
    };
    this.processed.push(result);
    
    const ratio = ((1 - compressed.length / data.length) * 100).toFixed(1);
    this.logLine(`  ${filename}: ${data.length} → ${compressed.length} (${ratio}%)`);
  }
  
  private writeSummary(outputDir: string): void {
    const builder = weave.Weave.create();
    
    builder.append('# Processing Summary\n\n');
    builder.append(`Total files: ${this.processed.length}\n`);
    
    let totalOriginal = 0;
    let totalCompressed = 0;
    
    builder.append('\n## Files\n\n');
    builder.append('| File | Original | Compressed | Ratio | Checksum |\n');
    builder.append('|------|----------|------------|-------|----------|\n');
    
    for (const file of this.processed) {
      totalOriginal += file.originalSize;
      totalCompressed += file.compressedSize;
      
      const ratio = ((1 - file.compressedSize / file.originalSize) * 100).toFixed(1);
      const name = fileops.basename(file.path);
      builder.append(`| ${name} | ${file.originalSize} | ${file.compressedSize} | ${ratio}% | ${file.checksum.slice(0, 8)} |\n`);
    }
    
    const totalRatio = ((1 - totalCompressed / totalOriginal) * 100).toFixed(1);
    builder.append(`\n**Total: ${totalOriginal} → ${totalCompressed} bytes (${totalRatio}% reduction)**\n`);
    
    fileops.writeFile(
      fileops.joinPath(outputDir, 'summary.md'),
      builder.toString()
    );
    
    this.logLine('\nSummary written to summary.md');
  }
  
  private logLine(message: string): void {
    console.log(message);
    this.log.append(message + '\n');
  }
  
  getLog(): string {
    return this.log.toString();
  }
}

// Usage
const processor = new FileProcessor();
processor.processDirectory('./input', './output');
```

---

## Performance Tips

### 1. Use the Right Tool

```typescript
// NOT SUGGESTED:JavaScript string concatenation
let result = '';
for (let i = 0; i < 10000; i++) {
  result += `line ${i}\n`;
}

// SUGGESTED: Use Weave for string building
const builder = weave.Weave.create();
builder.reserve(200000);
for (let i = 0; i < 10000; i++) {
  builder.append(`line ${i}\n`);
}
const result = builder.toString();
```

### 2. Batch Operations

```typescript
// NOT SUGGESTED: Reading files one at a time in loops
for (const path of paths) {
  const stat = fileops.stat(path);
  const data = fileops.readFile(path);
}

// SUGGESTED: Reduce syscalls when possible
const entries = fileops.readdirWithTypes(dir); // Type info included
```

### 3. Pre-allocate Capacity

```typescript
// NOT SUGGESTED: Multiple reallocations
const builder = weave.Weave.create();
// Many appends cause reallocations

// SUGGESTED: Reserve capacity upfront
const builder = weave.Weave.create();
builder.reserve(estimatedSize);
```

### 4. Use Streaming for Large Data

```typescript
// NOT SUGGESTED: Loading entire large file
const huge = fileops.readFile('/path/to/10gb-file');

// SUGGESTED: Process in chunks (implement chunked reading)
// Use memory mapping or chunked processing
```

### 5. Clean Up Native Resources

```typescript
// NOT SUGGESTED: Memory leak
function loadConfig() {
  const config = ini.INIDocument.load('config.ini');
  return config.get('key');
  // config never freed!
}

// SUGGESTED: Proper cleanup
function loadConfig() {
  const config = ini.INIDocument.load('config.ini');
  try {
    return config.get('key');
  } finally {
    config.free();
  }
}
```

---

## Common Patterns

### Configuration Loading

```typescript
import { ini, fileops } from '@zoryacorporation/pulsar';

function loadAppConfig(env: string = 'development') {
  const basePath = './config';
  
  // Load base config
  const config = ini.INIDocument.load(`${basePath}/app.ini`);
  
  // Override with environment config if exists
  const envPath = `${basePath}/app.${env}.ini`;
  if (fileops.exists(envPath)) {
    const envConfig = ini.INIDocument.load(envPath);
    // Merge logic here
    envConfig.free();
  }
  
  return config;
}
```

### Caching Layer

```typescript
import { dagger, hash, compress } from '@zoryacorporation/pulsar';

class CompressedCache {
  private cache = new dagger.DaggerTable();
  
  set(key: string, value: any): void {
    const json = JSON.stringify(value);
    const compressed = compress.zstd.compress(Buffer.from(json), 1);
    this.cache.set(key, compressed.toString('base64'));
  }
  
  get(key: string): any {
    const stored = this.cache.get(key);
    if (!stored) return undefined;
    
    const compressed = Buffer.from(stored, 'base64');
    const json = compress.zstd.decompress(compressed).toString();
    return JSON.parse(json);
  }
  
  checksum(key: string): string | null {
    const stored = this.cache.get(key);
    if (!stored) return null;
    return hash.nxhString(stored).toString(16);
  }
}
```

### File Watcher

```typescript
import { fileops } from '@zoryacorporation/pulsar';

class DevServer {
  private watcher: ReturnType<typeof fileops.watch>;
  
  constructor(srcDir: string) {
    this.watcher = fileops.watch(srcDir);
  }
  
  start(callback: (file: string) => void): void {
    const poll = () => {
      const events = this.watcher.poll(100);
      
      for (const event of events) {
        if (event.isModify && !event.isDir) {
          callback(event.path);
        }
      }
      
      setTimeout(poll, 0);
    };
    
    poll();
  }
  
  stop(): void {
    this.watcher.close();
  }
}
```

---

## TypeScript Support

Pulsar includes full TypeScript definitions. Import and get full IntelliSense:

```typescript
import { 
  weave,    // String primitives
  compress, // Compression
  fileops,  // File operations
  hash,     // Hashing
  dagger,   // Hash tables
  pcm,      // Bit manipulation
  ini       // Config parsing
} from '@zoryacorporation/pulsar';

// Full type safety and autocompletion
const builder: weave.Weave = weave.Weave.from('hello');
const config: ini.INIDocument = ini.INIDocument.load('config.ini');
```

---

## Debugging

### Check Module Versions

```typescript
import { versions } from '@zoryacorporation/pulsar';

console.log(versions());
// { weave: '1.0.0', compress: '1.0.0', ... }
```

### Memory Usage

```typescript
// Arena stats
const arena = weave.Arena.create(4096);
// ... use arena ...
console.log(arena.stats());
// { totalAllocated, totalCapacity, blockCount }

// Tablet stats
const tablet = weave.Tablet.create();
// ... intern strings ...
console.log(tablet.memory); // bytes used

// INI stats
const config = ini.INIDocument.load('config.ini');
console.log(config.stats());
// { sections, keys, memoryUsed }
```

---

## Next Steps

Now that you're familiar with Pulsar, dive deeper into specific modules:

1. **[Weave Guide](./weave.md)** - Master string manipulation
2. **[Compress Guide](./compress.md)** - Efficient compression
3. **[FileOps Guide](./fileops.md)** - File system operations
4. **[Hash Guide](./hash.md)** - Fast hashing
5. **[Dagger Guide](./dagger.md)** - High-performance hash tables
6. **[PCM Guide](./pcm.md)** - Bit manipulation
7. **[INI Guide](./ini.md)** - Configuration parsing

---

## Getting Help

- **GitHub Issues**: Report bugs or request features
- **Discussions**: Ask questions and share projects
- **Documentation**: Full API reference in each module guide

## Leverage GitHub Copilot:

If you're using GitHub Copilot, you can leverage it to help you write code that uses the Pulsar modules. Each guide contains code snippets that demonstrate how to use the various functions and classes provided by Pulsar. These guides can serve as a reference for Copilot to suggest relevant code completions and examples as you write your own code with Pulsar. We will also be working on creating dedicated Copilot guides and instrunctions that can help you get the most out of using Copilot with Pulsar in your projects. Each guide is designed to be clear and descriptive, making it easy for both humans and AI assistants to understand how to use the Pulsar library effectively.
