# PCM Module Guide

> **Hardware-Accelerated Bit Manipulation**  
> CPU intrinsics for POPCNT, CLZ, CTZ, rotations, and more

The PCM (Performance Critical Macro) module provides hardware-accelerated bit manipulation operations using compiler intrinsics. These operations compile to single CPU instructions (when available), delivering maximum performance for bit-level operations.

---

## Overview

| Category | Operations |
|----------|------------|
| **Population Count** | `popcount32`, `popcount64` |
| **Leading Zeros** | `clz32`, `clz64` |
| **Trailing Zeros** | `ctz32`, `ctz64` |
| **Rotations** | `rotl32`, `rotr32`, `rotl64`, `rotr64` |
| **Byte Swap** | `bswap16`, `bswap32`, `bswap64` |
| **Power of 2** | `isPowerOf2`, `nextPowerOf2_32`, `nextPowerOf2_64` |
| **Alignment** | `alignUp`, `alignDown`, `isAligned` |
| **Logarithm** | `log2_32`, `log2_64` |

---

## Quick Start

```typescript
import { pcm } from '@zoryacorporation/pulsar';

// Count set bits
const bits = pcm.popcount32(0xFF); // 8

// Find highest set bit position
const leading = pcm.clz32(0x00F00000); // 8

// Round up to power of 2
const aligned = pcm.nextPowerOf2_32(100); // 128

// Align to boundary
const aligned16 = pcm.alignUp(100, 16); // 112
```

---

## Population Count (Hamming Weight)

Count the number of set (1) bits in an integer.

### 32-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.popcount32(0b00000000); // 0
pcm.popcount32(0b00000001); // 1
pcm.popcount32(0b11111111); // 8
pcm.popcount32(0b10101010); // 4
pcm.popcount32(0xFFFFFFFF); // 32
```

### 64-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.popcount64(0n);                  // 0
pcm.popcount64(0xFFFFFFFFFFFFFFFFn); // 64
pcm.popcount64(0x5555555555555555n); // 32 (alternating bits)
```

### Use Cases

```typescript
// Count flags in a bitmask
const permissions = 0b00101101; // read, write, execute flags
const numPermissions = pcm.popcount32(permissions); // 4

// Hamming distance between two values
function hammingDistance(a: number, b: number): number {
  return pcm.popcount32(a ^ b);
}

// Check if exactly one bit is set
function exactlyOneBit(n: number): boolean {
  return n !== 0 && pcm.popcount32(n) === 1;
}
```

---

## Count Leading Zeros (CLZ)

Count zero bits before the first set bit (from MSB).

### 32-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.clz32(0x80000000); // 0  (MSB is set)
pcm.clz32(0x40000000); // 1
pcm.clz32(0x00010000); // 15
pcm.clz32(0x00000001); // 31
pcm.clz32(0x00000000); // 32 (all zeros)
```

### 64-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.clz64(0x8000000000000000n); // 0
pcm.clz64(1n);                   // 63
pcm.clz64(0n);                   // 64
```

### Use Cases

```typescript
// Find position of highest set bit (0-indexed from LSB)
function highestBitPosition(n: number): number {
  return n === 0 ? -1 : 31 - pcm.clz32(n);
}

// Integer log2 (floor)
function log2Floor(n: number): number {
  return 31 - pcm.clz32(n);
}

// Check if value fits in N bits
function fitsInBits(value: number, bits: number): boolean {
  if (value === 0) return true;
  return (32 - pcm.clz32(value)) <= bits;
}
```

---

## Count Trailing Zeros (CTZ)

Count zero bits after the last set bit (from LSB).

### 32-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.ctz32(0x00000001); // 0  (LSB is set)
pcm.ctz32(0x00000002); // 1
pcm.ctz32(0x80000000); // 31
pcm.ctz32(0x00000000); // 32 (all zeros)
pcm.ctz32(0x00001000); // 12
```

### 64-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.ctz64(1n);                   // 0
pcm.ctz64(0x8000000000000000n); // 63
pcm.ctz64(0n);                   // 64
```

### Use Cases

```typescript
// Find position of lowest set bit
function lowestBitPosition(n: number): number {
  return n === 0 ? -1 : pcm.ctz32(n);
}

// Extract and process each set bit
function* iterateBits(mask: number): Generator<number> {
  while (mask !== 0) {
    const bit = pcm.ctz32(mask);
    yield bit;
    mask &= mask - 1; // Clear lowest bit
  }
}

// Usage
for (const bit of iterateBits(0b10110)) {
  console.log(bit); // 1, 2, 4
}

// Check alignment (power of 2)
function alignmentOf(ptr: number): number {
  return ptr === 0 ? 0 : 1 << pcm.ctz32(ptr);
}
```

