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
export * as weave from './modules/weave.js';
export * as compress from './modules/compress.js';
export * as fileops from './modules/fileops.js';
export * as hash from './modules/hash.js';
export * as dagger from './modules/dagger.js';
export * as pcm from './modules/pcm.js';
export * as ini from './modules/ini.js';
/**
 * Pulsar version
 */
export declare const version = "1.0.0";
/**
 * Get all module versions
 */
export declare function versions(): Record<string, string>;
/**
 * Default export with all modules
 */
declare const _default: {
    version: string;
    versions: typeof versions;
    weave: {
        version: typeof import("./modules/weave.js").version;
        Weave: typeof import("./modules/weave.js").Weave;
        Cord: typeof import("./modules/weave.js").Cord;
        Tablet: typeof import("./modules/weave.js").Tablet;
        Arena: typeof import("./modules/weave.js").Arena;
    };
    compress: {
        zstd: typeof import("./modules/compress.js").zstd;
        lz4: typeof import("./modules/compress.js").lz4;
        compress: typeof import("./modules/compress.js").compress;
        decompress: typeof import("./modules/compress.js").decompress;
        version: typeof import("./modules/compress.js").version;
    };
    fileops: {
        readFile: typeof import("./modules/fileops.js").readFile;
        readText: typeof import("./modules/fileops.js").readText;
        writeFile: typeof import("./modules/fileops.js").writeFile;
        appendFile: typeof import("./modules/fileops.js").appendFile;
        copyFile: typeof import("./modules/fileops.js").copyFile;
        moveFile: typeof import("./modules/fileops.js").moveFile;
        remove: typeof import("./modules/fileops.js").remove;
        removeRecursive: typeof import("./modules/fileops.js").removeRecursive;
        stat: typeof import("./modules/fileops.js").stat;
        lstat: typeof import("./modules/fileops.js").lstat;
        exists: typeof import("./modules/fileops.js").exists;
        isFile: typeof import("./modules/fileops.js").isFile;
        isDirectory: typeof import("./modules/fileops.js").isDirectory;
        isSymlink: typeof import("./modules/fileops.js").isSymlink;
        fileSize: typeof import("./modules/fileops.js").fileSize;
        mkdir: typeof import("./modules/fileops.js").mkdir;
        mkdirp: typeof import("./modules/fileops.js").mkdirp;
        readdir: typeof import("./modules/fileops.js").readdir;
        readdirWithTypes: typeof import("./modules/fileops.js").readdirWithTypes;
        basename: typeof import("./modules/fileops.js").basename;
        dirname: typeof import("./modules/fileops.js").dirname;
        extname: typeof import("./modules/fileops.js").extname;
        joinPath: typeof import("./modules/fileops.js").joinPath;
        normalize: typeof import("./modules/fileops.js").normalize;
        resolve: typeof import("./modules/fileops.js").resolve;
        isAbsolute: typeof import("./modules/fileops.js").isAbsolute;
        symlink: typeof import("./modules/fileops.js").symlink;
        readlink: typeof import("./modules/fileops.js").readlink;
        tmpdir: typeof import("./modules/fileops.js").tmpdir;
        homedir: typeof import("./modules/fileops.js").homedir;
        cwd: typeof import("./modules/fileops.js").cwd;
        chdir: typeof import("./modules/fileops.js").chdir;
        chmod: typeof import("./modules/fileops.js").chmod;
        chown: typeof import("./modules/fileops.js").chown;
        glob: typeof import("./modules/fileops.js").glob;
        Watcher: typeof import("./modules/fileops.js").Watcher;
        watch: typeof import("./modules/fileops.js").watch;
        version: typeof import("./modules/fileops.js").version;
        FileType: typeof import("./modules/fileops.js").FileType;
    };
    hash: {
        version: typeof import("./modules/hash.js").version;
        SEED_DEFAULT: bigint;
        SEED_ALT: bigint;
        nxh64: typeof import("./modules/hash.js").nxh64;
        nxh32: typeof import("./modules/hash.js").nxh32;
        nxh64Alt: typeof import("./modules/hash.js").nxh64Alt;
        nxhString: typeof import("./modules/hash.js").nxhString;
        nxhString32: typeof import("./modules/hash.js").nxhString32;
        nxhInt64: typeof import("./modules/hash.js").nxhInt64;
        nxhInt32: typeof import("./modules/hash.js").nxhInt32;
        nxhCombine: typeof import("./modules/hash.js").nxhCombine;
    };
    dagger: {
        version: typeof import("./modules/dagger.js").version;
        PSL_THRESHOLD: number;
        LOAD_FACTOR_PERCENT: number;
        DaggerTable: typeof import("./modules/dagger.js").DaggerTable;
    };
    pcm: {
        version: typeof import("./modules/pcm.js").version;
        popcount32: typeof import("./modules/pcm.js").popcount32;
        popcount64: typeof import("./modules/pcm.js").popcount64;
        clz32: typeof import("./modules/pcm.js").clz32;
        clz64: typeof import("./modules/pcm.js").clz64;
        ctz32: typeof import("./modules/pcm.js").ctz32;
        ctz64: typeof import("./modules/pcm.js").ctz64;
        rotl32: typeof import("./modules/pcm.js").rotl32;
        rotr32: typeof import("./modules/pcm.js").rotr32;
        rotl64: typeof import("./modules/pcm.js").rotl64;
        rotr64: typeof import("./modules/pcm.js").rotr64;
        bswap16: typeof import("./modules/pcm.js").bswap16;
        bswap32: typeof import("./modules/pcm.js").bswap32;
        bswap64: typeof import("./modules/pcm.js").bswap64;
        isPowerOf2: typeof import("./modules/pcm.js").isPowerOf2;
        nextPowerOf2_32: typeof import("./modules/pcm.js").nextPowerOf2_32;
        nextPowerOf2_64: typeof import("./modules/pcm.js").nextPowerOf2_64;
        alignUp: typeof import("./modules/pcm.js").alignUp;
        alignDown: typeof import("./modules/pcm.js").alignDown;
        isAligned: typeof import("./modules/pcm.js").isAligned;
        log2_32: typeof import("./modules/pcm.js").log2_32;
        log2_64: typeof import("./modules/pcm.js").log2_64;
    };
    ini: {
        version: typeof import("./modules/ini.js").version;
        create: typeof import("./modules/ini.js").create;
        free: typeof import("./modules/ini.js").free;
        load: typeof import("./modules/ini.js").load;
        loadString: typeof import("./modules/ini.js").loadString;
        get: typeof import("./modules/ini.js").get;
        getDefault: typeof import("./modules/ini.js").getDefault;
        getInt: typeof import("./modules/ini.js").getInt;
        getFloat: typeof import("./modules/ini.js").getFloat;
        getBool: typeof import("./modules/ini.js").getBool;
        getArray: typeof import("./modules/ini.js").getArray;
        has: typeof import("./modules/ini.js").has;
        set: typeof import("./modules/ini.js").set;
        toString: typeof import("./modules/ini.js").toString;
        save: typeof import("./modules/ini.js").save;
        sections: typeof import("./modules/ini.js").sections;
        stats: typeof import("./modules/ini.js").stats;
        INIDocument: typeof import("./modules/ini.js").INIDocument;
    };
};
export default _default;
//# sourceMappingURL=index.d.ts.map