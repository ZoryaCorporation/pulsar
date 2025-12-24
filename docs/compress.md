# Compress Module Guide

> **Native Compression at Blazing Speed**  
> Zstandard and LZ4 compression with streaming support

The Compress module provides native bindings to two industry-leading compression algorithms: **Zstandard (zstd)** for best compression ratios and **LZ4** for maximum speed. Both are battle-tested in production at Facebook, Apple, and countless other companies. We have bound these algorithms to Node.js with a easy to use, high-performance API. This brings you the best of both worlds: native speed and seamless JavaScript integration. The Compress module is ideal for applications needing fast, efficient data storage and transfer and developers looking multiple compression options in a single package.

Also another note worthy mention is that compression is not just about saving space. It can also significantly reduce network latency and improve application responsiveness by minimizing the amount of data that needs to be transmitted or processed. This is especially important in real-time applications, mobile apps, and services with high data throughput. Additionally, compressed data is **not cryptographically secure,** so it should not be used as a substitute for encryption when data confidentiality is required. However, compressed data can be encrypted after compression to ensure both efficiency and security requirements are met. We do not provide encryption features in this module, but it can be easily combined with other libraries for secure data handling. As a rule of thumb compressed data is generally safer and less prone to certain types of attacks compared to uncompressed data, but it is **not a replacement for proper security measures.**

---

## Overview

| Algorithm | Ratio | Speed | Best For |
|-----------|-------|-------|----------|
| **Zstandard** | Excellent | Fast | General purpose, archives, network |
| **LZ4** | Good | Extreme | Real-time, gaming, IPC |
| **LZ4 HC** | Very Good | Fast | Better ratio than LZ4, still fast |

---

## Quick Start

```typescript
import { compress } from '@zoryacorporation/pulsar';

// Compress with Zstandard (default)
const data = Buffer.from('Hello, Pulsar!'.repeat(1000));
const compressed = compress.zstd.compress(data, 3);
const original = compress.zstd.decompress(compressed);

// Or use the unified API
const result = compress.compress(data, { algorithm: 'zstd', level: 3 });
```

---

## Zstandard (zstd)

Zstandard offers an excellent balance of compression ratio and speed. It's the default choice for most applications.

### Basic Compression

```typescript
import { compress } from '@zoryacorporation/pulsar';

const data = Buffer.from('Your data here');

// Compress with default level (3)
const compressed = compress.zstd.compress(data);

// Decompress
const original = compress.zstd.decompress(compressed);

console.log(original.toString()); // 'Your data here'
```

### Compression Levels

Zstandard supports levels 1-22. Higher levels = better ratio but slower compression.
the difference between levels is significant:
1 is fastest, 3 is default, 9 is good, 19 is high, 22 is max. The compression speed decreases as the level increases, while the compression ratio improves. The compression ratio is the proportion of the original data size to the compressed data size; a lower ratio indicates better compression. However not all data benefits equally from higher levels. High compressions levels are best for archival data where time is less critical, while low levels suit real-time use cases. Also note that decompression, and compression while reliable, can cause data issues with a great example being photo resolution where detail is lost at higher levels of compression.

Below is a guide that walks you through utilizing the compress module and its features effectively.

```typescript
import { compress } from '@zoryacorporation/pulsar';

const data = Buffer.from('x'.repeat(100000));

// Level 1: Fastest compression
const fast = compress.zstd.compress(data, 1);

// Level 3: Default, balanced (recommended)
const balanced = compress.zstd.compress(data, 3);

// Level 9: Good compression
const good = compress.zstd.compress(data, 9);

// Level 19: High compression
const high = compress.zstd.compress(data, 19);

// Level 22: Maximum compression (slowest)
const max = compress.zstd.compress(data, 22);

console.log({
  original: data.length,
  level1: fast.length,
  level3: balanced.length,
  level9: good.length,
  level19: high.length,
  level22: max.length
});
```

### Level Selection Guide

| Level | Use Case |
|-------|----------|
| 1-3 | Real-time compression, network transfer |
| 3-6 | General purpose (default: 3) |
| 6-12 | Better ratio when CPU is available |
| 12-19 | Archival, one-time compression |
| 19-22 | Maximum compression, time not critical |

### Bound Estimation

Calculate maximum possible compressed size (for buffer allocation):

```typescript
const data = Buffer.from('...');
const maxSize = compress.zstd.compressBound(data.length);
console.log(`Maximum compressed size: ${maxSize} bytes`);
```

---

## LZ4

LZ4 is the fastest compression algorithm available. Choose it when speed is critical.

### Basic Compression

```typescript
import { compress } from '@zoryacorporation/pulsar';

const data = Buffer.from('Fast compression!'.repeat(1000));

// Compress
const compressed = compress.lz4.compress(data);

// Decompress (original size may be needed for some formats)
const original = compress.lz4.decompress(compressed);
```

### High Compression Mode (LZ4 HC)

LZ4 HC trades some speed for better compression ratios while remaining fast:

```typescript
import { compress } from '@zoryacorporation/pulsar';

const data = Buffer.from('x'.repeat(100000));

// Regular LZ4 (fastest)
const regular = compress.lz4.compress(data);

// LZ4 HC with default level (9)
const hc = compress.lz4.compressHC(data);

// LZ4 HC with custom level (1-12)
const hcLow = compress.lz4.compressHC(data, 4);
const hcMax = compress.lz4.compressHC(data, 12);

console.log({
  original: data.length,
  lz4: regular.length,
  lz4hc: hc.length,
  lz4hcMax: hcMax.length
});
```

### Bound Estimation

```typescript
const data = Buffer.from('...');
const maxSize = compress.lz4.compressBound(data.length);
```

---

## Unified API

For flexibility, use the unified compress/decompress functions:

```typescript
import { compress } from '@zoryacorporation/pulsar';

const data = Buffer.from('Hello, unified API!'.repeat(1000));

// Zstandard
const zstdResult = compress.compress(data, { algorithm: 'zstd', level: 5 });

// LZ4
const lz4Result = compress.compress(data, { algorithm: 'lz4' });

// LZ4 HC
const lz4hcResult = compress.compress(data, { algorithm: 'lz4hc', level: 9 });

// Decompress
const original1 = compress.decompress(zstdResult, 'zstd');
const original2 = compress.decompress(lz4Result, 'lz4');
const original3 = compress.decompress(lz4hcResult, 'lz4'); // LZ4 HC uses LZ4 decompress
```

---

## Common Use Cases

### File Compression

```typescript
import { compress, fileops } from '@zoryacorporation/pulsar';

// Compress a file
function compressFile(inputPath: string, outputPath: string, level = 3): void {
  const data = fileops.readFile(inputPath);
  const compressed = compress.zstd.compress(data, level);
  fileops.writeFile(outputPath, compressed);
  
  const ratio = ((1 - compressed.length / data.length) * 100).toFixed(1);
  console.log(`Compressed: ${data.length} → ${compressed.length} bytes (${ratio}% reduction)`);
}

// Decompress a file
function decompressFile(inputPath: string, outputPath: string): void {
  const compressed = fileops.readFile(inputPath);
  const data = compress.zstd.decompress(compressed);
  fileops.writeFile(outputPath, data);
}

// Usage
compressFile('/path/to/large-file.json', '/path/to/large-file.json.zst', 9);
decompressFile('/path/to/large-file.json.zst', '/path/to/restored.json');
```

### Network Transfer

```typescript
import { compress } from '@zoryacorporation/pulsar';

// Compress API response
function compressResponse(data: object): Buffer {
  const json = JSON.stringify(data);
  const buffer = Buffer.from(json);
  
  // Use level 1-3 for real-time compression
  return compress.zstd.compress(buffer, 1);
}

// Decompress API request
function decompressRequest(compressed: Buffer): object {
  const buffer = compress.zstd.decompress(compressed);
  return JSON.parse(buffer.toString());
}

// Example
const payload = { users: Array(1000).fill({ name: 'John', age: 30 }) };
const compressed = compressResponse(payload);
const original = decompressRequest(compressed);

console.log(`Original: ${JSON.stringify(payload).length} bytes`);
console.log(`Compressed: ${compressed.length} bytes`);
```

### Cache Storage

```typescript
import { compress, hash } from '@zoryacorporation/pulsar';

class CompressedCache {
  private cache = new Map<string, Buffer>();
  
  set(key: string, value: any): void {
    const json = JSON.stringify(value);
    const buffer = Buffer.from(json);
    
    // Only compress if beneficial
    if (buffer.length > 100) {
      const compressed = compress.zstd.compress(buffer, 3);
      this.cache.set(key, compressed);
    } else {
      this.cache.set(key, buffer);
    }
  }
  
  get(key: string): any {
    const data = this.cache.get(key);
    if (!data) return undefined;
    
    try {
      // Try decompress first
      const decompressed = compress.zstd.decompress(data);
      return JSON.parse(decompressed.toString());
    } catch {
      // If decompress fails, it's uncompressed
      return JSON.parse(data.toString());
    }
  }
}
```

### Real-Time Game State

```typescript
import { compress } from '@zoryacorporation/pulsar';

// LZ4 for lowest latency
function packGameState(state: GameState): Buffer {
  const json = JSON.stringify(state);
  const buffer = Buffer.from(json);
  return compress.lz4.compress(buffer);
}

function unpackGameState(packed: Buffer): GameState {
  const buffer = compress.lz4.decompress(packed);
  return JSON.parse(buffer.toString());
}

// For slightly better compression with still-low latency
function packGameStateSave(state: GameState): Buffer {
  const json = JSON.stringify(state);
  const buffer = Buffer.from(json);
  return compress.lz4.compressHC(buffer, 6);
}
```

### Log Archival

