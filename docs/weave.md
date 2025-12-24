# Weave Module Guide

> **High-Performance String Primitives**  
> Native string data structures with arena allocation and interning

The Weave module provides four powerful string primitives that deliver native-speed string manipulation for Node.js applications. Whether you're building a text editor, parsing large documents, or handling millions of repeated strings, Weave has you covered.

---

## Overview

| Class | Purpose | Best For |
|-------|---------|----------|
| **Weave** | Mutable string buffer | String building, search/replace |
| **Cord** | Rope data structure | Large documents, text editors |
| **Tablet** | String interning | Symbol tables, deduplication |
| **Arena** | Bump allocator | Temporary strings, parsing |

---

## Quick Start

```typescript
import { weave } from '@zoryacorporation/pulsar';

// Create a mutable string builder
const builder = weave.Weave.from('Hello');
builder.append(', World!');
console.log(builder.toString()); // "Hello, World!"

// String interning for deduplication
const tablet = weave.Tablet.create();
const id1 = tablet.intern('hello');
const id2 = tablet.intern('hello');
console.log(id1 === id2); // true - same ID for same string
```

---

## Weave Class

`Weave` is a high-performance mutable string buffer—think `StringBuilder` but with native speed and additional operations like find, replace, and split.

### Creating a Weave

```typescript
import { weave } from '@zoryacorporation/pulsar';

// Create empty
const empty = weave.Weave.create();

// Create from string
const fromStr = weave.Weave.from('initial content');
```

### Building Strings

```typescript
const builder = weave.Weave.from('Hello');

// Append to end
builder.append(' World');
builder.append('!');

// Prepend to beginning
builder.prepend('>>> ');

console.log(builder.toString()); // ">>> Hello World!"
```

### Capacity Management

For optimal performance when you know the final size:

```typescript
const builder = weave.Weave.create();

// Pre-allocate capacity
builder.reserve(10000);

// Check current state
console.log(builder.length);   // Current string length
console.log(builder.capacity); // Allocated buffer size

// Build without reallocations
for (let i = 0; i < 1000; i++) {
  builder.append(`Item ${i}\n`);
}
```

### Search Operations

```typescript
const text = weave.Weave.from('The quick brown fox jumps over the lazy dog');

// Find position (-1 if not found)
const pos = text.find('fox');
console.log(pos); // 16

// Check if contains
if (text.contains('quick')) {
  console.log('Found it!');
}
```

### Replace Operations

```typescript
const template = weave.Weave.from('Hello {{name}}, welcome to {{place}}!');

// Replace first occurrence
template.replace('{{name}}', 'Alice');

// Replace all occurrences
const repeated = weave.Weave.from('apple apple apple');
repeated.replaceAll('apple', 'orange');
console.log(repeated.toString()); // "orange orange orange"
```

### Whitespace and Splitting

```typescript
// Trim whitespace
const padded = weave.Weave.from('   trimmed   ');
padded.trim();
console.log(padded.toString()); // "trimmed"

// Split by delimiter
const csv = weave.Weave.from('apple,banana,cherry');
const fruits = csv.split(',');
console.log(fruits); // ['apple', 'banana', 'cherry']
```

### Method Chaining

Weave supports fluent method chaining:

```typescript
const result = weave.Weave.create()
  .append('Hello')
  .append(' ')
  .append('World')
  .trim()
  .toString();
```

---

## Cord Class

`Cord` implements a rope data structure for efficient large string manipulation. Unlike regular strings that require O(n) concatenation, Cord provides O(log n) concatenation and substring operations.

**Perfect for:**
- Text editors handling large documents
- Log aggregation
- Building large strings from many small pieces

### Creating a Cord

```typescript
import { weave } from '@zoryacorporation/pulsar';

const cord = weave.Cord.create();
```

### Appending Chunks

```typescript
const doc = weave.Cord.create();

// Append multiple chunks efficiently
doc.append('Chapter 1\n');
doc.append('It was a dark and stormy night...\n');
doc.append('\n');
doc.append('Chapter 2\n');
doc.append('The morning brought clarity...\n');

// Check stats
console.log(doc.length);     // Total character count
console.log(doc.chunkCount); // Number of internal chunks
```

