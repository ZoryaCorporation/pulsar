# Dagger Module Guide

> **High-Performance Hash Table**  
> Robin Hood hashing with O(1) operations and excellent cache behavior

The Dagger module provides a native hash table implementation using Robin Hood hashing with probe sequence length (PSL) tracking. It delivers 7x faster insertions than JavaScript `Map` for large datasets while maintaining O(1) average-case performance. Dagger, NXH **Zorya's custom Hash method** and Weave **custom string libary** is also the backbone and major workhorse for the ZORYA C SDK. In addition to its speed, Dagger offers low memory overhead and predictable performance even under high load factors. 

Upstreaming the Zorya C developments and engineering API's into TypeScript/JavaScript via Pulsar allows Node.js and Electron applications to leverage these performance benefits natively. We will cover the Dagger API, usage patterns, and performance characteristics in this guide.

---

## Overview

| Feature | Description |
|---------|-------------|
| **Algorithm** | Robin Hood hashing with PSL tracking |
| **Performance** | O(1) average, excellent worst-case |
| **Key Type** | String |
| **Value Type** | String (serialize objects to JSON) |
| **Load Factor** | 70% before resize |

---

## Quick Start

```typescript
import { dagger } from '@zoryacorporation/pulsar';

// Create a table
const table = new dagger.DaggerTable();

// Set values
table.set('name', 'Alice');
table.set('age', '30');

// Get values
console.log(table.get('name')); // 'Alice'

// Check existence
if (table.has('name')) {
  console.log('Found!');
}

// Iterate
for (const [key, value] of table) {
  console.log(`${key}: ${value}`);
}
```

---

## Creating Tables

### Basic Construction

```typescript
import { dagger } from '@zoryacorporation/pulsar';

const table = new dagger.DaggerTable();
```

### Starting with Data

```typescript
const table = new dagger.DaggerTable();

// Populate with initial data
const initialData = {
  host: 'localhost',
  port: '8080',
  debug: 'true'
};

for (const [key, value] of Object.entries(initialData)) {
  table.set(key, value);
}
```

---

## Basic Operations

### Set Values

```typescript
import { dagger } from '@zoryacorporation/pulsar';

const table = new dagger.DaggerTable();

// Set returns `this` for chaining
table
  .set('key1', 'value1')
  .set('key2', 'value2')
  .set('key3', 'value3');

// Overwrite existing
table.set('key1', 'new value');
```

### Get Values

```typescript
const value = table.get('key1');

if (value !== undefined) {
  console.log(`Found: ${value}`);
} else {
  console.log('Not found');
}
```

### Check Existence

```typescript
if (table.has('key1')) {
  // Key exists
}

// Common pattern
const value = table.has('key') ? table.get('key') : 'default';
```

### Delete Keys

```typescript
// Returns true if key existed
const deleted = table.delete('key1');
console.log(deleted); // true or false
```

### Clear All

```typescript
table.clear();
console.log(table.size); // 0
```

---

## Size and Iteration

### Get Size

```typescript
const table = new dagger.DaggerTable();
table.set('a', '1').set('b', '2').set('c', '3');

console.log(table.size); // 3
```

### Get All Keys

```typescript
const keys = table.keys();
console.log(keys); // ['a', 'b', 'c']
```

### Get All Values

```typescript
const values = table.values();
console.log(values); // ['1', '2', '3']
```

### Iterate with for-of

```typescript
// Iterate over [key, value] pairs
for (const [key, value] of table) {
  console.log(`${key} = ${value}`);
}

// Using entries() explicitly
for (const [key, value] of table.entries()) {
  console.log(`${key} = ${value}`);
}
```

### forEach Callback

```typescript
table.forEach((value, key, t) => {
  console.log(`${key}: ${value}`);
});
```

---

## Working with Objects

Since Dagger stores strings, serialize objects to JSON:

```typescript
import { dagger } from '@zoryacorporation/pulsar';

const table = new dagger.DaggerTable();

// Store object as JSON
const user = { name: 'Alice', age: 30, admin: true };
table.set('user:123', JSON.stringify(user));

// Retrieve and parse
const stored = table.get('user:123');
if (stored) {
  const retrieved = JSON.parse(stored);
  console.log(retrieved.name); // 'Alice'
}
```

