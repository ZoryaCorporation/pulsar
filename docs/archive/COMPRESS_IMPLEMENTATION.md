# @aspect/compress Implementation Plan

**Priority:** 🔥🔥🔥🔥🔥 CRITICAL  
**Why:** Compression in TSX is genuinely terrible. This is low-hanging fruit with massive impact.

---

## The Problem

Current TSX compression landscape:

```typescript
// Option 1: Node's built-in zlib - SLOW
import zlib from 'zlib';
const compressed = zlib.gzipSync(data); // 10 MB/s

// Option 2: Random npm packages - inconsistent APIs
import pako from 'pako';        // gzip only
import lz4 from 'lz4';          // abandoned
import zstd from '@napi-rs/zstd'; // good but limited

// Option 3: Shell out - hacky
import { execSync } from 'child_process';
execSync(`zstd -c input.txt > output.zst`);
```

**The pain:**
- No unified API
- Performance varies wildly
- Streaming support is inconsistent
- Most packages are abandoned or poorly maintained

---

## Our Solution

```typescript
import { 
  compress, 
  decompress, 
  createCompressStream,
  Algorithm 
} from '@aspect/compress';

// Simple one-shot
const packed = compress(data, Algorithm.ZSTD);
const unpacked = decompress(packed);

// With options
const packed = compress(data, Algorithm.ZSTD, { 
  level: 19,           // Max compression
  checksumEnabled: true 
});

// Streaming for large files
fs.createReadStream('huge.log')
  .pipe(createCompressStream(Algorithm.LZ4))
  .pipe(fs.createWriteStream('huge.log.lz4'));

// Auto-detect decompression
const data = decompress(mysteryBlob); // Figures out format automatically
```

---

## Implementation Strategy

### Phase 1: zstd-napi (Week 1)

**Why zstd first:**
- Best general-purpose algorithm
- 500 MB/s compression, 1.5 GB/s decompression
- Facebook uses it (proven at scale)
- Critical for ZoryaWrite document storage

**Approach:** Follow the weave-napi pattern exactly.

```
compress-napi/
├── src/
│   ├── napi/
│   │   └── compress_napi.c    # NAPI bindings
│   └── ts/
│       └── index.ts           # TypeScript wrapper
├── include/
│   ├── zstd.h                 # From zstd source
│   └── lz4.h                  # From lz4 source
├── lib/
│   ├── libzstd.a              # Pre-compiled static libs
│   └── liblz4.a
├── binding.gyp
├── package.json
└── test/
    └── compress.test.cjs
```

**C Implementation:**

```c
// src/napi/compress_napi.c

#include <node_api.h>
#include <zstd.h>
#include <stdlib.h>

// Compress buffer
static napi_value zstd_compress(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    // Get input buffer
    void* input_data;
    size_t input_len;
    napi_get_buffer_info(env, argv[0], &input_data, &input_len);
    
    // Get compression level (default 3)
    int32_t level = 3;
    if (argc > 1) {
        napi_get_value_int32(env, argv[1], &level);
    }
    
    // Calculate max output size
    size_t max_dst_size = ZSTD_compressBound(input_len);
    
    // Allocate output buffer
    void* output_data = malloc(max_dst_size);
    
    // Compress
    size_t compressed_size = ZSTD_compress(
        output_data, max_dst_size,
        input_data, input_len,
        level
    );
    
    if (ZSTD_isError(compressed_size)) {
        free(output_data);
        napi_throw_error(env, NULL, ZSTD_getErrorName(compressed_size));
        return NULL;
    }
    
    // Create result buffer (copy to Node.js managed memory)
    napi_value result;
    void* result_data;
    napi_create_buffer_copy(env, compressed_size, output_data, &result_data, &result);
    
    free(output_data);
    return result;
}

// Decompress buffer
static napi_value zstd_decompress(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    void* input_data;
    size_t input_len;
    napi_get_buffer_info(env, argv[0], &input_data, &input_len);
    
    // Get decompressed size from frame header
    unsigned long long decompressed_size = ZSTD_getFrameContentSize(input_data, input_len);
    
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
        napi_throw_error(env, NULL, "Not valid zstd compressed data");
        return NULL;
    }
    
    void* output_data = malloc(decompressed_size);
    
    size_t actual_size = ZSTD_decompress(
        output_data, decompressed_size,
        input_data, input_len
    );
    
    if (ZSTD_isError(actual_size)) {
        free(output_data);
        napi_throw_error(env, NULL, ZSTD_getErrorName(actual_size));
        return NULL;
    }
    
    napi_value result;
    void* result_data;
    napi_create_buffer_copy(env, actual_size, output_data, &result_data, &result);
    
    free(output_data);
    return result;
}
```