### Materializing to String

```typescript
// When you need the final string
const finalText = doc.toString();
```

### Clearing Content

```typescript
doc.clear(); // Remove all chunks
```

### Use Case: Building a Large Document

```typescript
function buildReport(data: ReportData[]): string {
  const doc = weave.Cord.create();
  
  // Header
  doc.append('='.repeat(60) + '\n');
  doc.append('MONTHLY REPORT\n');
  doc.append('='.repeat(60) + '\n\n');
  
  // Body - each section appends efficiently
  for (const section of data) {
    doc.append(`## ${section.title}\n`);
    doc.append(section.content);
    doc.append('\n\n');
  }
  
  // Footer
  doc.append('-'.repeat(60) + '\n');
  doc.append(`Generated: ${new Date().toISOString()}\n`);
  
  return doc.toString();
}
```

---

## Tablet Class

`Tablet` provides string interning—storing unique strings once and returning IDs. This is essential for:

- Symbol tables in compilers/interpreters
- Deduplicating repeated strings
- Reducing memory when storing many identical strings

### Creating a Tablet

```typescript
import { weave } from '@zoryacorporation/pulsar';

const tablet = weave.Tablet.create();
```

### Interning Strings

```typescript
const tablet = weave.Tablet.create();

// Intern strings - same string returns same ID
const id1 = tablet.intern('hello');
const id2 = tablet.intern('hello');
const id3 = tablet.intern('world');

console.log(id1 === id2); // true
console.log(id1 === id3); // false

// IDs are BigInt for 64-bit precision
console.log(typeof id1); // 'bigint'
```

### Retrieving Strings

```typescript
const tablet = weave.Tablet.create();

const helloId = tablet.intern('hello');
const worldId = tablet.intern('world');

// Get string by ID
console.log(tablet.get(helloId)); // 'hello'
console.log(tablet.get(worldId)); // 'world'

// Non-existent ID returns null
console.log(tablet.get(999n)); // null
```

### Statistics

```typescript
const tablet = weave.Tablet.create();

tablet.intern('apple');
tablet.intern('banana');
tablet.intern('apple'); // Same ID as first 'apple'

console.log(tablet.count);  // 2 (unique strings)
console.log(tablet.memory); // Memory used in bytes
```

### Use Case: Symbol Table for Parser

```typescript
interface Token {
  type: string;
  symbolId: bigint; // Store ID instead of string
  line: number;
  column: number;
}

class Lexer {
  private symbols = weave.Tablet.create();
  
  tokenize(source: string): Token[] {
    const tokens: Token[] = [];
    
    // When we encounter an identifier:
    const identifier = 'myVariable';
    
    tokens.push({
      type: 'IDENTIFIER',
      symbolId: this.symbols.intern(identifier), // Deduplicated!
      line: 1,
      column: 5
    });
    
    return tokens;
  }
  
  getSymbol(id: bigint): string | null {
    return this.symbols.get(id);
  }
}
```

### Use Case: Deduplicating Configuration Values

```typescript
class ConfigStore {
  private strings = weave.Tablet.create();
  private values = new Map<string, bigint>();
  
  set(key: string, value: string): void {
    // Intern the value - if we've seen it before, we reuse it
    this.values.set(key, this.strings.intern(value));
  }
  
  get(key: string): string | undefined {
    const id = this.values.get(key);
    if (id === undefined) return undefined;
    return this.strings.get(id) ?? undefined;
  }
  
  stats(): { keys: number; uniqueValues: number; memory: number } {
    return {
      keys: this.values.size,
      uniqueValues: this.strings.count,
      memory: this.strings.memory
    };
  }
}
```

---

## Arena Class

`Arena` is a bump allocator optimized for temporary strings. Instead of individual allocations, it allocates from a contiguous block and can deallocate everything at once.

**Perfect for:**
- Parsing (allocate during parse, free all at end)
- Request handling (allocate per request, free on completion)
- Any scope-based allocation pattern

### Creating an Arena

```typescript
import { weave } from '@zoryacorporation/pulsar';