### Type-Safe Wrapper

```typescript
import { dagger } from '@zoryacorporation/pulsar';

class TypedDagger<T> {
  private table = new dagger.DaggerTable();
  
  set(key: string, value: T): this {
    this.table.set(key, JSON.stringify(value));
    return this;
  }
  
  get(key: string): T | undefined {
    const stored = this.table.get(key);
    return stored ? JSON.parse(stored) : undefined;
  }
  
  has(key: string): boolean {
    return this.table.has(key);
  }
  
  delete(key: string): boolean {
    return this.table.delete(key);
  }
  
  get size(): number {
    return this.table.size;
  }
  
  *entries(): Generator<[string, T]> {
    for (const [key, value] of this.table) {
      yield [key, JSON.parse(value)];
    }
  }
  
  [Symbol.iterator]() {
    return this.entries();
  }
}

// Usage
interface User {
  name: string;
  age: number;
}

const users = new TypedDagger<User>();
users.set('alice', { name: 'Alice', age: 30 });
users.set('bob', { name: 'Bob', age: 25 });

const alice = users.get('alice');
console.log(alice?.name); // 'Alice'
```

---

## Common Use Cases

### Configuration Store

```typescript
import { dagger } from '@zoryacorporation/pulsar';

class ConfigStore {
  private config = new dagger.DaggerTable();
  
  load(data: Record<string, string>): void {
    for (const [key, value] of Object.entries(data)) {
      this.config.set(key, value);
    }
  }
  
  get(key: string, defaultValue?: string): string {
    const value = this.config.get(key);
    return value !== undefined ? value : (defaultValue ?? '');
  }
  
  getInt(key: string, defaultValue = 0): number {
    const value = this.config.get(key);
    return value !== undefined ? parseInt(value, 10) : defaultValue;
  }
  
  getBool(key: string, defaultValue = false): boolean {
    const value = this.config.get(key);
    if (value === undefined) return defaultValue;
    return value === 'true' || value === '1' || value === 'yes';
  }
  
  toObject(): Record<string, string> {
    const result: Record<string, string> = {};
    for (const [key, value] of this.config) {
      result[key] = value;
    }
    return result;
  }
}

// Usage
const config = new ConfigStore();
config.load({
  'server.host': 'localhost',
  'server.port': '8080',
  'debug': 'true'
});

console.log(config.get('server.host'));     // 'localhost'
console.log(config.getInt('server.port'));  // 8080
console.log(config.getBool('debug'));       // true
```

### Session Store

```typescript
import { dagger } from '@zoryacorporation/pulsar';

interface Session {
  userId: string;
  createdAt: number;
  data: Record<string, any>;
}

class SessionStore {
  private sessions = new dagger.DaggerTable();
  
  create(sessionId: string, userId: string): Session {
    const session: Session = {
      userId,
      createdAt: Date.now(),
      data: {}
    };
    this.sessions.set(sessionId, JSON.stringify(session));
    return session;
  }
  
  get(sessionId: string): Session | null {
    const stored = this.sessions.get(sessionId);
    return stored ? JSON.parse(stored) : null;
  }
  
  update(sessionId: string, data: Partial<Session['data']>): boolean {
    const session = this.get(sessionId);
    if (!session) return false;
    
    session.data = { ...session.data, ...data };
    this.sessions.set(sessionId, JSON.stringify(session));
    return true;
  }
  
  destroy(sessionId: string): boolean {
    return this.sessions.delete(sessionId);
  }
  
  gc(maxAge: number): number {
    const now = Date.now();
    const toDelete: string[] = [];
    
    for (const [sessionId, stored] of this.sessions) {
      const session: Session = JSON.parse(stored);
      if (now - session.createdAt > maxAge) {
        toDelete.push(sessionId);
      }
    }
    
    for (const id of toDelete) {
      this.sessions.delete(id);
    }
    
    return toDelete.length;
  }
}
```

### Symbol Table (Compiler)

