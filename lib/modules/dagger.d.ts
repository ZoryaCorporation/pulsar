/**
 * @file dagger.ts
 * @brief Dagger - O(1) Hash Table bindings
 *
 * High-performance hash table with Robin Hood hashing and probe sequence
 * length tracking. Provides constant-time operations with excellent
 * worst-case behavior.
 *
 * @example
 * ```typescript
 * import { dagger } from '@aspect/pulsar';
 *
 * // Create a table
 * const table = new dagger.DaggerTable();
 *
 * // Set/get values
 * table.set('key', 'value');
 * const val = table.get('key');
 *
 * // Check existence
 * if (table.has('key')) { ... }
 *
 * // Iterate
 * for (const k of table.keys()) { ... }
 * for (const v of table.values()) { ... }
 * ```
 */
/**
 * Module version
 */
export declare function version(): string;
/**
 * Probe sequence length threshold for Robin Hood rehashing
 */
export declare const PSL_THRESHOLD: number;
/**
 * Load factor percentage before resize (default 70%)
 */
export declare const LOAD_FACTOR_PERCENT: number;
/**
 * DaggerTable - High-performance hash table
 *
 * Uses Robin Hood hashing with probe sequence length tracking.
 * Provides O(1) average-case and excellent worst-case performance.
 */
export declare class DaggerTable {
    private _native;
    constructor();
    /**
     * Set a key-value pair
     */
    set(key: string, value: string): this;
    /**
     * Get a value by key
     */
    get(key: string): string | undefined;
    /**
     * Check if key exists
     */
    has(key: string): boolean;
    /**
     * Delete a key
     */
    delete(key: string): boolean;
    /**
     * Number of entries
     */
    get size(): number;
    /**
     * Get all keys
     */
    keys(): string[];
    /**
     * Get all values
     */
    values(): string[];
    /**
     * Clear all entries
     */
    clear(): void;
    /**
     * Iterate over entries
     */
    entries(): Generator<[string, string]>;
    /**
     * forEach iteration
     */
    forEach(callback: (value: string, key: string, table: DaggerTable) => void): void;
    /**
     * Symbol.iterator for for-of loops
     */
    [Symbol.iterator](): Generator<[string, string]>;
}
declare const _default: {
    version: typeof version;
    PSL_THRESHOLD: number;
    LOAD_FACTOR_PERCENT: number;
    DaggerTable: typeof DaggerTable;
};
export default _default;
//# sourceMappingURL=dagger.d.ts.map