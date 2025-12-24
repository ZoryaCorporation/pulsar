# Hash Module Guide

> **Lightning-Fast Non-Cryptographic Hashing**  
> NXH64 - High-quality 64-bit hashes at native speed

The Hash module provides NXH (Nexus Hash), a high-performance non-cryptographic hash function optimized for hash tables, checksums, and data integrity. With excellent avalanche properties and 56x faster than typical JavaScript implementations, it's perfect for performance-critical hashing needs.

---

## Overview

| Function | Output | Best For |
|----------|--------|----------|
| `nxh64` | 64-bit BigInt | Hash tables, checksums |
| `nxh32` | 32-bit number | Smaller hash tables |
| `nxhString` | 64-bit BigInt | Direct string hashing |
| `nxhCombine` | 64-bit BigInt | Composite keys |

> **Note**: NXH is NOT cryptographic. Do not use for passwords, signatures, or security-sensitive operations. Use `crypto` module for those.

---

## Quick Start

```typescript
import { hash } from '@zoryacorporation/pulsar';

// Hash a string
const h = hash.nxhString('hello world');
console.log(h); // 1234567890123456789n (BigInt)

// Hash a buffer
const buffer = Buffer.from('data');
const bufHash = hash.nxh64(buffer);

// Combine hashes for composite keys
const combined = hash.nxhCombine(hash1, hash2);
```

---

## Basic Hashing

### Hash Buffers

```typescript
import { hash } from '@zoryacorporation/pulsar';

// Hash a buffer - returns BigInt
const buffer = Buffer.from('Hello, World!');
const h = hash.nxh64(buffer);

console.log(h);           // 1234567890123456789n
console.log(typeof h);    // 'bigint'
```

### Hash Strings Directly

```typescript
import { hash } from '@zoryacorporation/pulsar';

// Hash strings without creating a buffer first
const h = hash.nxhString('hello world');

// Great for hash table keys
const userKey = hash.nxhString(`user:${userId}`);
```

### 32-bit Hashes

When you need smaller hashes (hash tables with fewer buckets):

```typescript
import { hash } from '@zoryacorporation/pulsar';

// 32-bit hash from buffer
const h32 = hash.nxh32(Buffer.from('data'));
console.log(typeof h32); // 'number'

// 32-bit hash from string
const strH32 = hash.nxhString32('data');
```

---

## Seeded Hashing

Seeds allow you to create different hash functions from the same algorithm:

```typescript
import { hash } from '@zoryacorporation/pulsar';

const data = Buffer.from('same data');

// Default seed
const h1 = hash.nxh64(data);

// Custom seed
const h2 = hash.nxh64(data, 42n);

// Different seeds produce different hashes
console.log(h1 === h2); // false

// Use provided constants
const withDefault = hash.nxh64(data, hash.SEED_DEFAULT);
const withAlt = hash.nxh64(data, hash.SEED_ALT);
```

### When to Use Seeds

- **Cuckoo hashing**: Use `SEED_DEFAULT` and `SEED_ALT` for two hash functions
- **Bloom filters**: Different seeds for each hash function
- **Randomization**: Prevent hash-flooding attacks
- **Partitioning**: Consistent hashing across nodes

---

## Hash Integers

Hash integer values directly without buffer conversion:

```typescript
import { hash } from '@zoryacorporation/pulsar';

// Hash 64-bit integer
const id = 12345678901234n;
const h64 = hash.nxhInt64(id);

// Hash 32-bit integer
const smallId = 42;
const h32from64 = hash.nxhInt32(smallId);
```

---

## Combining Hashes

For composite keys, combine multiple hashes:

```typescript
import { hash } from '@zoryacorporation/pulsar';

// Hash multiple values
const userId = hash.nxhString('user123');
const timestamp = hash.nxhInt64(Date.now());

// Combine into single hash
const compositeKey = hash.nxhCombine(userId, timestamp);

// Works for any number of values
function hashMultiple(...values: string[]): bigint {
  return values
    .map(v => hash.nxhString(v))
    .reduce((acc, h) => hash.nxhCombine(acc, h));
}

const combined = hashMultiple('a', 'b', 'c', 'd');
```