---

## Bit Rotations

Rotate bits left or right (bits that fall off one end appear at the other).

### Rotate Left

```typescript
import { pcm } from '@zoryacorporation/pulsar';

// 32-bit rotate left
pcm.rotl32(0x12345678, 4);  // 0x23456781
pcm.rotl32(0x80000000, 1);  // 0x00000001
pcm.rotl32(0xF0000000, 4);  // 0x0000000F

// 64-bit rotate left
pcm.rotl64(0x123456789ABCDEF0n, 8); // Rotated value
```

### Rotate Right

```typescript
import { pcm } from '@zoryacorporation/pulsar';

// 32-bit rotate right
pcm.rotr32(0x12345678, 4);  // 0x81234567
pcm.rotr32(0x00000001, 1);  // 0x80000000

// 64-bit rotate right
pcm.rotr64(0x123456789ABCDEF0n, 8); // Rotated value
```

### Use Cases

```typescript
// Simple hash mixing
function mixHash(h: number): number {
  h ^= pcm.rotr32(h, 7);
  h ^= pcm.rotl32(h, 13);
  return h;
}

// Barrel shifter
function barrelShift(value: number, amount: number): number {
  amount = amount & 31; // Normalize to 0-31
  return pcm.rotl32(value, amount);
}
```

---

## Byte Swapping (Endian Conversion)

Reverse the byte order of an integer.

### 16-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.bswap16(0x1234); // 0x3412
pcm.bswap16(0xABCD); // 0xCDAB
```

### 32-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.bswap32(0x12345678); // 0x78563412
pcm.bswap32(0xAABBCCDD); // 0xDDCCBBAA
```

### 64-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.bswap64(0x0102030405060708n); // 0x0807060504030201n
```

### Use Cases

```typescript
// Convert between big-endian and little-endian
function networkToHost32(value: number): number {
  // Network byte order is big-endian
  // If host is little-endian (most x86), swap
  return pcm.bswap32(value);
}

function hostToNetwork32(value: number): number {
  return pcm.bswap32(value);
}

// Read big-endian value from buffer
function readBE32(buffer: Buffer, offset: number): number {
  const le = buffer.readUInt32LE(offset);
  return pcm.bswap32(le);
}
```

---

## Power of 2 Utilities

### Check Power of 2

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.isPowerOf2(1);   // true  (2^0)
pcm.isPowerOf2(2);   // true  (2^1)
pcm.isPowerOf2(4);   // true  (2^2)
pcm.isPowerOf2(3);   // false
pcm.isPowerOf2(5);   // false
pcm.isPowerOf2(256); // true  (2^8)
pcm.isPowerOf2(0);   // false
```

### Next Power of 2

Round up to the next power of 2:

```typescript
import { pcm } from '@zoryacorporation/pulsar';

// 32-bit
pcm.nextPowerOf2_32(0);   // 1
pcm.nextPowerOf2_32(1);   // 1
pcm.nextPowerOf2_32(2);   // 2
pcm.nextPowerOf2_32(3);   // 4
pcm.nextPowerOf2_32(5);   // 8
pcm.nextPowerOf2_32(100); // 128
pcm.nextPowerOf2_32(256); // 256 (already power of 2)

// 64-bit
pcm.nextPowerOf2_64(100n);            // 128n
pcm.nextPowerOf2_64(0x100000000n);    // 0x100000000n
pcm.nextPowerOf2_64(0x100000001n);    // 0x200000000n
```

### Use Cases

```typescript
// Size hash table to power of 2
function createHashTable(minCapacity: number) {
  const capacity = pcm.nextPowerOf2_32(minCapacity);
  return new Array(capacity);
}

// Allocate aligned buffer
function allocateAligned(size: number): Buffer {
  const alignedSize = pcm.nextPowerOf2_32(size);
  return Buffer.alloc(alignedSize);
}
```

---

## Alignment Utilities

### Align Up

Round up to alignment boundary:

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.alignUp(0, 16);   // 0
pcm.alignUp(1, 16);   // 16
pcm.alignUp(15, 16);  // 16
pcm.alignUp(16, 16);  // 16
pcm.alignUp(17, 16);  // 32
pcm.alignUp(100, 64); // 128
```

### Align Down

Round down to alignment boundary:

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.alignDown(0, 16);   // 0
pcm.alignDown(1, 16);   // 0
pcm.alignDown(15, 16);  // 0
pcm.alignDown(16, 16);  // 16
pcm.alignDown(17, 16);  // 16
pcm.alignDown(100, 64); // 64
```

