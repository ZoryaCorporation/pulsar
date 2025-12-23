/**
 * @file compress.ts
 * @brief Pulsar Compress Module - High-Performance Compression
 *
 * Native compression using:
 * - Zstandard (zstd): Best compression ratio
 * - LZ4: Fastest compression
 *
 * @author Anthony Taliento
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 *
 * @example
 * ```typescript
 * import { compress } from '@aspect/pulsar';
 *
 * // Compress with zstd (default)
 * const compressed = compress.zstd.compress(data);
 * const original = compress.zstd.decompress(compressed);
 *
 * // Fast compression with LZ4
 * const fast = compress.lz4.compress(data);
 * ```
 */
export declare namespace zstd {
    /**
     * Compress data with zstd
     * @param data Input buffer
     * @param level Compression level (1-22, default: 3)
     */
    function compress(data: Buffer, level?: number): Buffer;
    /**
     * Decompress zstd data
     */
    function decompress(data: Buffer): Buffer;
    /**
     * Get maximum compressed size
     */
    function compressBound(size: number): number;
}
export declare namespace lz4 {
    /**
     * Compress data with LZ4
     */
    function compress(data: Buffer): Buffer;
    /**
     * Compress with high compression mode
     * @param level Compression level (1-12, default: 9)
     */
    function compressHC(data: Buffer, level?: number): Buffer;
    /**
     * Decompress LZ4 data
     */
    function decompress(data: Buffer, originalSize?: number): Buffer;
    /**
     * Get maximum compressed size
     */
    function compressBound(size: number): number;
}
export type CompressionAlgorithm = 'zstd' | 'lz4' | 'lz4hc';
export interface CompressOptions {
    algorithm?: CompressionAlgorithm;
    level?: number;
}
/**
 * Compress data using specified algorithm
 */
export declare function compress(data: Buffer, options?: CompressOptions): Buffer;
/**
 * Decompress data (auto-detects format)
 */
export declare function decompress(data: Buffer, algorithm?: CompressionAlgorithm): Buffer;
/**
 * Get version
 */
export declare function version(): string;
declare const _default: {
    zstd: typeof zstd;
    lz4: typeof lz4;
    compress: typeof compress;
    decompress: typeof decompress;
    version: typeof version;
};
export default _default;
//# sourceMappingURL=compress.d.ts.map