---

## Common Use Cases

### Hash Table Keys

```typescript
import { hash } from '@zoryacorporation/pulsar';

class FastHashMap<T> {
  private buckets: Array<Array<[string, T]>>;
  private size: number;
  
  constructor(capacity = 1024) {
    this.buckets = new Array(capacity).fill(null).map(() => []);
    this.size = 0;
  }
  
  private getBucket(key: string): number {
    const h = hash.nxhString(key);
    return Number(h % BigInt(this.buckets.length));
  }
  
  set(key: string, value: T): void {
    const idx = this.getBucket(key);
    const bucket = this.buckets[idx];
    
    for (const entry of bucket) {
      if (entry[0] === key) {
        entry[1] = value;
        return;
      }
    }
    
    bucket.push([key, value]);
    this.size++;
  }
  
  get(key: string): T | undefined {
    const idx = this.getBucket(key);
    const bucket = this.buckets[idx];
    
    for (const entry of bucket) {
      if (entry[0] === key) {
        return entry[1];
      }
    }
    
    return undefined;
  }
}
```

### File Checksums

```typescript
import { hash, fileops } from '@zoryacorporation/pulsar';

function fileChecksum(path: string): string {
  const data = fileops.readFile(path);
  const h = hash.nxh64(data);
  return h.toString(16); // Hex string
}

function hasFileChanged(path: string, knownChecksum: string): boolean {
  const current = fileChecksum(path);
  return current !== knownChecksum;
}

// Usage
const checksum = fileChecksum('/path/to/file');
console.log(`Checksum: ${checksum}`);
```

### Cuckoo Hashing

Use two hash functions for cuckoo hashing:

```typescript
import { hash } from '@zoryacorporation/pulsar';

class CuckooHashTable<T> {
  private table1: Array<[string, T] | null>;
  private table2: Array<[string, T] | null>;
  private capacity: number;
  
  constructor(capacity = 1024) {
    this.capacity = capacity;
    this.table1 = new Array(capacity).fill(null);
    this.table2 = new Array(capacity).fill(null);
  }
  
  private hash1(key: string): number {
    const h = hash.nxh64(Buffer.from(key), hash.SEED_DEFAULT);
    return Number(h % BigInt(this.capacity));
  }
  
  private hash2(key: string): number {
    const h = hash.nxh64(Buffer.from(key), hash.SEED_ALT);
    return Number(h % BigInt(this.capacity));
  }
  
  get(key: string): T | undefined {
    const idx1 = this.hash1(key);
    if (this.table1[idx1]?.[0] === key) {
      return this.table1[idx1]![1];
    }
    
    const idx2 = this.hash2(key);
    if (this.table2[idx2]?.[0] === key) {
      return this.table2[idx2]![1];
    }
    
    return undefined;
  }
  
  // ... insert with displacement logic
}
```

### Bloom Filter

```typescript
import { hash } from '@zoryacorporation/pulsar';

class BloomFilter {
  private bits: Uint8Array;
  private numHashes: number;
  private size: number;
  
  constructor(size = 1024, numHashes = 3) {
    this.size = size;
    this.numHashes = numHashes;
    this.bits = new Uint8Array(Math.ceil(size / 8));
  }
  
  private getHashes(value: string): number[] {
    const hashes: number[] = [];
    const buffer = Buffer.from(value);
    
    for (let i = 0; i < this.numHashes; i++) {
      const h = hash.nxh64(buffer, BigInt(i));
      hashes.push(Number(h % BigInt(this.size)));
    }
    
    return hashes;
  }
  
  add(value: string): void {
    for (const h of this.getHashes(value)) {
      const byteIdx = Math.floor(h / 8);
      const bitIdx = h % 8;
      this.bits[byteIdx] |= (1 << bitIdx);
    }
  }
  
  mightContain(value: string): boolean {
    for (const h of this.getHashes(value)) {
      const byteIdx = Math.floor(h / 8);
      const bitIdx = h % 8;
      if (!(this.bits[byteIdx] & (1 << bitIdx))) {
        return false;
      }
    }
    return true;
  }
}

// Usage
const filter = new BloomFilter(10000, 3);
filter.add('hello');
filter.add('world');

console.log(filter.mightContain('hello')); // true
console.log(filter.mightContain('world')); // true
console.log(filter.mightContain('foo'));   // false (probably)
```

