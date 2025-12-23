/**
 * @file ini.ts
 * @brief INI Parser bindings
 *
 * Zero-copy INI file parser with section and key-value support.
 * Handles comments, multi-line values, and type coercion.
 *
 * Keys use dot notation: "section.key"
 *
 * @example
 * ```typescript
 * import { ini } from '@aspect/pulsar';
 *
 * // Load from file
 * const handle = ini.load('/path/to/config.ini');
 *
 * // Get values with type coercion (dot notation: section.key)
 * const port = ini.getInt(handle, 'server.port');
 * const debug = ini.getBool(handle, 'server.debug');
 *
 * // Set and save
 * ini.set(handle, 'server.port', '8080');
 * ini.save(handle, '/path/to/config.ini');
 *
 * // Clean up
 * ini.free(handle);
 * ```
 */
type IniHandle = object;
interface IniStats {
    sections: number;
    keys: number;
    memoryUsed: number;
}
/**
 * Module version
 */
export declare function version(): string;
/**
 * Create a new empty INI document
 */
export declare function create(): IniHandle;
/**
 * Free an INI handle
 */
export declare function free(handle: IniHandle): void;
/**
 * Load INI from file into existing handle
 */
export declare function load(handle: IniHandle, path: string): boolean;
/**
 * Load INI from string content into existing handle
 */
export declare function loadString(handle: IniHandle, content: string): boolean;
/**
 * Get a string value (key format: "section.key")
 */
export declare function get(handle: IniHandle, key: string): string | null;
/**
 * Get a string value with default (key format: "section.key")
 */
export declare function getDefault(handle: IniHandle, key: string, defaultValue: string): string;
/**
 * Get an integer value (key format: "section.key")
 */
export declare function getInt(handle: IniHandle, key: string): number;
/**
 * Get a float value (key format: "section.key")
 */
export declare function getFloat(handle: IniHandle, key: string): number;
/**
 * Get a boolean value (key format: "section.key")
 */
export declare function getBool(handle: IniHandle, key: string): boolean;
/**
 * Get an array value (split by delimiter, key format: "section.key")
 */
export declare function getArray(handle: IniHandle, key: string, delimiter?: string): string[];
/**
 * Check if key exists (key format: "section.key")
 */
export declare function has(handle: IniHandle, key: string): boolean;
/**
 * Set a value (key format: "section.key")
 */
export declare function set(handle: IniHandle, key: string, value: string): void;
/**
 * Serialize INI to string
 */
export declare function toString(handle: IniHandle): string;
/**
 * Save INI to file
 */
export declare function save(handle: IniHandle, path: string): boolean;
/**
 * Get all section names
 */
export declare function sections(handle: IniHandle): string[];
/**
 * Get INI stats
 */
export declare function stats(handle: IniHandle): IniStats;
/**
 * Wrapper class for more ergonomic usage
 */
export declare class INIDocument {
    private _handle;
    private _freed;
    private constructor();
    /**
     * Create a new empty document
     */
    static create(): INIDocument;
    /**
     * Load from file
     */
    static load(path: string): INIDocument;
    /**
     * Parse from string
     */
    static parse(content: string): INIDocument;
    private checkFreed;
    /**
     * Get a string value (key format: "section.key")
     */
    get(key: string): string | null;
    /**
     * Get a string value (key format: section, key separately)
     */
    get(section: string, key: string): string | null;
    /**
     * Get a string value with default
     */
    getDefault(key: string, defaultValue: string): string;
    getDefault(section: string, key: string, defaultValue: string): string;
    /**
     * Get an integer value
     */
    getInt(key: string): number;
    getInt(section: string, key: string): number;
    /**
     * Get a float value
     */
    getFloat(key: string): number;
    getFloat(section: string, key: string): number;
    /**
     * Get a boolean value
     */
    getBool(key: string): boolean;
    getBool(section: string, key: string): boolean;
    /**
     * Get an array value
     */
    getArray(key: string, delimiter?: string): string[];
    getArray(section: string, key: string, delimiter?: string): string[];
    /**
     * Check if key exists
     */
    has(key: string): boolean;
    has(section: string, key: string): boolean;
    /**
     * Set a value
     */
    set(key: string, value: string): this;
    set(section: string, key: string, value: string): this;
    /**
     * Serialize to string
     */
    toString(): string;
    /**
     * Save to file
     */
    save(path: string): boolean;
    /**
     * Get all section names
     */
    sections(): string[];
    /**
     * Get stats
     */
    stats(): IniStats;
    /**
     * Free resources
     */
    free(): void;
    /**
     * Symbol.dispose for 'using' keyword
     */
    [Symbol.dispose](): void;
}
declare const _default: {
    version: typeof version;
    create: typeof create;
    free: typeof free;
    load: typeof load;
    loadString: typeof loadString;
    get: typeof get;
    getDefault: typeof getDefault;
    getInt: typeof getInt;
    getFloat: typeof getFloat;
    getBool: typeof getBool;
    getArray: typeof getArray;
    has: typeof has;
    set: typeof set;
    toString: typeof toString;
    save: typeof save;
    sections: typeof sections;
    stats: typeof stats;
    INIDocument: typeof INIDocument;
};
export default _default;
//# sourceMappingURL=ini.d.ts.map