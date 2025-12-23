# @zoryacorporation/pulsar

Author: Anthony Taliento -- Zorya Corporation   

High-performance native acceleration for Node.js and Electron applications.

Version: 1.0.0 | License: Apache-2.0 | Node: >=18.0.0 | 

## Platforms: 

Linux, macOS, Any Windows users are welcome to test, We just don't have CI for it yet however to guarantee compatibility utilizing WSL2 is recommended. Windows support is planned for a future release when the time comes. Another platform we will be supporting is WebAssembly (WASM) but that is also planned to be a seperate branch of Pulsar as this will require a different build system, and API's will need to be engineered to work within the constraints of WASM. The increase in portability WASM offers justifies the effort. We will announce when work begins on these platforms.

## Overview

Pulsar provides native-speed implementations for operations that are ordinarly slow or even downright impossible in pure JavaScript. Built on the ZORYA C SDK, it delivers 10-100x performance improvements for string manipulation, compression, file operations, and hashing. These modules are exposed via a clean TypeScript interface, making it easy to integrate high-performance native code into your Node.js applications. With Pulsar, you can handle large datasets, perform intensive computations, and manage files efficiently without sacrificing the developer experience. We achieve this by utilizng Node.js N-API for seamless native module integration, ensuring compatibility across Node.js versions and platforms. Whether you're building data-intensive applications, working with large files, or need fast hashing algorithms, Pulsar provides the tools you need to boost performance while keeping your codebase clean and maintainable.

This also allows developers to stay within the node.js ecosystem and use familiar tools while benefiting from the performance of native code. Modern codebases often require high performance for tasks like data processing, file handling, and compression. Electron applications tend to rely on Rust or C++ multi language projects for performance-critical components. Pulsar aims to simplify this by providing a single, easy-to-use package that covers a wide range of performance needs without the complexity of managing multiple languages or build systems.

**We will be adding comprehensive documentation and examples for each module including on guides, tutorials, getting started, and best practices utilizing Pulsar.**

Feel free to reach out with any questions or feedback directly here on GitHub! In addition to GitHub, we are in the process of setting up community channels and a formal web platform for Pulsar and other Zorya projects.   

Below is a brief overview of each module included in Pulsar along with example usage, snippets and a quick start guide.

## Installation

```bash
npm install @zoryacorporation/pulsar
```

Requirements:
- Node.js 18.0.0 or later
- Linux or macOS (x64 or arm64)
- Build tools for native compilation (automatically handled)

## Quick Start

```typescript
import { weave, compress, fileops, hash, ini } from '@zoryacorporation/pulsar';

// Hash a string
const h = hash.nxh64('hello world');

// Compress data
const data = Buffer.from('Hello, Pulsar!'.repeat(1000));
const compressed = compress.zstdCompress(data, 3);

// Read a file
const content = fileops.readFile('/etc/hostname');
```

## Modules

Pulsar includes seven native modules:

### weave

String primitives with arena allocation and string interning.

```typescript
import { weave } from '@zoryacorporation/pulsar';

// Immutable string with O(1) operations
const str = weave.weaveCreate('hello world');
const len = weave.weaveLength(str);
const sub = weave.weaveSlice(str, 0, 5);

// String interning (deduplication)
const tablet = weave.tabletCreate();
const id1 = weave.tabletIntern(tablet, 'hello');
const id2 = weave.tabletIntern(tablet, 'hello');
// id1 === id2 (same string, same ID)

// Arena allocator for bulk string operations
const arena = weave.arenaCreate(4096);
const ptr = weave.arenaAllocString(arena, 'data');
```

### compress

Zstandard and LZ4 compression.

```typescript
import { compress } from '@zoryacorporation/pulsar';

// Zstandard (best ratio)
const zstd = compress.zstdCompress(buffer, 3);  // level 1-22
const original = compress.zstdDecompress(zstd);

// LZ4 (fastest)
const lz4 = compress.lz4Compress(buffer);
const restored = compress.lz4Decompress(lz4, originalSize);

// Streaming compression
const stream = compress.zstdStreamCreate(3);
compress.zstdStreamWrite(stream, chunk1);
compress.zstdStreamWrite(stream, chunk2);
const result = compress.zstdStreamEnd(stream);
```