### Check Alignment

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.isAligned(0, 16);   // true
pcm.isAligned(16, 16);  // true
pcm.isAligned(32, 16);  // true
pcm.isAligned(1, 16);   // false
pcm.isAligned(17, 16);  // false
```

### Use Cases

```typescript
// Memory pool with alignment
class AlignedPool {
  private buffer: Buffer;
  private offset = 0;
  
  constructor(size: number) {
    this.buffer = Buffer.alloc(size);
  }
  
  alloc(size: number, alignment = 8): number {
    // Align current offset
    this.offset = pcm.alignUp(this.offset, alignment);
    
    if (this.offset + size > this.buffer.length) {
      throw new Error('Pool exhausted');
    }
    
    const ptr = this.offset;
    this.offset += size;
    return ptr;
  }
}

// Page-aligned allocation
const PAGE_SIZE = 4096;
function pageAlign(size: number): number {
  return pcm.alignUp(size, PAGE_SIZE);
}
```

---

## Integer Logarithm

Compute floor(log2(x)):

### 32-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.log2_32(1);   // 0
pcm.log2_32(2);   // 1
pcm.log2_32(4);   // 2
pcm.log2_32(8);   // 3
pcm.log2_32(15);  // 3 (floor)
pcm.log2_32(16);  // 4
pcm.log2_32(255); // 7
pcm.log2_32(256); // 8
```

### 64-bit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

pcm.log2_64(1n);                   // 0
pcm.log2_64(0x100000000n);         // 32
pcm.log2_64(0x8000000000000000n);  // 63
```

### Use Cases

```typescript
// Number of bits needed to represent value
function bitsRequired(n: number): number {
  return n === 0 ? 1 : pcm.log2_32(n) + 1;
}

// Calculate tree depth
function treeDepth(nodes: number): number {
  return pcm.log2_32(nodes);
}
```

---

## Common Patterns

### Bit Manipulation Toolkit

```typescript
import { pcm } from '@zoryacorporation/pulsar';

const BitUtils = {
  // Set bit at position
  setBit(n: number, pos: number): number {
    return n | (1 << pos);
  },
  
  // Clear bit at position
  clearBit(n: number, pos: number): number {
    return n & ~(1 << pos);
  },
  
  // Toggle bit at position
  toggleBit(n: number, pos: number): number {
    return n ^ (1 << pos);
  },
  
  // Check if bit is set
  testBit(n: number, pos: number): boolean {
    return (n & (1 << pos)) !== 0;
  },
  
  // Isolate lowest set bit
  lowestBit(n: number): number {
    return n & (-n);
  },
  
  // Clear lowest set bit
  clearLowestBit(n: number): number {
    return n & (n - 1);
  },
  
  // Count set bits
  countBits: pcm.popcount32,
  
  // Find lowest set bit position
  lowestBitPos(n: number): number {
    return n === 0 ? -1 : pcm.ctz32(n);
  },
  
  // Find highest set bit position
  highestBitPos(n: number): number {
    return n === 0 ? -1 : 31 - pcm.clz32(n);
  }
};
```

### Bitmap Implementation

```typescript
import { pcm } from '@zoryacorporation/pulsar';

class Bitmap {
  private bits: Uint32Array;
  private _size: number;
  
  constructor(size: number) {
    this._size = size;
    this.bits = new Uint32Array(Math.ceil(size / 32));
  }
  
  set(index: number): void {
    const word = Math.floor(index / 32);
    const bit = index % 32;
    this.bits[word] |= (1 << bit);
  }
  
  clear(index: number): void {
    const word = Math.floor(index / 32);
    const bit = index % 32;
    this.bits[word] &= ~(1 << bit);
  }
  
  test(index: number): boolean {
    const word = Math.floor(index / 32);
    const bit = index % 32;
    return (this.bits[word] & (1 << bit)) !== 0;
  }
  
  // Count total set bits
  count(): number {
    let total = 0;
    for (const word of this.bits) {
      total += pcm.popcount32(word);
    }
    return total;
  }
  
  // Find first set bit
  findFirst(): number {
    for (let i = 0; i < this.bits.length; i++) {
      if (this.bits[i] !== 0) {
        return i * 32 + pcm.ctz32(this.bits[i]);
      }
    }
    return -1;
  }
  
  // Find first clear bit
  findFirstClear(): number {
    for (let i = 0; i < this.bits.length; i++) {
      const inverted = ~this.bits[i];
      if (inverted !== 0) {
        const pos = i * 32 + pcm.ctz32(inverted);
        return pos < this._size ? pos : -1;
      }
    }
    return -1;
  }
  
  // Iterate all set bits
  *[Symbol.iterator](): Generator<number> {
    for (let i = 0; i < this.bits.length; i++) {
      let word = this.bits[i];
      while (word !== 0) {
        const bit = pcm.ctz32(word);
        yield i * 32 + bit;
        word &= word - 1;
      }
    }
  }
}