### Phase 2: Add LZ4 (Week 2)

**Why LZ4:**
- 2 GB/s compression (real-time capable)
- Perfect for logging, game assets, temporary files
- Lower ratio but speed is worth it

```c
#include <lz4.h>

static napi_value lz4_compress(napi_env env, napi_callback_info info) {
    // Similar pattern to zstd
    int max_dst_size = LZ4_compressBound(input_len);
    void* output = malloc(max_dst_size);
    
    int compressed_size = LZ4_compress_default(
        input_data, output,
        input_len, max_dst_size
    );
    
    // Return buffer...
}
```

### Phase 3: Streaming Support (Week 3)

For files larger than memory:

```typescript
// TypeScript streaming API
export function createCompressStream(
  algorithm: Algorithm,
  options?: CompressOptions
): Transform {
  return new ZstdCompressTransform(options);
}

// Internal transform stream
class ZstdCompressTransform extends Transform {
  private ctx: ZSTD_CCtx;
  
  constructor(options: CompressOptions) {
    super();
    this.ctx = native.zstdCreateCCtx();
    native.zstdCCtxSetLevel(this.ctx, options.level ?? 3);
  }
  
  _transform(chunk: Buffer, encoding: string, callback: Function) {
    const compressed = native.zstdCompressStream(this.ctx, chunk);
    this.push(compressed);
    callback();
  }
  
  _flush(callback: Function) {
    const final = native.zstdEndStream(this.ctx);
    this.push(final);
    native.zstdFreeCCtx(this.ctx);
    callback();
  }
}
```

### Phase 4: Unified TypeScript API

```typescript
// src/ts/index.ts

export enum Algorithm {
  ZSTD = 'zstd',
  LZ4 = 'lz4',
  GZIP = 'gzip',
  BROTLI = 'brotli'
}

export interface CompressOptions {
  level?: number;        // 1-22 for zstd, 1-9 for gzip
  checksumEnabled?: boolean;
  windowLog?: number;    // Advanced tuning
}

/**
 * Compress data using specified algorithm
 */
export function compress(
  data: Buffer | Uint8Array | string,
  algorithm: Algorithm = Algorithm.ZSTD,
  options: CompressOptions = {}
): Buffer {
  const input = toBuffer(data);
  
  switch (algorithm) {
    case Algorithm.ZSTD:
      return native.zstdCompress(input, options.level ?? 3);
    case Algorithm.LZ4:
      return native.lz4Compress(input);
    case Algorithm.GZIP:
      return native.gzipCompress(input, options.level ?? 6);
    case Algorithm.BROTLI:
      return native.brotliCompress(input, options.level ?? 4);
    default:
      throw new Error(`Unknown algorithm: ${algorithm}`);
  }
}

/**
 * Decompress data (auto-detects format)
 */
export function decompress(data: Buffer): Buffer {
  const magic = data.slice(0, 4);
  
  // Magic number detection
  if (magic[0] === 0x28 && magic[1] === 0xB5 && magic[2] === 0x2F && magic[3] === 0xFD) {
    return native.zstdDecompress(data);
  }
  if (magic[0] === 0x04 && magic[1] === 0x22 && magic[2] === 0x4D && magic[3] === 0x18) {
    return native.lz4Decompress(data);
  }
  if (magic[0] === 0x1F && magic[1] === 0x8B) {
    return native.gzipDecompress(data);
  }
  // Add more formats...
  
  throw new Error('Unknown compression format');
}
```

---

## Benchmarks to Show

Create a compelling benchmark that proves the value:

