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
import { createRequire } from 'module';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';
const __dirname = dirname(fileURLToPath(import.meta.url));
const require = createRequire(import.meta.url);
const native = require(join(__dirname, '..', 'native', 'pulsar_ini.node'));
/**
 * Module version
 */
export function version() {
    return native.version;
}
/**
 * Create a new empty INI document
 */
export function create() {
    return native.create();
}
/**
 * Free an INI handle
 */
export function free(handle) {
    native.free(handle);
}
/**
 * Load INI from file into existing handle
 */
export function load(handle, path) {
    return native.load(handle, path);
}
/**
 * Load INI from string content into existing handle
 */
export function loadString(handle, content) {
    return native.loadString(handle, content);
}
/**
 * Get a string value (key format: "section.key")
 */
export function get(handle, key) {
    return native.get(handle, key);
}
/**
 * Get a string value with default (key format: "section.key")
 */
export function getDefault(handle, key, defaultValue) {
    return native.getDefault(handle, key, defaultValue);
}
/**
 * Get an integer value (key format: "section.key")
 */
export function getInt(handle, key) {
    return native.getInt(handle, key);
}
/**
 * Get a float value (key format: "section.key")
 */
export function getFloat(handle, key) {
    return native.getFloat(handle, key);
}
/**
 * Get a boolean value (key format: "section.key")
 */
export function getBool(handle, key) {
    return native.getBool(handle, key);
}
/**
 * Get an array value (split by delimiter, key format: "section.key")
 */
export function getArray(handle, key, delimiter = ',') {
    return native.getArray(handle, key, delimiter);
}
/**
 * Check if key exists (key format: "section.key")
 */
export function has(handle, key) {
    return native.has(handle, key);
}
/**
 * Set a value (key format: "section.key")
 */
export function set(handle, key, value) {
    native.set(handle, key, value);
}
/**
 * Serialize INI to string
 */
export function toString(handle) {
    return native.toString(handle);
}
/**
 * Save INI to file
 */
export function save(handle, path) {
    return native.save(handle, path);
}
/**
 * Get all section names
 */
export function sections(handle) {
    return native.sections(handle);
}
/**
 * Get INI stats
 */
export function stats(handle) {
    return native.stats(handle);
}
/**
 * Wrapper class for more ergonomic usage
 */
export class INIDocument {
    _handle;
    _freed = false;
    constructor(handle) {
        this._handle = handle;
    }
    /**
     * Create a new empty document
     */
    static create() {
        return new INIDocument(native.create());
    }
    /**
     * Load from file
     */
    static load(path) {
        const doc = new INIDocument(native.create());
        native.load(doc._handle, path);
        return doc;
    }
    /**
     * Parse from string
     */
    static parse(content) {
        const doc = new INIDocument(native.create());
        native.loadString(doc._handle, content);
        return doc;
    }
    checkFreed() {
        if (this._freed) {
            throw new Error('INIDocument has been freed');
        }
    }
    get(sectionOrKey, key) {
        this.checkFreed();
        const fullKey = key !== undefined ? `${sectionOrKey}.${key}` : sectionOrKey;
        return native.get(this._handle, fullKey);
    }
    getDefault(sectionOrKey, keyOrDefault, defaultValue) {
        this.checkFreed();
        if (defaultValue !== undefined) {
            return native.getDefault(this._handle, `${sectionOrKey}.${keyOrDefault}`, defaultValue);
        }
        return native.getDefault(this._handle, sectionOrKey, keyOrDefault);
    }
    getInt(sectionOrKey, key) {
        this.checkFreed();
        const fullKey = key !== undefined ? `${sectionOrKey}.${key}` : sectionOrKey;
        return native.getInt(this._handle, fullKey);
    }
    getFloat(sectionOrKey, key) {
        this.checkFreed();
        const fullKey = key !== undefined ? `${sectionOrKey}.${key}` : sectionOrKey;
        return native.getFloat(this._handle, fullKey);
    }
    getBool(sectionOrKey, key) {
        this.checkFreed();
        const fullKey = key !== undefined ? `${sectionOrKey}.${key}` : sectionOrKey;
        return native.getBool(this._handle, fullKey);
    }
    getArray(sectionOrKey, keyOrDelimiter, delimiter) {
        this.checkFreed();
        // If third arg exists, first two are section.key
        if (delimiter !== undefined) {
            return native.getArray(this._handle, `${sectionOrKey}.${keyOrDelimiter}`, delimiter);
        }
        // If second arg looks like a delimiter (short), use first as key
        if (keyOrDelimiter && keyOrDelimiter.length <= 2) {
            return native.getArray(this._handle, sectionOrKey, keyOrDelimiter);
        }
        // Otherwise second arg is key, default delimiter
        if (keyOrDelimiter) {
            return native.getArray(this._handle, `${sectionOrKey}.${keyOrDelimiter}`, ',');
        }
        return native.getArray(this._handle, sectionOrKey, ',');
    }
    has(sectionOrKey, key) {
        this.checkFreed();
        const fullKey = key !== undefined ? `${sectionOrKey}.${key}` : sectionOrKey;
        return native.has(this._handle, fullKey);
    }
    set(sectionOrKey, keyOrValue, value) {
        this.checkFreed();
        if (value !== undefined) {
            native.set(this._handle, `${sectionOrKey}.${keyOrValue}`, value);
        }
        else {
            native.set(this._handle, sectionOrKey, keyOrValue);
        }
        return this;
    }
    /**
     * Serialize to string
     */
    toString() {
        this.checkFreed();
        return native.toString(this._handle);
    }
    /**
     * Save to file
     */
    save(path) {
        this.checkFreed();
        return native.save(this._handle, path);
    }
    /**
     * Get all section names
     */
    sections() {
        this.checkFreed();
        return native.sections(this._handle);
    }
    /**
     * Get stats
     */
    stats() {
        this.checkFreed();
        return native.stats(this._handle);
    }
    /**
     * Free resources
     */
    free() {
        if (!this._freed) {
            native.free(this._handle);
            this._freed = true;
        }
    }
    /**
     * Symbol.dispose for 'using' keyword
     */
    [Symbol.dispose]() {
        this.free();
    }
}
export default {
    version,
    create,
    free,
    load,
    loadString,
    get,
    getDefault,
    getInt,
    getFloat,
    getBool,
    getArray,
    has,
    set,
    toString,
    save,
    sections,
    stats,
    INIDocument,
};
//# sourceMappingURL=ini.js.map