// Usage
const bitmap = new Bitmap(1000);
bitmap.set(5);
bitmap.set(100);
bitmap.set(500);

console.log(bitmap.count());     // 3
console.log(bitmap.findFirst()); // 5

for (const index of bitmap) {
  console.log(index); // 5, 100, 500
}
```

### Bit Field Packing

```typescript
import { pcm } from '@zoryacorporation/pulsar';

// Pack multiple values into single 32-bit integer
interface PackedColor {
  r: number; // 8 bits
  g: number; // 8 bits
  b: number; // 8 bits
  a: number; // 8 bits
}

function packColor(color: PackedColor): number {
  return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
}

function unpackColor(packed: number): PackedColor {
  return {
    a: (packed >> 24) & 0xFF,
    r: (packed >> 16) & 0xFF,
    g: (packed >> 8) & 0xFF,
    b: packed & 0xFF
  };
}

// Efficient color processing
function blendColors(c1: number, c2: number): number {
  // Extract and blend each channel
  const r = (((c1 >> 16) & 0xFF) + ((c2 >> 16) & 0xFF)) >> 1;
  const g = (((c1 >> 8) & 0xFF) + ((c2 >> 8) & 0xFF)) >> 1;
  const b = ((c1 & 0xFF) + (c2 & 0xFF)) >> 1;
  const a = (((c1 >> 24) & 0xFF) + ((c2 >> 24) & 0xFF)) >> 1;
  
  return (a << 24) | (r << 16) | (g << 8) | b;
}
```

---

## Performance Notes

### Hardware Acceleration

Most PCM operations map to single CPU instructions:

| Operation | x86 Instruction | ARM Instruction |
|-----------|-----------------|-----------------|
| `popcount32` | `POPCNT` | `CNT` |
| `clz32` | `LZCNT` / `BSR` | `CLZ` |
| `ctz32` | `TZCNT` / `BSF` | `RBIT` + `CLZ` |
| `rotl32` | `ROL` | `ROR` (with negated shift) |
| `bswap32` | `BSWAP` | `REV` |

These execute in 1-3 cycles on modern CPUs.

### When to Use PCM

✅ **Use PCM for:**
- Hot loops with bit manipulation
- Hash functions
- Compression algorithms
- Memory allocators
- Bitmap operations
- Endian conversion

❌ **Don't use PCM for:**
- Single operations (JS overhead may dominate)
- Non-performance-critical code
- When readability matters more than speed

---

## API Reference

### Population Count

| Function | Description |
|----------|-------------|
| `popcount32(x)` | Count set bits in 32-bit |
| `popcount64(x)` | Count set bits in 64-bit |

### Leading/Trailing Zeros

| Function | Description |
|----------|-------------|
| `clz32(x)` | Count leading zeros, 32-bit |
| `clz64(x)` | Count leading zeros, 64-bit |
| `ctz32(x)` | Count trailing zeros, 32-bit |
| `ctz64(x)` | Count trailing zeros, 64-bit |

### Rotations

| Function | Description |
|----------|-------------|
| `rotl32(x, r)` | Rotate left 32-bit |
| `rotr32(x, r)` | Rotate right 32-bit |
| `rotl64(x, r)` | Rotate left 64-bit |
| `rotr64(x, r)` | Rotate right 64-bit |

### Byte Swap

| Function | Description |
|----------|-------------|
| `bswap16(x)` | Byte swap 16-bit |
| `bswap32(x)` | Byte swap 32-bit |
| `bswap64(x)` | Byte swap 64-bit |

### Power of 2

| Function | Description |
|----------|-------------|
| `isPowerOf2(x)` | Check if power of 2 |
| `nextPowerOf2_32(x)` | Round up to power of 2, 32-bit |
| `nextPowerOf2_64(x)` | Round up to power of 2, 64-bit |

### Alignment

| Function | Description |
|----------|-------------|
| `alignUp(x, alignment)` | Round up to alignment |
| `alignDown(x, alignment)` | Round down to alignment |
| `isAligned(x, alignment)` | Check if aligned |

### Logarithm

| Function | Description |
|----------|-------------|
| `log2_32(x)` | Integer log2, 32-bit |
| `log2_64(x)` | Integer log2, 64-bit |

### Utility

| Function | Description |
|----------|-------------|
| `version()` | Get module version |

---

## Next Steps

- [Hash Module](./hash.md) - Hashing uses bit manipulation
- [Dagger Module](./dagger.md) - Hash tables benefit from PCM
- [Compress Module](./compress.md) - Compression uses bit operations
