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

import { createRequire } from 'module';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const require = createRequire(import.meta.url);

interface NativePCM {
  version: string;
  popcount32(x: number): number;
  popcount64(x: bigint): number;
  clz32(x: number): number;
  clz64(x: bigint): number;
  ctz32(x: number): number;
  ctz64(x: bigint): number;
  rotl32(x: number, r: number): number;
  rotr32(x: number, r: number): number;
  rotl64(x: bigint, r: number): bigint;
  rotr64(x: bigint, r: number): bigint;
  bswap16(x: number): number;
  bswap32(x: number): number;
  bswap64(x: bigint): bigint;
  isPowerOf2(x: number): boolean;
  nextPowerOf2_32(x: number): number;
  nextPowerOf2_64(x: bigint): bigint;
  alignUp(x: number, alignment: number): number;
  alignDown(x: number, alignment: number): number;
  isAligned(x: number, alignment: number): boolean;
  log2_32(x: number): number;
  log2_64(x: bigint): number;
}

const native: NativePCM = require(join(__dirname, '..', 'native', 'pulsar_pcm.node'));

/**
 * Module version
 */
export function version(): string {
  return native.version;
}

// Population count (Hamming weight)

/**
 * Count set bits in 32-bit integer
 */
export function popcount32(x: number): number {
  return native.popcount32(x);
}

/**
 * Count set bits in 64-bit integer
 */
export function popcount64(x: bigint): number {
  return native.popcount64(x);
}

// Count leading zeros

/**
 * Count leading zeros in 32-bit integer
 */
export function clz32(x: number): number {
  return native.clz32(x);
}

/**
 * Count leading zeros in 64-bit integer
 */
export function clz64(x: bigint): number {
  return native.clz64(x);
}

// Count trailing zeros

/**
 * Count trailing zeros in 32-bit integer
 */
export function ctz32(x: number): number {
  return native.ctz32(x);
}

/**
 * Count trailing zeros in 64-bit integer
 */
export function ctz64(x: bigint): number {
  return native.ctz64(x);
}

// Rotations

/**
 * Rotate left 32-bit
 */
export function rotl32(x: number, r: number): number {
  return native.rotl32(x, r);
}

/**
 * Rotate right 32-bit
 */
export function rotr32(x: number, r: number): number {
  return native.rotr32(x, r);
}

/**
 * Rotate left 64-bit
 */
export function rotl64(x: bigint, r: number): bigint {
  return native.rotl64(x, r);
}

/**
 * Rotate right 64-bit
 */
export function rotr64(x: bigint, r: number): bigint {
  return native.rotr64(x, r);
}

// Byte swapping

/**
 * Byte swap 16-bit (endian conversion)
 */
export function bswap16(x: number): number {
  return native.bswap16(x);
}

/**
 * Byte swap 32-bit (endian conversion)
 */
export function bswap32(x: number): number {
  return native.bswap32(x);
}

/**
 * Byte swap 64-bit (endian conversion)
 */
export function bswap64(x: bigint): bigint {
  return native.bswap64(x);
}

// Power of 2 utilities

/**
 * Check if value is a power of 2
 */
export function isPowerOf2(x: number): boolean {
  return native.isPowerOf2(x);
}

/**
 * Round up to next power of 2 (32-bit)
 */
export function nextPowerOf2_32(x: number): number {
  return native.nextPowerOf2_32(x);
}

/**
 * Round up to next power of 2 (64-bit)
 */
export function nextPowerOf2_64(x: bigint): bigint {
  return native.nextPowerOf2_64(x);
}

// Alignment utilities

/**
 * Align value up to boundary
 */
export function alignUp(x: number, alignment: number): number {
  return native.alignUp(x, alignment);
}

/**
 * Align value down to boundary
 */
export function alignDown(x: number, alignment: number): number {
  return native.alignDown(x, alignment);
}

/**
 * Check if value is aligned
 */
export function isAligned(x: number, alignment: number): boolean {
  return native.isAligned(x, alignment);
}

// Logarithm

/**
 * Integer log2 (32-bit)
 */
export function log2_32(x: number): number {
  return native.log2_32(x);
}

/**
 * Integer log2 (64-bit)
 */
export function log2_64(x: bigint): number {
  return native.log2_64(x);
}

export default {
  version,
  popcount32,
  popcount64,
  clz32,
  clz64,
  ctz32,
  ctz64,
  rotl32,
  rotr32,
  rotl64,
  rotr64,
  bswap16,
  bswap32,
  bswap64,
  isPowerOf2,
  nextPowerOf2_32,
  nextPowerOf2_64,
  alignUp,
  alignDown,
  isAligned,
  log2_32,
  log2_64,
};
