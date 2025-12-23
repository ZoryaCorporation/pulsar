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

// INI handle is an opaque pointer (external)
type IniHandle = object;

interface IniStats {
  sections: number;
  keys: number;
  memoryUsed: number;
}

interface NativeINI {
  version: string;
  create(): IniHandle;
  free(handle: IniHandle): void;
  load(handle: IniHandle, path: string): boolean;
  loadString(handle: IniHandle, content: string): boolean;
  get(handle: IniHandle, key: string): string | null;
  getDefault(handle: IniHandle, key: string, defaultValue: string): string;
  getInt(handle: IniHandle, key: string): number;
  getFloat(handle: IniHandle, key: string): number;
  getBool(handle: IniHandle, key: string): boolean;
  getArray(handle: IniHandle, key: string, delimiter?: string): string[];
  has(handle: IniHandle, key: string): boolean;
  set(handle: IniHandle, key: string, value: string): void;
  toString(handle: IniHandle): string;
  save(handle: IniHandle, path: string): boolean;
  sections(handle: IniHandle): string[];
  stats(handle: IniHandle): IniStats;
}

const native: NativeINI = require(join(__dirname, '..', 'native', 'pulsar_ini.node'));

/**
 * Module version
 */
export function version(): string {
  return native.version;
}

/**
 * Create a new empty INI document
 */
export function create(): IniHandle {
  return native.create();
}

/**
 * Free an INI handle
 */
export function free(handle: IniHandle): void {
  native.free(handle);
}

/**
 * Load INI from file into existing handle
 */
export function load(handle: IniHandle, path: string): boolean {
  return native.load(handle, path);
}

/**
 * Load INI from string content into existing handle
 */
export function loadString(handle: IniHandle, content: string): boolean {
  return native.loadString(handle, content);
}

/**
 * Get a string value (key format: "section.key")
 */
export function get(handle: IniHandle, key: string): string | null {
  return native.get(handle, key);
}

/**
 * Get a string value with default (key format: "section.key")
 */
export function getDefault(handle: IniHandle, key: string, defaultValue: string): string {
  return native.getDefault(handle, key, defaultValue);
}

/**
 * Get an integer value (key format: "section.key")
 */
export function getInt(handle: IniHandle, key: string): number {
  return native.getInt(handle, key);
}

/**
 * Get a float value (key format: "section.key")
 */
export function getFloat(handle: IniHandle, key: string): number {
  return native.getFloat(handle, key);
}

/**
 * Get a boolean value (key format: "section.key")
 */
export function getBool(handle: IniHandle, key: string): boolean {
  return native.getBool(handle, key);
}

/**
 * Get an array value (split by delimiter, key format: "section.key")
 */
export function getArray(handle: IniHandle, key: string, delimiter: string = ','): string[] {
  return native.getArray(handle, key, delimiter);
}

/**
 * Check if key exists (key format: "section.key")
 */
export function has(handle: IniHandle, key: string): boolean {
  return native.has(handle, key);
}

/**
 * Set a value (key format: "section.key")
 */
export function set(handle: IniHandle, key: string, value: string): void {
  native.set(handle, key, value);
}

/**
 * Serialize INI to string
 */
export function toString(handle: IniHandle): string {
  return native.toString(handle);
}

/**
 * Save INI to file
 */
export function save(handle: IniHandle, path: string): boolean {
  return native.save(handle, path);
}

/**
 * Get all section names
 */
export function sections(handle: IniHandle): string[] {
  return native.sections(handle);
}

/**
 * Get INI stats
 */
export function stats(handle: IniHandle): IniStats {
  return native.stats(handle);
}

/**
 * Wrapper class for more ergonomic usage
 */
export class INIDocument {
  private _handle: IniHandle;
  private _freed: boolean = false;

  private constructor(handle: IniHandle) {
    this._handle = handle;
  }

  /**
   * Create a new empty document
   */
  static create(): INIDocument {
    return new INIDocument(native.create());
  }

  /**
   * Load from file
   */
  static load(path: string): INIDocument {
    const doc = new INIDocument(native.create());
    native.load(doc._handle, path);
    return doc;
  }

