/**
 * @file index.ts
 * @brief Pulsar Framework - Native Acceleration for Node.js & Electron
 *
 * @author Anthony Taliento
 * @copyright Copyright (c) 2025 Zorya Corporation
 * @license Apache-2.0
 * @version 1.0.0
 *
 * Pulsar provides native-speed implementations for common operations:
 *
 * - **weave**: String primitives (Weave, Cord, Tablet, Arena)
 * - **compress**: Data compression (Zstandard, LZ4)
 * - **fileops**: File I/O, watching, glob patterns
 * - **hash**: NXH high-performance non-cryptographic hashing
 * - **dagger**: O(1) hash table with Robin Hood hashing
 * - **pcm**: Hardware-accelerated bit manipulation
 * - **ini**: Zero-copy INI file parser
 *
 * @example
 * ```typescript
 * import { weave, compress, fileops, hash, dagger, pcm, ini } from '@aspect/pulsar';
 *
 * // String primitives
 * const w = weave.Weave.from('hello');
 *
 * // Compression
 * const compressed = compress.zstd.compress(buffer);
 *
 * // File operations
 * const content = fileops.readFile('/path/to/file');
 *
 * // Hashing
 * const h = hash.nxh64(Buffer.from('hello'));
 *
 * // Hash table
 * const table = new dagger.DaggerTable();
 * table.set('key', 'value');
 *
 * // Bit operations
 * const bits = pcm.popcount32(0xFF);
 *
 * // INI parsing
 * const doc = ini.INIDocument.load('config.ini');
 * ```
 */
/* Re-export all modules */
export * as weave from './modules/weave.js';
export * as compress from './modules/compress.js';
export * as fileops from './modules/fileops.js';
export * as hash from './modules/hash.js';
export * as dagger from './modules/dagger.js';
export * as pcm from './modules/pcm.js';
export * as ini from './modules/ini.js';
/* Import for namespace re-export */
import weaveModule from './modules/weave.js';
import compressModule from './modules/compress.js';
import fileopsModule from './modules/fileops.js';
import hashModule from './modules/hash.js';
import daggerModule from './modules/dagger.js';
import pcmModule from './modules/pcm.js';
import iniModule from './modules/ini.js';
/**
 * Pulsar version
 */
export const version = '1.0.0';
/**
 * Get all module versions
 */
export function versions() {
    return {
        pulsar: version,
        weave: weaveModule.version(),
        compress: compressModule.version(),
        fileops: fileopsModule.version(),
        hash: hashModule.version(),
        dagger: daggerModule.version(),
        pcm: pcmModule.version(),
        ini: iniModule.version(),
    };
}
/**
 * Default export with all modules
 */
export default {
    version,
    versions,
    weave: weaveModule,
    compress: compressModule,
    fileops: fileopsModule,
    hash: hashModule,
    dagger: daggerModule,
    pcm: pcmModule,
    ini: iniModule,
};
//# sourceMappingURL=index.js.map