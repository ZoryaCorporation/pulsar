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
import { createRequire } from 'module';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';
const __dirname = dirname(fileURLToPath(import.meta.url));
const require = createRequire(import.meta.url);
const native = require(join(__dirname, '..', 'native', 'pulsar_hash.node'));
/**
 * Module version
 */
export function version() {
    return native.version;
}
/**
 * Default seed (0)
 */
export const SEED_DEFAULT = native.SEED_DEFAULT;
/**
 * Alternate seed for second hash function
 */
export const SEED_ALT = native.SEED_ALT;
/**
 * Hash a buffer with 64-bit output
 */
export function nxh64(buffer, seed) {
    return native.nxh64(buffer, seed);
}
/**
 * Hash a buffer with 32-bit output
 */
export function nxh32(buffer, seed) {
    return native.nxh32(buffer, seed);
}
/**
 * Alternate 64-bit hash (for Cuckoo hashing)
 */
export function nxh64Alt(buffer, seed) {
    return native.nxh64Alt(buffer, seed);
}
/**
 * Hash a string directly (UTF-8)
 */
export function nxhString(str, seed) {
    return native.nxhString(str, seed);
}
/**
 * Hash a string with 32-bit output
 */
export function nxhString32(str, seed) {
    return native.nxhString32(str, seed);
}
/**
 * Hash a 64-bit integer
 */
export function nxhInt64(value) {
    return native.nxhInt64(value);
}
/**
 * Hash a 32-bit integer
 */
export function nxhInt32(value) {
    return native.nxhInt32(value);
}
/**
 * Combine two hashes (for composite keys)
 */
export function nxhCombine(h1, h2) {
    return native.nxhCombine(h1, h2);
}
export default {
    version,
    SEED_DEFAULT,
    SEED_ALT,
    nxh64,
    nxh32,
    nxh64Alt,
    nxhString,
    nxhString32,
    nxhInt64,
    nxhInt32,
    nxhCombine,
};
//# sourceMappingURL=hash.js.map