```typescript
import { compress, fileops } from '@zoryacorporation/pulsar';

async function archiveLogs(logDir: string, archiveDir: string): Promise<void> {
  const files = fileops.glob(`${logDir}/*.log`);
  
  for (const file of files) {
    const data = fileops.readFile(file);
    const filename = fileops.basename(file);
    
    // Maximum compression for archival
    const compressed = compress.zstd.compress(data, 19);
    
    fileops.writeFile(`${archiveDir}/${filename}.zst`, compressed);
    
    const ratio = ((1 - compressed.length / data.length) * 100).toFixed(1);
    console.log(`${filename}: ${ratio}% reduction`);
    
    // Remove original after successful compression
    fileops.remove(file);
  }
}
```

---

## Choosing an Algorithm

### Use Zstandard When:

- General purpose compression needed
- Archiving files for storage
- Network transfer where ratio matters
- Compressing JSON, text, or structured data
- You need a good balance of speed and ratio

### Use LZ4 When:

- Speed is the top priority
- Real-time applications (games, streaming)
- Inter-process communication (IPC)
- Memory compression
- Latency-sensitive operations

### Use LZ4 HC When:

- Need better ratio than LZ4
- Compression happens once, decompression many times
- Still need fast decompression
- Building assets/resources

---

## Performance Comparison

Typical results on 1MB of JSON data (Node.js 22, x64 Linux):

| Algorithm | Level | Ratio | Compress | Decompress |
|-----------|-------|-------|----------|------------|
| Zstd | 1 | 78% | 180 MB/s | 900 MB/s |
| Zstd | 3 | 82% | 120 MB/s | 900 MB/s |
| Zstd | 9 | 86% | 45 MB/s | 900 MB/s |
| Zstd | 19 | 89% | 8 MB/s | 900 MB/s |
| LZ4 | - | 67% | 750 MB/s | 2800 MB/s |
| LZ4 HC | 9 | 75% | 80 MB/s | 2800 MB/s |

> Note: Actual performance varies based on data type and system.

---

## Best Practices

### 1. Choose the Right Level

```typescript
// NOT RECOMMENDED: Using max level for real-time data
const slow = compress.zstd.compress(realtimeData, 22);

// RECOMMENDED: Use appropriate level
const fast = compress.zstd.compress(realtimeData, 1);
const archival = compress.zstd.compress(backupData, 19);
```

### 2. Consider Data Size

```typescript
// NOT RECOMMENDED: Compressing tiny data
const tiny = Buffer.from('hi');
const worse = compress.zstd.compress(tiny); // Larger than original!

// RECOMMENDED: Only compress when beneficial
function smartCompress(data: Buffer): Buffer {
  if (data.length < 100) return data;
  
  const compressed = compress.zstd.compress(data, 3);
  return compressed.length < data.length ? compressed : data;
}
```

### 3. Batch Similar Data

```typescript
// NOT RECOMMENDED: Compressing each small record
for (const record of records) {
  compressed.push(compress.zstd.compress(Buffer.from(JSON.stringify(record))));
}

// RECOMMENDED: Batch records together
const batch = JSON.stringify(records);
const compressed = compress.zstd.compress(Buffer.from(batch), 3);
```

### 4. Reuse Compressed Data

```typescript
// NOT RECOMMENDED: Compressing same data multiple times
clients.forEach(client => {
  const compressed = compress.zstd.compress(broadcastData);
  client.send(compressed);
});

// RECOMMENDED: Compress once, send to all
const compressed = compress.zstd.compress(broadcastData, 1);
clients.forEach(client => client.send(compressed));
```

---

## Error Handling

```typescript
import { compress } from '@zoryacorporation/pulsar';

function safeDecompress(data: Buffer): Buffer | null {
  try {
    return compress.zstd.decompress(data);
  } catch (error) {
    console.error('Decompression failed:', error);
    return null;
  }
}

function validateCompressed(data: Buffer): boolean {
  try {
    compress.zstd.decompress(data);
    return true;
  } catch {
    return false;
  }
}
```

---

## API Reference

### Zstandard Namespace

| Function | Description |
|----------|-------------|
| `zstd.compress(data, level?)` | Compress buffer (level 1-22, default 3) |
| `zstd.decompress(data)` | Decompress buffer |
| `zstd.compressBound(size)` | Max compressed size estimate |

### LZ4 Namespace

| Function | Description |
|----------|-------------|
| `lz4.compress(data)` | Compress buffer |
| `lz4.compressHC(data, level?)` | High compression mode (level 1-12) |
| `lz4.decompress(data, origSize?)` | Decompress buffer |
| `lz4.compressBound(size)` | Max compressed size estimate |

### Unified API

| Function | Description |
|----------|-------------|
| `compress(data, options?)` | Compress with any algorithm |
| `decompress(data, algorithm?)` | Decompress (specify algorithm) |
| `version()` | Get module version |

### CompressOptions

```typescript
interface CompressOptions {
  algorithm?: 'zstd' | 'lz4' | 'lz4hc';
  level?: number;
}
```

---

## Next Steps

- [FileOps Module](./fileops.md) - Read/write files to compress
- [Hash Module](./hash.md) - Generate checksums for compressed data
- [Weave Module](./weave.md) - Build strings before compression
