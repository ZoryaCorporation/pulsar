/**
 * @file weave.ts
 * @brief Pulsar Weave Module - String Primitives
 *
 * High-performance native string data structures:
 * - Weave: Mutable string buffer with efficient operations
 * - Cord: Rope-like immutable string with O(log n) concatenation
 * - Tablet: String interning table (deduplication)
 * - Arena: Bump allocator for temporary strings
 *
 * @author Anthony Taliento
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * @example
 * ```typescript
 * import { weave } from '@aspect/pulsar';
 *
 * // Mutable string builder
 * const w = weave.Weave.from('Hello');
 * w.append(', World!');
 * console.log(w.toString()); // "Hello, World!"
 *
 * // String interning
 * const tablet = weave.Tablet.create();
 * const id1 = tablet.intern('hello');
 * const id2 = tablet.intern('hello'); // Same ID
 *
 * // Rope for large text
 * const cord = weave.Cord.create();
 * cord.append('chunk1').append('chunk2');
 * ```
 */
import { createRequire } from 'module';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
const __dirname = dirname(fileURLToPath(import.meta.url));
const require = createRequire(import.meta.url);
/* Load native module */
const NATIVE_PATH = join(__dirname, '..', 'native', 'pulsar_weave.node');
const native = require(NATIVE_PATH);
/* ============================================================
 * Weave - Mutable String Buffer
 * ============================================================ */
/**
 * Weave - High-performance mutable string buffer
 *
 * Like StringBuilder but with native performance and additional
 * operations like find, replace, trim, split.
 */
export class Weave {
    _handle;
    constructor(handle) {
        this._handle = handle;
    }
    /**
     * Create empty Weave
     */
    static create() {
        return new Weave(native.weaveCreate());
    }
    /**
     * Create from string
     */
    static from(str) {
        return new Weave(native.weaveCreate(str));
    }
    /**
     * Append string
     */
    append(str) {
        native.weaveAppend(this._handle, str);
        return this;
    }
    /**
     * Prepend string
     */
    prepend(str) {
        native.weavePrepend(this._handle, str);
        return this;
    }
    /**
     * Get string content
     */
    toString() {
        return native.weaveToString(this._handle);
    }
    /**
     * String length
     */
    get length() {
        return native.weaveLength(this._handle);
    }
    /**
     * Buffer capacity
     */
    get capacity() {
        return native.weaveCapacity(this._handle);
    }
    /**
     * Reserve capacity
     */
    reserve(capacity) {
        native.weaveReserve(this._handle, capacity);
        return this;
    }
    /**
     * Find substring, returns -1 if not found
     */
    find(needle) {
        return native.weaveFind(this._handle, needle);
    }
    /**
     * Check if contains substring
     */
    contains(needle) {
        return native.weaveContains(this._handle, needle);
    }
    /**
     * Replace first occurrence
     */
    replace(old, replacement) {
        native.weaveReplace(this._handle, old, replacement);
        return this;
    }
    /**
     * Replace all occurrences
     */
    replaceAll(old, replacement) {
        native.weaveReplaceAll(this._handle, old, replacement);
        return this;
    }
    /**
     * Trim whitespace
     */
    trim() {
        native.weaveTrim(this._handle);
        return this;
    }
    /**
     * Split by delimiter
     */
    split(delimiter) {
        return native.weaveSplit(this._handle, delimiter);
    }
}
/* ============================================================
 * Cord - Rope-like Immutable String
 * ============================================================ */
/**
 * Cord - Rope data structure for efficient large string manipulation
 *
 * Provides O(log n) concatenation and substring operations.
 * Ideal for text editors or large document manipulation.
 */
export class Cord {
    _handle;
    constructor(handle) {
        this._handle = handle;
    }
    /**
     * Create empty Cord
     */
    static create() {
        return new Cord(native.cordCreate());
    }
    /**
     * Append string chunk
     */
    append(str) {
        native.cordAppend(this._handle, str);
        return this;
    }
    /**
     * Total length
     */
    get length() {
        return native.cordLength(this._handle);
    }
    /**
     * Number of chunks
     */
    get chunkCount() {
        return native.cordChunkCount(this._handle);
    }
    /**
     * Materialize to string
     */
    toString() {
        return native.cordToString(this._handle);
    }
    /**
     * Clear all chunks
     */
    clear() {
        native.cordClear(this._handle);
        return this;
    }
}
/* ============================================================
 * Tablet - String Interning Table
 * ============================================================ */
/**
 * Tablet - String interning for deduplication
 *
 * Stores unique strings once and returns IDs.
 * Perfect for symbol tables, identifiers, or repeated strings.
 */
export class Tablet {
    _handle;
    constructor(handle) {
        this._handle = handle;
    }
    /**
     * Create new tablet
     */
    static create() {
        return new Tablet(native.tabletCreate());
    }
    /**
     * Intern a string, returns ID
     */
    intern(str) {
        return native.tabletIntern(this._handle, str);
    }
    /**
     * Get string by ID
     */
    get(id) {
        return native.tabletGet(this._handle, id);
    }
    /**
     * Number of interned strings
     */
    get count() {
        return native.tabletCount(this._handle);
    }
    /**
     * Memory usage
     */
    get memory() {
        return native.tabletMemory(this._handle);
    }
}
/**
 * Arena - Bump allocator for temporary strings
 *
 * Fast allocation with batch deallocation.
 * Great for parsing or temporary computation.
 */
export class Arena {
    _handle;
    constructor(handle) {
        this._handle = handle;
    }
    /**
     * Create arena with initial capacity
     */
    static create(capacity) {
        return new Arena(native.arenaCreate(capacity ?? 4096));
    }
    /**
     * Allocate bytes, returns offset
     */
    alloc(size) {
        return native.arenaAlloc(this._handle, size);
    }
    /**
     * Allocate and store string, returns offset
     */
    allocString(str) {
        return native.arenaAllocString(this._handle, str);
    }
    /**
     * Get string at offset
     */
    getString(offset) {
        return native.arenaGetString(this._handle, offset);
    }
    /**
     * Begin temporary scope (for nested allocations)
     */
    tempBegin() {
        return native.arenaTempBegin(this._handle);
    }
    /**
     * End temporary scope, deallocating back to mark
     */
    tempEnd(mark) {
        native.arenaTempEnd(this._handle, mark);
    }
    /**
     * Reset arena (deallocate everything)
     */
    reset() {
        native.arenaReset(this._handle);
        return this;
    }
    /**
     * Get stats
     */
    stats() {
        return native.arenaStats(this._handle);
    }
}
/* ============================================================
 * Module Exports
 * ============================================================ */
/**
 * Get module version
 */
export function version() {
    if (typeof native.version === 'function') {
        return native.version();
    }
    if (typeof native.weaveVersion === 'function') {
        return native.weaveVersion();
    }
    return '1.0.0';
}
export default {
    version,
    Weave,
    Cord,
    Tablet,
    Arena,
};
//# sourceMappingURL=weave.js.map