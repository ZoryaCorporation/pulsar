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
import { createRequire } from 'module';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';
const __dirname = dirname(fileURLToPath(import.meta.url));
const require = createRequire(import.meta.url);
/* Load native module */
const NATIVE_PATH = join(__dirname, '..', 'native', 'pulsar_compress.node');
const native = require(NATIVE_PATH);
/* ============================================================
 * Zstandard (zstd)
 * ============================================================ */
export var zstd;
(function (zstd) {
    /**
     * Compress data with zstd
     * @param data Input buffer
     * @param level Compression level (1-22, default: 3)
     */
    function compress(data, level = 3) {
        return native.zstdCompress(data, level);
    }
    zstd.compress = compress;
    /**
     * Decompress zstd data
     */
    function decompress(data) {
        return native.zstdDecompress(data);
    }
    zstd.decompress = decompress;
    /**
     * Get maximum compressed size
     */
    function compressBound(size) {
        return native.zstdCompressBound(size);
    }
    zstd.compressBound = compressBound;
})(zstd || (zstd = {}));
/* ============================================================
 * LZ4
 * ============================================================ */
export var lz4;
(function (lz4) {
    /**
     * Compress data with LZ4
     */
    function compress(data) {
        return native.lz4Compress(data);
    }
    lz4.compress = compress;
    /**
     * Compress with high compression mode
     * @param level Compression level (1-12, default: 9)
     */
    function compressHC(data, level = 9) {
        return native.lz4CompressHC(data, level);
    }
    lz4.compressHC = compressHC;
    /**
     * Decompress LZ4 data
     */
    function decompress(data, originalSize) {
        return native.lz4Decompress(data, originalSize);
    }
    lz4.decompress = decompress;
    /**
     * Get maximum compressed size
     */
    function compressBound(size) {
        return native.lz4CompressBound(size);
    }
    lz4.compressBound = compressBound;
})(lz4 || (lz4 = {}));
/**
 * Compress data using specified algorithm
 */
export function compress(data, options = {}) {
    const { algorithm = 'zstd', level } = options;
    switch (algorithm) {
        case 'zstd':
            return zstd.compress(data, level ?? 3);
        case 'lz4':
            return lz4.compress(data);
        case 'lz4hc':
            return lz4.compressHC(data, level ?? 9);
        default:
            throw new Error(`Unknown algorithm: ${algorithm}`);
    }
}
/**
 * Decompress data (auto-detects format)
 */
export function decompress(data, algorithm = 'zstd') {
    switch (algorithm) {
        case 'zstd':
            return zstd.decompress(data);
        case 'lz4':
        case 'lz4hc':
            return lz4.decompress(data);
        default:
            throw new Error(`Unknown algorithm: ${algorithm}`);
    }
}
/**
 * Get version
 */
export function version() {
    return native.version();
}
export default { zstd, lz4, compress, decompress, version };
//# sourceMappingURL=compress.js.map