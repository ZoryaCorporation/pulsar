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

import { createRequire } from 'module';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const require = createRequire(import.meta.url);

interface NativeDagger {
  version: string;
  PSL_THRESHOLD: number;
  LOAD_FACTOR_PERCENT: number;
  DaggerTable: new () => NativeDaggerTable;
}

interface NativeDaggerTable {
  set(key: string, value: string): void;
  get(key: string): string | undefined;
  has(key: string): boolean;
  delete(key: string): boolean;
  size: number;
  keys(): string[];
  values(): string[];
  clear(): void;
}

const native: NativeDagger = require(join(__dirname, '..', 'native', 'pulsar_dagger.node'));

/**
 * Module version
 */
export function version(): string {
  return native.version;
}

/**
 * Probe sequence length threshold for Robin Hood rehashing
 */
export const PSL_THRESHOLD = native.PSL_THRESHOLD;

/**
 * Load factor percentage before resize (default 70%)
 */
export const LOAD_FACTOR_PERCENT = native.LOAD_FACTOR_PERCENT;

/**
 * DaggerTable - High-performance hash table
 *
 * Uses Robin Hood hashing with probe sequence length tracking.
 * Provides O(1) average-case and excellent worst-case performance.
 */
export class DaggerTable {
  private _native: NativeDaggerTable;

  constructor() {
    this._native = new native.DaggerTable();
  }

  /**
   * Set a key-value pair
   */
  set(key: string, value: string): this {
    this._native.set(key, value);
    return this;
  }

  /**
   * Get a value by key
   */
  get(key: string): string | undefined {
    return this._native.get(key);
  }

  /**
   * Check if key exists
   */
  has(key: string): boolean {
    return this._native.has(key);
  }

  /**
   * Delete a key
   */
  delete(key: string): boolean {
    return this._native.delete(key);
  }

  /**
   * Number of entries
   */
  get size(): number {
    return this._native.size;
  }

  /**
   * Get all keys
   */
  keys(): string[] {
    return this._native.keys();
  }

  /**
   * Get all values
   */
  values(): string[] {
    return this._native.values();
  }

  /**
   * Clear all entries
   */
  clear(): void {
    this._native.clear();
  }

  /**
   * Iterate over entries
   */
  *entries(): Generator<[string, string]> {
    const keys = this.keys();
    const values = this.values();
    for (let i = 0; i < keys.length; i++) {
      yield [keys[i], values[i]];
    }
  }

  /**
   * forEach iteration
   */
  forEach(callback: (value: string, key: string, table: DaggerTable) => void): void {
    const keys = this.keys();
    const values = this.values();
    for (let i = 0; i < keys.length; i++) {
      callback(values[i], keys[i], this);
    }
  }

  /**
   * Symbol.iterator for for-of loops
   */
  [Symbol.iterator](): Generator<[string, string]> {
    return this.entries();
  }
}

export default {
  version,
  PSL_THRESHOLD,
  LOAD_FACTOR_PERCENT,
  DaggerTable,
};
