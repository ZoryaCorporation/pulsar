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
/**
 * Weave - High-performance mutable string buffer
 *
 * Like StringBuilder but with native performance and additional
 * operations like find, replace, trim, split.
 */
export declare class Weave {
    private _handle;
    private constructor();
    /**
     * Create empty Weave
     */
    static create(): Weave;
    /**
     * Create from string
     */
    static from(str: string): Weave;
    /**
     * Append string
     */
    append(str: string): this;
    /**
     * Prepend string
     */
    prepend(str: string): this;
    /**
     * Get string content
     */
    toString(): string;
    /**
     * String length
     */
    get length(): number;
    /**
     * Buffer capacity
     */
    get capacity(): number;
    /**
     * Reserve capacity
     */
    reserve(capacity: number): this;
    /**
     * Find substring, returns -1 if not found
     */
    find(needle: string): number;
    /**
     * Check if contains substring
     */
    contains(needle: string): boolean;
    /**
     * Replace first occurrence
     */
    replace(old: string, replacement: string): this;
    /**
     * Replace all occurrences
     */
    replaceAll(old: string, replacement: string): this;
    /**
     * Trim whitespace
     */
    trim(): this;
    /**
     * Split by delimiter
     */
    split(delimiter: string): string[];
}
/**
 * Cord - Rope data structure for efficient large string manipulation
 *
 * Provides O(log n) concatenation and substring operations.
 * Ideal for text editors or large document manipulation.
 */
export declare class Cord {
    private _handle;
    private constructor();
    /**
     * Create empty Cord
     */
    static create(): Cord;
    /**
     * Append string chunk
     */
    append(str: string): this;
    /**
     * Total length
     */
    get length(): number;
    /**
     * Number of chunks
     */
    get chunkCount(): number;
    /**
     * Materialize to string
     */
    toString(): string;
    /**
     * Clear all chunks
     */
    clear(): this;
}
/**
 * Tablet - String interning for deduplication
 *
 * Stores unique strings once and returns IDs.
 * Perfect for symbol tables, identifiers, or repeated strings.
 */
export declare class Tablet {
    private _handle;
    private constructor();
    /**
     * Create new tablet
     */
    static create(): Tablet;
    /**
     * Intern a string, returns ID
     */
    intern(str: string): bigint;
    /**
     * Get string by ID
     */
    get(id: bigint): string | null;
    /**
     * Number of interned strings
     */
    get count(): number;
    /**
     * Memory usage
     */
    get memory(): number;
}
interface ArenaStats {
    totalAllocated: number;
    totalCapacity: number;
    blockCount: number;
}
/**
 * Arena - Bump allocator for temporary strings
 *
 * Fast allocation with batch deallocation.
 * Great for parsing or temporary computation.
 */
export declare class Arena {
    private _handle;
    private constructor();
    /**
     * Create arena with initial capacity
     */
    static create(capacity?: number): Arena;
    /**
     * Allocate bytes, returns offset
     */
    alloc(size: number): number;
    /**
     * Allocate and store string, returns offset
     */
    allocString(str: string): number;
    /**
     * Get string at offset
     */
    getString(offset: number): string;
    /**
     * Begin temporary scope (for nested allocations)
     */
    tempBegin(): number;
    /**
     * End temporary scope, deallocating back to mark
     */
    tempEnd(mark: number): void;
    /**
     * Reset arena (deallocate everything)
     */
    reset(): this;
    /**
     * Get stats
     */
    stats(): ArenaStats;
}
/**
 * Get module version
 */
export declare function version(): string;
declare const _default: {
    version: typeof version;
    Weave: typeof Weave;
    Cord: typeof Cord;
    Tablet: typeof Tablet;
    Arena: typeof Arena;
};
export default _default;
//# sourceMappingURL=weave.d.ts.map