### fileops

Native file operations.

```typescript
import { fileops } from '@zoryacorporation/pulsar';

// Read/write files
const data = fileops.readFile('/path/to/file');
fileops.writeFile('/path/to/file', buffer);
fileops.appendFile('/path/to/file', buffer);

// File info
const stat = fileops.stat('/path/to/file');
const exists = fileops.exists('/path/to/file');

// Directory operations
const files = fileops.readDir('/path/to/dir');
fileops.mkdir('/path/to/dir');
fileops.remove('/path/to/file');

// Memory-mapped files
const mapped = fileops.mmap('/large/file');
```

### hash

NXH64 non-cryptographic hash function.

```typescript
import { hash } from '@zoryacorporation/pulsar';

// Hash string or buffer
const h = hash.nxh64('hello world');
const seeded = hash.nxh64Seeded('hello', 42n);

// Incremental hashing
const state = hash.nxh64Init();
hash.nxh64Update(state, 'part1');
hash.nxh64Update(state, 'part2');
const final = hash.nxh64Final(state);
```

### dagger

Robin Hood hash table with cuckoo hashing fallback.

```typescript
import { dagger } from '@zoryacorporation/pulsar';

const table = dagger.create();

// Key-value operations
dagger.set(table, 'key', { data: 'value' });
const val = dagger.get(table, 'key');
const has = dagger.has(table, 'key');
dagger.delete(table, 'key');

// Iteration
dagger.forEach(table, (key, value) => {
    console.log(key, value);
});

// Stats
const stats = dagger.stats(table);
console.log(stats.count, stats.loadFactor);
```

### pcm

Bit manipulation and prime cascade multiplication.

```typescript
import { pcm } from '@zoryacorporation/pulsar';

// Bit operations
const pop = pcm.popcount(0xFF);      // Count set bits
const clz = pcm.clz(0x0F);           // Count leading zeros
const ctz = pcm.ctz(0x10);           // Count trailing zeros
const rev = pcm.bitReverse32(n);     // Reverse bit order

// PCM hash
const h = pcm.pcmHash('data');
```

### ini

INI file parser with dot notation access.

```typescript
import { ini } from '@zoryacorporation/pulsar';

const config = ini.create();

// Load from file or string
ini.load(config, '/path/to/config.ini');
ini.loadString(config, '[section]\nkey=value');

// Access values (dot notation: section.key)
const host = ini.get(config, 'database.host');
const port = ini.getInt(config, 'database.port');
const debug = ini.getBool(config, 'app.debug');

// Arrays use pipe delimiter
// items = a|b|c
const items = ini.getArray(config, 'section.items');

// Check existence
if (ini.has(config, 'section.key')) { ... }

// Set values
ini.set(config, 'section.key', 'value');

// List sections
const sections = ini.sections(config);

ini.free(config);
```

## API Reference

All functions are available as direct exports:

```typescript
import {
    weave,    // String primitives
    compress, // Zstd/LZ4 compression
    fileops,  // File operations
    hash,     // NXH64 hashing
    dagger,   // Hash table
    pcm,      // Bit operations
    ini,      // INI parser
    versions  // Get all module versions
} from '@zoryacorporation/pulsar';

// Check versions
console.log(versions());
```

## Performance

Benchmarks vs pure JavaScript equivalents (Node.js 22, Linux x64):

| Operation | JS | Pulsar | Speedup |
|-----------|---:|-------:|--------:|
| String hash (1KB) | 45us | 0.8us | 56x |
| Zstd compress (1MB) | N/A | 12ms | - |
| LZ4 compress (1MB) | N/A | 3ms | - |
| File read (1MB) | 2.1ms | 0.4ms | 5x |
| Hash table insert (100K) | 89ms | 12ms | 7x |

## Building from Source

```bash
git clone https://github.com/ZoryaCorporation/pulsar.git
cd pulsar
npm install
npm run build
npm test
```

## Requirements

Runtime:
- Node.js >= 18.0.0
- Linux (glibc 2.17+) or macOS 11+
- x64 or arm64 architecture

Build:
- C compiler (gcc, clang)
- Python 3 (for node-gyp)
- make

## License

Apache-2.0

Copyright 2025 Zorya Corporation