// Default 4KB initial capacity
const arena = weave.Arena.create();

// Custom initial capacity
const largeArena = weave.Arena.create(1024 * 1024); // 1MB
```

### Allocating Strings

```typescript
const arena = weave.Arena.create();

// Allocate and store a string, returns offset
const offset1 = arena.allocString('hello');
const offset2 = arena.allocString('world');

// Retrieve strings by offset
console.log(arena.getString(offset1)); // 'hello'
console.log(arena.getString(offset2)); // 'world'
```

### Raw Byte Allocation

```typescript
const arena = weave.Arena.create();

// Allocate raw bytes
const offset = arena.alloc(256); // Returns offset
```

### Temporary Scopes

Nested allocations with automatic cleanup:

```typescript
const arena = weave.Arena.create();

// Outer allocations
const permanent = arena.allocString('permanent');

// Begin temporary scope
const mark = arena.tempBegin();

// Temporary allocations
arena.allocString('temp1');
arena.allocString('temp2');
arena.allocString('temp3');

// End scope - deallocates temp1, temp2, temp3
arena.tempEnd(mark);

// 'permanent' is still valid
console.log(arena.getString(permanent)); // 'permanent'
```

### Resetting the Arena

```typescript
const arena = weave.Arena.create();

// Do a bunch of work...
arena.allocString('...');
arena.allocString('...');

// Reset - deallocates everything, keeps capacity
arena.reset();
```

### Statistics

```typescript
const arena = weave.Arena.create(4096);

arena.allocString('test string');

const stats = arena.stats();
console.log(stats.totalAllocated); // Bytes allocated
console.log(stats.totalCapacity);  // Total capacity
console.log(stats.blockCount);     // Number of memory blocks
```

### Use Case: Request-Scoped Parsing

```typescript
interface ParseResult {
  type: string;
  data: any;
}

class RequestParser {
  private arena = weave.Arena.create(64 * 1024); // 64KB per request
  
  parse(body: string): ParseResult {
    // Reset arena from previous request
    this.arena.reset();
    
    // All parsing allocations go to arena
    const lines = body.split('\n');
    const tokens: number[] = [];
    
    for (const line of lines) {
      tokens.push(this.arena.allocString(line.trim()));
    }
    
    // Process tokens...
    // When request completes, next request will reset()
    
    return { type: 'success', data: {} };
  }
}
```

### Use Case: Nested Parse Operations

```typescript
function parseExpression(arena: weave.Arena, expr: string): any {
  // Save arena state
  const mark = arena.tempBegin();
  
  try {
    // Temporary strings during parsing
    const parts = expr.split('+');
    for (const part of parts) {
      arena.allocString(part.trim());
    }
    
    // Parse recursively...
    return { success: true };
    
  } finally {
    // Always restore arena state
    arena.tempEnd(mark);
  }
}
```

---

## Performance Tips

### 1. Pre-allocate Weave Capacity

```typescript
// ❌ Multiple reallocations
const slow = weave.Weave.create();
for (let i = 0; i < 10000; i++) {
  slow.append(`line ${i}\n`);
}

// ✅ Single allocation
const fast = weave.Weave.create();
fast.reserve(200000); // Estimate final size
for (let i = 0; i < 10000; i++) {
  fast.append(`line ${i}\n`);
}
```

### 2. Use Cord for Large Documents

```typescript
// ❌ Quadratic time complexity
let text = '';
for (let i = 0; i < 10000; i++) {
  text += `chunk ${i}\n`; // Each += copies entire string
}

// ✅ O(log n) per operation
const cord = weave.Cord.create();
for (let i = 0; i < 10000; i++) {
  cord.append(`chunk ${i}\n`);
}
const text = cord.toString(); // Single materialization
```

### 3. Intern Repeated Strings

```typescript
// ❌ Many duplicate strings in memory
const tags: string[] = [];
for (const item of items) {
  tags.push(item.category); // 'electronics' stored 1000x
}