```typescript
// benchmark/compress-bench.ts
import { compress as pulsarCompress, Algorithm } from '@aspect/compress';
import zlib from 'zlib';
import { readFileSync } from 'fs';

const testData = readFileSync('./testdata/large.json'); // 100MB file

console.log('Compression Benchmark');
console.log('='.repeat(50));

// Node.js zlib
console.time('Node zlib (gzip)');
zlib.gzipSync(testData);
console.timeEnd('Node zlib (gzip)');

// Pulsar zstd
console.time('Pulsar zstd');
pulsarCompress(testData, Algorithm.ZSTD);
console.timeEnd('Pulsar zstd');

// Pulsar lz4
console.time('Pulsar lz4');
pulsarCompress(testData, Algorithm.LZ4);
console.timeEnd('Pulsar lz4');
```

**Expected output:**
```
Compression Benchmark
==================================================
Node zlib (gzip): 10240ms
Pulsar zstd: 205ms       (50x faster!)
Pulsar lz4: 52ms         (200x faster!)
```

---

## Build Dependencies

### Getting zstd source

```bash
# Clone official repo
git clone https://github.com/facebook/zstd.git
cd zstd

# Build static library
make lib-release
# Output: lib/libzstd.a

# Copy what we need
cp lib/libzstd.a ../compress-napi/lib/
cp lib/zstd.h ../compress-napi/include/
```

### Getting lz4 source

```bash
git clone https://github.com/lz4/lz4.git
cd lz4

make lib
# Output: lib/liblz4.a

cp lib/liblz4.a ../compress-napi/lib/
cp lib/lz4.h ../compress-napi/include/
```

### binding.gyp

```python
{
  "targets": [{
    "target_name": "compress_napi",
    "sources": ["src/napi/compress_napi.c"],
    "include_dirs": [
      "include",
      "<!@(node -p \"require('node-addon-api').include\")"
    ],
    "libraries": [
      "<(module_root_dir)/lib/libzstd.a",
      "<(module_root_dir)/lib/liblz4.a"
    ],
    "cflags": ["-O3", "-fPIC"],
    "defines": ["NAPI_VERSION=8"]
  }]
}
```

---

## Test Plan

```javascript
// test/compress.test.cjs
const assert = require('assert');
const { compress, decompress, Algorithm } = require('../dist/index.js');

suite('zstd', () => {
  test('compresses and decompresses', () => {
    const input = Buffer.from('hello world'.repeat(1000));
    const compressed = compress(input, Algorithm.ZSTD);
    const decompressed = decompress(compressed);
    assert.deepStrictEqual(decompressed, input);
  });
  
  test('compression actually reduces size', () => {
    const input = Buffer.from('aaaa'.repeat(10000));
    const compressed = compress(input, Algorithm.ZSTD);
    assert(compressed.length < input.length / 10);
  });
  
  test('handles empty input', () => {
    const input = Buffer.alloc(0);
    const compressed = compress(input, Algorithm.ZSTD);
    const decompressed = decompress(compressed);
    assert.deepStrictEqual(decompressed, input);
  });
});

suite('lz4', () => {
  test('faster than zstd for same data', () => {
    const input = Buffer.from('test data'.repeat(100000));
    
    const start1 = performance.now();
    compress(input, Algorithm.LZ4);
    const lz4Time = performance.now() - start1;
    
    const start2 = performance.now();
    compress(input, Algorithm.ZSTD);
    const zstdTime = performance.now() - start2;
    
    assert(lz4Time < zstdTime, 'LZ4 should be faster');
  });
});

suite('auto-detect', () => {
  test('detects zstd format', () => {
    const input = Buffer.from('test');
    const compressed = compress(input, Algorithm.ZSTD);
    const decompressed = decompress(compressed); // No algorithm specified
    assert.deepStrictEqual(decompressed, input);
  });
});
```

---

## Timeline

| Day | Task |
|-----|------|
| 1 | Scaffold project structure |
| 2 | Build zstd static library |
| 3-4 | NAPI bindings for zstd compress/decompress |
| 5 | TypeScript wrapper + tests |
| 6-7 | Add LZ4 |
| 8 | Streaming support |
| 9-10 | Benchmarks + documentation |

**Total:** ~2 weeks for production-ready @aspect/compress

---

## Success Metrics

- [ ] zstd: 500 MB/s compression verified
- [ ] lz4: 2 GB/s compression verified  
- [ ] All tests passing on Linux/macOS/Windows
- [ ] Streaming works for 1GB+ files
- [ ] Auto-detect decompression works
- [ ] Clean npm install (no external deps)