  /**
   * Parse from string
   */
  static parse(content: string): INIDocument {
    const doc = new INIDocument(native.create());
    native.loadString(doc._handle, content);
    return doc;
  }

  private checkFreed(): void {
    if (this._freed) {
      throw new Error('INIDocument has been freed');
    }
  }

  /**
   * Get a string value (key format: "section.key")
   */
  get(key: string): string | null;
  /**
   * Get a string value (key format: section, key separately)
   */
  get(section: string, key: string): string | null;
  get(sectionOrKey: string, key?: string): string | null {
    this.checkFreed();
    const fullKey = key !== undefined ? `${sectionOrKey}.${key}` : sectionOrKey;
    return native.get(this._handle, fullKey);
  }

  /**
   * Get a string value with default
   */
  getDefault(key: string, defaultValue: string): string;
  getDefault(section: string, key: string, defaultValue: string): string;
  getDefault(sectionOrKey: string, keyOrDefault: string, defaultValue?: string): string {
    this.checkFreed();
    if (defaultValue !== undefined) {
      return native.getDefault(this._handle, `${sectionOrKey}.${keyOrDefault}`, defaultValue);
    }
    return native.getDefault(this._handle, sectionOrKey, keyOrDefault);
  }

  /**
   * Get an integer value
   */
  getInt(key: string): number;
  getInt(section: string, key: string): number;
  getInt(sectionOrKey: string, key?: string): number {
    this.checkFreed();
    const fullKey = key !== undefined ? `${sectionOrKey}.${key}` : sectionOrKey;
    return native.getInt(this._handle, fullKey);
  }

  /**
   * Get a float value
   */
  getFloat(key: string): number;
  getFloat(section: string, key: string): number;
  getFloat(sectionOrKey: string, key?: string): number {
    this.checkFreed();
    const fullKey = key !== undefined ? `${sectionOrKey}.${key}` : sectionOrKey;
    return native.getFloat(this._handle, fullKey);
  }

  /**
   * Get a boolean value
   */
  getBool(key: string): boolean;
  getBool(section: string, key: string): boolean;
  getBool(sectionOrKey: string, key?: string): boolean {
    this.checkFreed();
    const fullKey = key !== undefined ? `${sectionOrKey}.${key}` : sectionOrKey;
    return native.getBool(this._handle, fullKey);
  }

  /**
   * Get an array value
   */
  getArray(key: string, delimiter?: string): string[];
  getArray(section: string, key: string, delimiter?: string): string[];
  getArray(sectionOrKey: string, keyOrDelimiter?: string, delimiter?: string): string[] {
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

  /**
   * Check if key exists
   */
  has(key: string): boolean;
  has(section: string, key: string): boolean;
  has(sectionOrKey: string, key?: string): boolean {
    this.checkFreed();
    const fullKey = key !== undefined ? `${sectionOrKey}.${key}` : sectionOrKey;
    return native.has(this._handle, fullKey);
  }

  /**
   * Set a value
   */
  set(key: string, value: string): this;
  set(section: string, key: string, value: string): this;
  set(sectionOrKey: string, keyOrValue: string, value?: string): this {
    this.checkFreed();
    if (value !== undefined) {
      native.set(this._handle, `${sectionOrKey}.${keyOrValue}`, value);
    } else {
      native.set(this._handle, sectionOrKey, keyOrValue);
    }
    return this;
  }

  /**
   * Serialize to string
   */
  toString(): string {
    this.checkFreed();
    return native.toString(this._handle);
  }

  /**
   * Save to file
   */
  save(path: string): boolean {
    this.checkFreed();
    return native.save(this._handle, path);
  }

  /**
   * Get all section names
   */
  sections(): string[] {
    this.checkFreed();
    return native.sections(this._handle);
  }

  /**
   * Get stats
   */
  stats(): IniStats {
    this.checkFreed();
    return native.stats(this._handle);
  }

  /**
   * Free resources
   */
  free(): void {
    if (!this._freed) {
      native.free(this._handle);
      this._freed = true;
    }
  }

  /**
   * Symbol.dispose for 'using' keyword
   */
  [Symbol.dispose](): void {
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