// ✅ Single storage per unique string
const tablet = weave.Tablet.create();
const tagIds: bigint[] = [];
for (const item of items) {
  tagIds.push(tablet.intern(item.category)); // ID only
}
```

### 4. Arena for Batch Processing

```typescript
// ❌ Many small allocations
function processItems(items: string[]) {
  for (const item of items) {
    const temp = item.toUpperCase(); // JS GC overhead
    // ...
  }
}

// ✅ Single arena allocation
function processItemsFast(items: string[]) {
  const arena = weave.Arena.create(items.length * 100);
  
  for (const item of items) {
    const offset = arena.allocString(item.toUpperCase());
    // ...
  }
  
  arena.reset(); // All freed at once
}
```

---

## Real-World Example: Log Processor

```typescript
import { weave } from '@zoryacorporation/pulsar';

interface LogEntry {
  timestamp: bigint; // Interned
  level: bigint;     // Interned
  message: string;
}

class LogProcessor {
  private strings = weave.Tablet.create();
  private parseArena = weave.Arena.create(64 * 1024);
  private output = weave.Cord.create();
  
  processFile(lines: string[]): string {
    const entries: LogEntry[] = [];
    
    for (const line of lines) {
      // Reset arena for each line
      this.parseArena.reset();
      
      // Parse log entry
      const entry = this.parseLine(line);
      if (entry) entries.push(entry);
    }
    
    // Build output document
    this.output.append('LOG REPORT\n');
    this.output.append('='.repeat(40) + '\n\n');
    
    for (const entry of entries) {
      const level = this.strings.get(entry.level)!;
      const ts = this.strings.get(entry.timestamp)!;
      this.output.append(`[${ts}] ${level}: ${entry.message}\n`);
    }
    
    return this.output.toString();
  }
  
  private parseLine(line: string): LogEntry | null {
    const match = line.match(/\[(.+?)\] (\w+): (.+)/);
    if (!match) return null;
    
    return {
      timestamp: this.strings.intern(match[1]),
      level: this.strings.intern(match[2]),
      message: match[3]
    };
  }
  
  stats() {
    return {
      uniqueStrings: this.strings.count,
      stringMemory: this.strings.memory,
      outputChunks: this.output.chunkCount,
      outputLength: this.output.length
    };
  }
}
```

---

## API Reference

### Weave

| Method | Description |
|--------|-------------|
| `Weave.create()` | Create empty buffer |
| `Weave.from(str)` | Create from string |
| `append(str)` | Append to end |
| `prepend(str)` | Prepend to start |
| `toString()` | Get string content |
| `length` | Current length |
| `capacity` | Buffer capacity |
| `reserve(n)` | Reserve capacity |
| `find(needle)` | Find position (-1 if not found) |
| `contains(needle)` | Check if contains |
| `replace(old, new)` | Replace first |
| `replaceAll(old, new)` | Replace all |
| `trim()` | Trim whitespace |
| `split(delim)` | Split by delimiter |

### Cord

| Method | Description |
|--------|-------------|
| `Cord.create()` | Create empty cord |
| `append(str)` | Append chunk |
| `length` | Total length |
| `chunkCount` | Number of chunks |
| `toString()` | Materialize to string |
| `clear()` | Remove all chunks |

### Tablet

| Method | Description |
|--------|-------------|
| `Tablet.create()` | Create new tablet |
| `intern(str)` | Intern string, returns ID |
| `get(id)` | Get string by ID |
| `count` | Number of unique strings |
| `memory` | Memory usage in bytes |

### Arena

| Method | Description |
|--------|-------------|
| `Arena.create(cap?)` | Create with optional capacity |
| `alloc(size)` | Allocate bytes |
| `allocString(str)` | Allocate string |
| `getString(offset)` | Get string at offset |
| `tempBegin()` | Begin temporary scope |
| `tempEnd(mark)` | End temporary scope |
| `reset()` | Deallocate everything |
| `stats()` | Get allocation stats |

---

## Next Steps

- [Compress Module](./compress.md) - Learn about native compression
- [FileOps Module](./fileops.md) - High-performance file operations
- [Hash Module](./hash.md) - Fast non-cryptographic hashing