```typescript
import { dagger } from '@zoryacorporation/pulsar';

interface Symbol {
  name: string;
  type: string;
  scope: number;
  line: number;
}

class SymbolTable {
  private symbols = new dagger.DaggerTable();
  private scopeStack: number[] = [0];
  private nextScope = 1;
  
  get currentScope(): number {
    return this.scopeStack[this.scopeStack.length - 1];
  }
  
  enterScope(): void {
    this.scopeStack.push(this.nextScope++);
  }
  
  exitScope(): void {
    if (this.scopeStack.length > 1) {
      this.scopeStack.pop();
    }
  }
  
  define(name: string, type: string, line: number): void {
    const key = `${this.currentScope}:${name}`;
    const symbol: Symbol = { name, type, scope: this.currentScope, line };
    this.symbols.set(key, JSON.stringify(symbol));
  }
  
  lookup(name: string): Symbol | null {
    // Search from current scope up to global
    for (let i = this.scopeStack.length - 1; i >= 0; i--) {
      const key = `${this.scopeStack[i]}:${name}`;
      const stored = this.symbols.get(key);
      if (stored) return JSON.parse(stored);
    }
    return null;
  }
  
  isDefined(name: string): boolean {
    return this.lookup(name) !== null;
  }
}

// Usage
const symbols = new SymbolTable();

symbols.define('x', 'int', 1);

symbols.enterScope();
symbols.define('y', 'string', 5);
console.log(symbols.lookup('x')); // Found in parent scope
console.log(symbols.lookup('y')); // Found in current scope

symbols.exitScope();
console.log(symbols.lookup('y')); // null - out of scope
```

### LRU Cache

```typescript
import { dagger } from '@zoryacorporation/pulsar';

class LRUCache<T> {
  private cache = new dagger.DaggerTable();
  private order: string[] = [];
  private maxSize: number;
  
  constructor(maxSize: number) {
    this.maxSize = maxSize;
  }
  
  get(key: string): T | undefined {
    const stored = this.cache.get(key);
    if (stored === undefined) return undefined;
    
    // Move to end (most recently used)
    this.order = this.order.filter(k => k !== key);
    this.order.push(key);
    
    return JSON.parse(stored);
  }
  
  set(key: string, value: T): void {
    // Remove if already exists
    if (this.cache.has(key)) {
      this.order = this.order.filter(k => k !== key);
    }
    
    // Evict oldest if at capacity
    while (this.order.length >= this.maxSize) {
      const oldest = this.order.shift()!;
      this.cache.delete(oldest);
    }
    
    // Add new entry
    this.cache.set(key, JSON.stringify(value));
    this.order.push(key);
  }
  
  has(key: string): boolean {
    return this.cache.has(key);
  }
  
  get size(): number {
    return this.cache.size;
  }
}

// Usage
const cache = new LRUCache<{ data: string }>(100);

cache.set('key1', { data: 'value1' });
cache.set('key2', { data: 'value2' });

const item = cache.get('key1'); // Moves to most recent
```

### Multi-Map (Multiple Values per Key)

```typescript
import { dagger } from '@zoryacorporation/pulsar';

class MultiMap<T> {
  private map = new dagger.DaggerTable();
  
  add(key: string, value: T): void {
    const stored = this.map.get(key);
    const values: T[] = stored ? JSON.parse(stored) : [];
    values.push(value);
    this.map.set(key, JSON.stringify(values));
  }
  
  get(key: string): T[] {
    const stored = this.map.get(key);
    return stored ? JSON.parse(stored) : [];
  }
  
  has(key: string): boolean {
    return this.map.has(key);
  }
  
  remove(key: string, value: T): boolean {
    const stored = this.map.get(key);
    if (!stored) return false;
    
    const values: T[] = JSON.parse(stored);
    const idx = values.findIndex(v => JSON.stringify(v) === JSON.stringify(value));
    if (idx === -1) return false;
    
    values.splice(idx, 1);
    
    if (values.length === 0) {
      this.map.delete(key);
    } else {
      this.map.set(key, JSON.stringify(values));
    }
    
    return true;
  }
  
  count(key: string): number {
    const stored = this.map.get(key);
    return stored ? JSON.parse(stored).length : 0;
  }
}

// Usage
const tags = new MultiMap<string>();

tags.add('post:1', 'javascript');
tags.add('post:1', 'nodejs');
tags.add('post:1', 'pulsar');

console.log(tags.get('post:1')); // ['javascript', 'nodejs', 'pulsar']
console.log(tags.count('post:1')); // 3
```