### Content Deduplication

```typescript
import { hash, fileops } from '@zoryacorporation/pulsar';

class ContentDeduplicator {
  private hashes = new Set<string>();
  
  isDuplicate(path: string): boolean {
    const data = fileops.readFile(path);
    const h = hash.nxh64(data).toString(16);
    
    if (this.hashes.has(h)) {
      return true;
    }
    
    this.hashes.add(h);
    return false;
  }
  
  findDuplicates(paths: string[]): Map<string, string[]> {
    const byHash = new Map<string, string[]>();
    
    for (const path of paths) {
      const data = fileops.readFile(path);
      const h = hash.nxh64(data).toString(16);
      
      if (!byHash.has(h)) {
        byHash.set(h, []);
      }
      byHash.get(h)!.push(path);
    }
    
    // Return only groups with duplicates
    const duplicates = new Map<string, string[]>();
    for (const [h, files] of byHash) {
      if (files.length > 1) {
        duplicates.set(h, files);
      }
    }
    
    return duplicates;
  }
}
```

### Sharding / Partitioning

```typescript
import { hash } from '@zoryacorporation/pulsar';

class Sharder {
  private numShards: number;
  
  constructor(numShards: number) {
    this.numShards = numShards;
  }
  
  getShard(key: string): number {
    const h = hash.nxhString(key);
    return Number(h % BigInt(this.numShards));
  }
}

// Usage
const sharder = new Sharder(8);

// Keys consistently map to same shard
console.log(sharder.getShard('user:123')); // Always same shard
console.log(sharder.getShard('user:456')); // Different shard
```

### Cache Keys

```typescript
import { hash } from '@zoryacorporation/pulsar';

function cacheKey(...parts: (string | number | boolean)[]): string {
  // Combine all parts into hash
  const combined = parts
    .map(p => hash.nxhString(String(p)))
    .reduce((a, b) => hash.nxhCombine(a, b));
  
  return combined.toString(36); // Base36 for shorter strings
}

// Usage
const key = cacheKey('user', 123, 'profile', true);
// Returns something like: "abc123xyz"
```

---

## Performance Tips

### 1. Use nxhString for Strings

```typescript
import { hash } from '@zoryacorporation/pulsar';

// NOT SUGGESTED: Extra buffer allocation
const h = hash.nxh64(Buffer.from(str));

// SUGGESTED: Direct string hashing
const h = hash.nxhString(str);
```

### 2. Reuse Buffers

```typescript
import { hash } from '@zoryacorporation/pulsar';

// NOT SUGGESTED: New buffer each time
for (const item of items) {
  const h = hash.nxh64(Buffer.from(item.data));
}

// SUGGESTED: If data is already a buffer, use it directly
for (const item of items) {
  const h = hash.nxh64(item.buffer);
}
```

### 3. Use 32-bit When Sufficient

```typescript
import { hash } from '@zoryacorporation/pulsar';

// NOT SUGGESTED: 64-bit for small hash table
const h = hash.nxh64(data);
const idx = Number(h % BigInt(256));

// SUGGESTED: 32-bit is enough
const h = hash.nxh32(data);
const idx = h % 256;
```

### 4. Combine vs Concatenate

```typescript
import { hash } from '@zoryacorporation/pulsar';

// NOT SUGGESTED: Concatenating strings then hashing
const h = hash.nxhString(`${a}:${b}:${c}`);

// SUGGESTED: Combine hashes (no string allocation)
const h = hash.nxhCombine(
  hash.nxhCombine(hash.nxhString(a), hash.nxhString(b)),
  hash.nxhString(c)
);
```

