/**
 * @file hash.ts
 * @brief NXH - Nexus Hash bindings
 *
 * High-performance non-cryptographic hash functions.
 * NXH provides 64-bit and 32-bit hashing with excellent avalanche properties.
 *
 * @example
 * ```typescript
 * import { hash } from '@aspect/pulsar';
 *
 * // Hash a buffer
 * const h = hash.nxh64(Buffer.from('hello'));
 *
 * // Hash a string directly
 * const strHash = hash.nxhString('hello world');
 *
 * // Combine hashes
 * const combined = hash.nxhCombine(hash1, hash2);
 * ```
 */
/**
 * Module version
 */
export declare function version(): string;
/**
 * Default seed (0)
 */
export declare const SEED_DEFAULT: bigint;
/**
 * Alternate seed for second hash function
 */
export declare const SEED_ALT: bigint;
/**
 * Hash a buffer with 64-bit output
 */
export declare function nxh64(buffer: Buffer, seed?: bigint): bigint;
/**
 * Hash a buffer with 32-bit output
 */
export declare function nxh32(buffer: Buffer, seed?: bigint): number;
/**
 * Alternate 64-bit hash (for Cuckoo hashing)
 */
export declare function nxh64Alt(buffer: Buffer, seed?: bigint): bigint;
/**
 * Hash a string directly (UTF-8)
 */
export declare function nxhString(str: string, seed?: bigint): bigint;
/**
 * Hash a string with 32-bit output
 */
export declare function nxhString32(str: string, seed?: bigint): number;
/**
 * Hash a 64-bit integer
 */
export declare function nxhInt64(value: bigint): bigint;
/**
 * Hash a 32-bit integer
 */
export declare function nxhInt32(value: number): bigint;
/**
 * Combine two hashes (for composite keys)
 */
export declare function nxhCombine(h1: bigint, h2: bigint): bigint;
declare const _default: {
    version: typeof version;
    SEED_DEFAULT: bigint;
    SEED_ALT: bigint;
    nxh64: typeof nxh64;
    nxh32: typeof nxh32;
    nxh64Alt: typeof nxh64Alt;
    nxhString: typeof nxhString;
    nxhString32: typeof nxhString32;
    nxhInt64: typeof nxhInt64;
    nxhInt32: typeof nxhInt32;
    nxhCombine: typeof nxhCombine;
};
export default _default;
//# sourceMappingURL=hash.d.ts.map