---

## Performance Characteristics

### Robin Hood Hashing

Dagger uses Robin Hood hashing, which "steals" from rich (low PSL) slots to give to poor (high PSL) entries. This results in:

- **Lower variance** in probe lengths
- **Better cache performance** (shorter probe sequences)
- **Faster lookups** on average

### Load Factor

Tables resize at 70% load factor:

```typescript
import { dagger } from '@zoryacorporation/pulsar';

console.log(dagger.LOAD_FACTOR_PERCENT); // 70
```

### PSL Threshold

When probe sequence length exceeds threshold, rehashing occurs:

```typescript
import { dagger } from '@zoryacorporation/pulsar';

console.log(dagger.PSL_THRESHOLD); // Implementation-specific
```

---

## Performance Tips

### 1. Batch Operations

```typescript
// ❌ Many individual operations
for (const item of items) {
  const value = table.get(item.key);
  if (value) {
    // process
  }
}

// ✅ Consider data locality
// Group related keys together for better cache behavior
```

### 2. Key Design

```typescript
// ❌ Long composite keys
table.set(`user:${userId}:session:${sessionId}:data:${dataKey}`, value);

// ✅ Shorter keys when possible
table.set(`u:${userId}:s:${sessionId}`, value);

// Or use hierarchical tables
userSessions.set(sessionId, value);
```

### 3. Avoid Unnecessary Lookups

```typescript
// ❌ Double lookup
if (table.has(key)) {
  const value = table.get(key);
}

// ✅ Single lookup
const value = table.get(key);
if (value !== undefined) {
  // use value
}
```

### 4. Serialize Once

```typescript
// ❌ Multiple serializations
for (let i = 0; i < 1000; i++) {
  const data = { complex: 'object', index: i };
  table.set(`key${i}`, JSON.stringify(data));
}

// ✅ Reuse serialization logic, batch when possible
const entries = items.map(item => [item.key, JSON.stringify(item.value)]);
for (const [key, value] of entries) {
  table.set(key, value);
}
```

---

## Comparison with Map

| Feature | `DaggerTable` | JavaScript `Map` |
|---------|---------------|------------------|
| Insert 100K | ~12ms | ~89ms |
| Lookup 100K | ~8ms | ~15ms |
| Memory | Native, compact | JS objects |
| Key Type | String | Any |
| Value Type | String | Any |
| Iteration | `keys()`, `values()`, `entries()` | Same |

> Use `DaggerTable` when:
> - Working with large datasets (10K+ entries)
> - Keys and values are strings (or serializable)
> - Performance is critical
>
> Use JavaScript `Map` when:
> - Need non-string keys
> - Need object values without serialization
> - Small datasets where overhead doesn't matter

---

## API Reference

### Constructor

| Method | Description |
|--------|-------------|
| `new DaggerTable()` | Create empty table |

### Basic Operations

| Method | Description |
|--------|-------------|
| `set(key, value)` | Set key-value, returns `this` |
| `get(key)` | Get value, returns `undefined` if not found |
| `has(key)` | Check if key exists |
| `delete(key)` | Delete key, returns `true` if existed |
| `clear()` | Remove all entries |

### Properties

| Property | Description |
|----------|-------------|
| `size` | Number of entries |

### Iteration

| Method | Description |
|--------|-------------|
| `keys()` | Get all keys as array |
| `values()` | Get all values as array |
| `entries()` | Generator of [key, value] pairs |
| `forEach(callback)` | Call function for each entry |
| `[Symbol.iterator]` | Enable for-of loops |

### Constants

| Constant | Description |
|----------|-------------|
| `PSL_THRESHOLD` | Max probe sequence before rehash |
| `LOAD_FACTOR_PERCENT` | Load factor percentage (70) |

### Utility

| Function | Description |
|----------|-------------|
| `version()` | Get module version |

---

## Next Steps

- [Hash Module](./hash.md) - Understanding NXH hashing used by Dagger
- [Weave Module](./weave.md) - String interning complements Dagger
- [INI Module](./ini.md) - Configuration parsing with key-value access