---

## Properties of NXH

### Avalanche Effect

Small input changes produce completely different hashes:

```typescript
import { hash } from '@zoryacorporation/pulsar';

const h1 = hash.nxhString('hello');
const h2 = hash.nxhString('hellp'); // One character different

console.log(h1.toString(16)); // e.g., "a1b2c3d4e5f6"
console.log(h2.toString(16)); // e.g., "9f8e7d6c5b4a" (completely different)

// This almost is starting to look like C programming IN TYPESCRIPT! This is a great example of how Pulsar can bring low-level performance to high-level languages without the hassle of having to learn and manage C/C++ code. This is should also be a great on ramp for people looking to get into systems programming. If you use these patterns enough you might even want to try dropping down to C99 with confidence! And trust me.. It's addicting.. 

```

### Uniform Distribution

Hashes distribute evenly across output space:

```typescript
import { hash } from '@zoryacorporation/pulsar';

const buckets = new Array(16).fill(0);

for (let i = 0; i < 100000; i++) {
  const h = hash.nxhString(`key${i}`);
  const bucket = Number(h % 16n);
  buckets[bucket]++;
}

// All buckets should have roughly equal counts (~6250 each)
console.log(buckets);
```

### Deterministic

Same input always produces same output:

```typescript
import { hash } from '@zoryacorporation/pulsar';

const h1 = hash.nxhString('test');
const h2 = hash.nxhString('test');

console.log(h1 === h2); // Always true
```

---

## Hash Format Conversion

Convert hashes to different string formats:

```typescript
import { hash } from '@zoryacorporation/pulsar';

const h = hash.nxhString('hello world');

// Hexadecimal (most common)
const hex = h.toString(16);
console.log(hex); // "abc123def456..."

// Zero-padded hex (16 chars for 64-bit)
const paddedHex = h.toString(16).padStart(16, '0');

// Base36 (shorter)
const base36 = h.toString(36);

// Decimal
const decimal = h.toString(10);

// Convert back from hex
const restored = BigInt(`0x${hex}`);
```

---

## API Reference

### Hash Functions

| Function | Input | Output | Description |
|----------|-------|--------|-------------|
| `nxh64(buffer, seed?)` | Buffer | BigInt | 64-bit hash |
| `nxh32(buffer, seed?)` | Buffer | number | 32-bit hash |
| `nxh64Alt(buffer, seed?)` | Buffer | BigInt | Alternate 64-bit (for cuckoo) |
| `nxhString(str, seed?)` | string | BigInt | Direct string hash |
| `nxhString32(str, seed?)` | string | number | 32-bit string hash |
| `nxhInt64(value)` | BigInt | BigInt | Hash 64-bit integer |
| `nxhInt32(value)` | number | BigInt | Hash 32-bit integer |
| `nxhCombine(h1, h2)` | BigInt, BigInt | BigInt | Combine two hashes |

### Constants

| Constant | Type | Description |
|----------|------|-------------|
| `SEED_DEFAULT` | BigInt | Default seed (0) |
| `SEED_ALT` | BigInt | Alternate seed for second hash |

### Utility

| Function | Description |
|----------|-------------|
| `version()` | Get module version |

---

## Security Warning

 **NXH is NOT cryptographic!**

Do NOT use for:
- Password hashing (use bcrypt, argon2)
- Digital signatures
- Encryption keys
- Security tokens
- Anything where an attacker could craft collisions

NXH IS great for:
- Hash tables
- Checksums
- Deduplication
- Sharding
- Bloom filters
- Cache keys

---

## Next Steps

- [Dagger Module](./dagger.md) - Hash table using NXH
- [Weave Module](./weave.md) - String primitives with interning
- [PCM Module](./pcm.md) - Bit manipulation utilities
- Try out pulsar! And if you need help getting started, check out:
- [Getting Started Guide](./getting-started.md). Also, feel free to reach out on our
- [GitHub Discussions](https://github.com/ZoryaCorporation/pulsar/discussions).
--------------------------------------------------------------------------------------
