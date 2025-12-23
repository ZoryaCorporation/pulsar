/**
 * @file pcm.ts
 * @brief PCM - Primitive CPU Math bindings
 *
 * Hardware-accelerated bit manipulation and integer operations.
 * Uses compiler intrinsics (POPCNT, LZCNT, TZCNT, etc.) for maximum performance.
 *
 * @example
 * ```typescript
 * import { pcm } from '@aspect/pulsar';
 *
 * // Count bits
 * const bits = pcm.popcount32(0xFF);  // 8
 *
 * // Leading/trailing zeros
 * const leading = pcm.clz32(0x80000000);  // 0
 * const trailing = pcm.ctz32(0x80000000); // 31
 *
 * // Power of 2 utilities
 * const next = pcm.nextPowerOf2_32(100); // 128
 * const aligned = pcm.alignUp(100, 64);   // 128
 * ```
 */
/**
 * Module version
 */
export declare function version(): string;
/**
 * Count set bits in 32-bit integer
 */
export declare function popcount32(x: number): number;
/**
 * Count set bits in 64-bit integer
 */
export declare function popcount64(x: bigint): number;
/**
 * Count leading zeros in 32-bit integer
 */
export declare function clz32(x: number): number;
/**
 * Count leading zeros in 64-bit integer
 */
export declare function clz64(x: bigint): number;
/**
 * Count trailing zeros in 32-bit integer
 */
export declare function ctz32(x: number): number;
/**
 * Count trailing zeros in 64-bit integer
 */
export declare function ctz64(x: bigint): number;
/**
 * Rotate left 32-bit
 */
export declare function rotl32(x: number, r: number): number;
/**
 * Rotate right 32-bit
 */
export declare function rotr32(x: number, r: number): number;
/**
 * Rotate left 64-bit
 */
export declare function rotl64(x: bigint, r: number): bigint;
/**
 * Rotate right 64-bit
 */
export declare function rotr64(x: bigint, r: number): bigint;
/**
 * Byte swap 16-bit (endian conversion)
 */
export declare function bswap16(x: number): number;
/**
 * Byte swap 32-bit (endian conversion)
 */
export declare function bswap32(x: number): number;
/**
 * Byte swap 64-bit (endian conversion)
 */
export declare function bswap64(x: bigint): bigint;
/**
 * Check if value is a power of 2
 */
export declare function isPowerOf2(x: number): boolean;
/**
 * Round up to next power of 2 (32-bit)
 */
export declare function nextPowerOf2_32(x: number): number;
/**
 * Round up to next power of 2 (64-bit)
 */
export declare function nextPowerOf2_64(x: bigint): bigint;
/**
 * Align value up to boundary
 */
export declare function alignUp(x: number, alignment: number): number;
/**
 * Align value down to boundary
 */
export declare function alignDown(x: number, alignment: number): number;
/**
 * Check if value is aligned
 */
export declare function isAligned(x: number, alignment: number): boolean;
/**
 * Integer log2 (32-bit)
 */
export declare function log2_32(x: number): number;
/**
 * Integer log2 (64-bit)
 */
export declare function log2_64(x: bigint): number;
declare const _default: {
    version: typeof version;
    popcount32: typeof popcount32;
    popcount64: typeof popcount64;
    clz32: typeof clz32;
    clz64: typeof clz64;
    ctz32: typeof ctz32;
    ctz64: typeof ctz64;
    rotl32: typeof rotl32;
    rotr32: typeof rotr32;
    rotl64: typeof rotl64;
    rotr64: typeof rotr64;
    bswap16: typeof bswap16;
    bswap32: typeof bswap32;
    bswap64: typeof bswap64;
    isPowerOf2: typeof isPowerOf2;
    nextPowerOf2_32: typeof nextPowerOf2_32;
    nextPowerOf2_64: typeof nextPowerOf2_64;
    alignUp: typeof alignUp;
    alignDown: typeof alignDown;
    isAligned: typeof isAligned;
    log2_32: typeof log2_32;
    log2_64: typeof log2_64;
};
export default _default;
//# sourceMappingURL=pcm.d.ts.map