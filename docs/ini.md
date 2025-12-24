# INI Module Guide

> **Fast INI Configuration Parser**  
> Zero-copy parsing with type coercion and dot notation

The INI module provides a native INI file parser with support for sections, type coercion, and arrays. It's perfect for application configuration, game settings, or any hierarchical key-value data. The Zorya Ini parser is bound to TypeScript/JavaScript via Pulsar's native bindings, delivering high performance and low memory overhead. The native ini parser is very comprehensive and additional features have been added to this parser to support structured data formats. We will cover the methods that are currently bound via the API and how to use them effectively.

The Zorya Ini parser also works independently of pulsar and will be released included with the ZORYA-C SDK for C/C++ applications. We also will be releasing bindings for other languages in the future.

---

## Overview

| Feature | Description |
|---------|-------------|
| **Parsing** | Zero-copy native parsing |
| **Sections** | Standard `[section]` support |
| **Key Access** | Dot notation: `section.key` |
| **Types** | String, int, float, bool, array |
| **Comments** | `;` and `#` line comments |

---

## Quick Start

```typescript
import { ini } from '@zoryacorporation/pulsar';

// Create and load configuration
const config = ini.INIDocument.load('/path/to/config.ini');

// Read values with type coercion
const host = config.get('server.host');
const port = config.getInt('server.port');
const debug = config.getBool('app.debug');

// Set values
config.set('server.port', '8080');

// Save changes
config.save('/path/to/config.ini');

// Clean up
config.free();
```

---

## INI File Format

### Basic Structure

```ini
; This is a comment
# This is also a comment

[section]
key = value
another_key = another value

[database]
host = localhost
port = 5432
name = myapp

[features]
enabled = true
max_items = 100
rate = 0.75
tags = web|api|v2
```

### Key Format

Keys are accessed using dot notation: `section.key`

```typescript
const host = config.get('database.host');     // 'localhost'
const port = config.getInt('database.port');  // 5432
```

---

## Creating Documents

### Empty Document

```typescript
import { ini } from '@zoryacorporation/pulsar';

const doc = ini.INIDocument.create();
```

### Load from File

```typescript
const config = ini.INIDocument.load('/etc/myapp/config.ini');
```

### Parse from String

```typescript
const iniContent = `
[server]
host = localhost
port = 8080

[app]
debug = true
`;

const config = ini.INIDocument.parse(iniContent);
```

---

## Reading Values

### Strings

```typescript
import { ini } from '@zoryacorporation/pulsar';

const config = ini.INIDocument.load('config.ini');

// Get string value
const host = config.get('server.host');

// Returns null if not found
const missing = config.get('nonexistent.key'); // null

// Get with default
const timeout = config.getDefault('server.timeout', '30');
```

### Alternative Syntax

Both formats work:

```typescript
// Dot notation (recommended)
const host = config.get('database.host');

// Separate section and key
const host = config.get('database', 'host');
```

### Integers

```typescript
const port = config.getInt('server.port');     // number
const workers = config.getInt('app.workers');  // number

// Alternative syntax
const maxConn = config.getInt('database', 'max_connections');
```

### Floats

```typescript
const rate = config.getFloat('limits.rate');
const threshold = config.getFloat('ml.threshold');
```

### Booleans

```typescript
const debug = config.getBool('app.debug');
const enabled = config.getBool('feature.enabled');

// Truthy values: true, 1, yes, on
// Falsy values: false, 0, no, off
```

### Arrays

Arrays use a delimiter (default: comma):

```ini
[settings]
tags = web,api,v2
flags = enabled|disabled|pending
```

```typescript
// Default delimiter (comma)
const tags = config.getArray('settings.tags');
// ['web', 'api', 'v2']

// Custom delimiter
const flags = config.getArray('settings.flags', '|');
// ['enabled', 'disabled', 'pending']

// Alternative syntax
const items = config.getArray('section', 'key', ',');
```

---

## Writing Values

### Set Values

```typescript
import { ini } from '@zoryacorporation/pulsar';

const config = ini.INIDocument.create(); // create new ini document

// Set values (creates section if needed)
config.set('server.host', 'localhost');
config.set('server.port', '8080');
config.set('app.debug', 'true');

// Chaining supported
config
  .set('database.host', 'localhost')
  .set('database.port', '5432')
  .set('database.name', 'myapp');
```

### Serialize to String

```typescript
const iniString = config.toString();
console.log(iniString);
// [server]
// host = localhost
// port = 8080
// ...
```

### Save to File

```typescript
config.save('/path/to/config.ini');
```

---

## Checking Keys

### Has Key

```typescript
if (config.has('server.host')) {
  // Key exists
}

// Alternative syntax
if (config.has('server', 'host')) {
  // Key exists
}
```

### List Sections

```typescript
const sections = config.sections();
console.log(sections); // ['server', 'database', 'app']
```

---

## Statistics

```typescript
// Example pattern where we save config stats to console.log({}) we cannot use just '()' because that would be interpreted as a code block. we use '{}' to indicate an object literal. It's a common pattern in JavaScript/TypeScript to log structured data. For const stats = config.stats(); this means we receive various object literals in a code block which saves to the console log.
const stats = config.stats();
console.log({
  sections: stats.sections,
  keys: stats.keys,
  memoryUsed: stats.memoryUsed
});
```

---

## Memory Management

INI documents use native memory. Always clean up when done:

### Manual Cleanup

```typescript
const config = ini.INIDocument.load('config.ini');
// ... use config ...
config.free();
```

### Using `using` (TypeScript 5.2+)

```typescript
{
  using config = ini.INIDocument.load('config.ini');
  // ... use config ...
} // Automatically freed
```

### Try-Finally Pattern

```typescript
const config = ini.INIDocument.load('config.ini');
try {
  // ... use config ...
} finally {
  config.free();
}
```

---

## Low-Level API

For more control, use the function-based API:

```typescript
import { ini } from '@zoryacorporation/pulsar';

// Create handle
const handle = ini.create();

// Load file
ini.load(handle, '/path/to/config.ini');

// Or load string
ini.loadString(handle, '[section]\nkey=value');

// Read values
const value = ini.get(handle, 'section.key');
const num = ini.getInt(handle, 'section.number');
const flag = ini.getBool(handle, 'section.flag');
const items = ini.getArray(handle, 'section.list', ',');

// Write values
ini.set(handle, 'section.key', 'new value');

// Save
ini.save(handle, '/path/to/output.ini');

// Cleanup
ini.free(handle);
```

---

## Common Use Cases

### Application Configuration

```typescript
// Example: app config loader -- here we showcase importing both ini and fileops modules to work together in finding, loading and managing configuration files. This process depends on file existence checks and ini parsing. 
import { ini, fileops } from '@zoryacorporation/pulsar';

class AppConfig {
  private config: ReturnType<typeof ini.INIDocument.create>; // INI handle
  private path: string; // results path returned as a string value
  
  constructor(path: string) {
    this.path = path; // Store config file path
    
    if (fileops.exists(path)) { // Check if file exists
      this.config = ini.INIDocument.load(path); // locks in the config file if it exists.
    } else { // we use 'else' to handle the case where the file does not exist.
      this.config = ini.INIDocument.create(); // automatically creates a new INI document in memory.
      this.setDefaults(); // populate with default settings
    }
  }
  // default settings that will be used if no config file is found:
  // this is a simple example of how to set default values programmatically. However, in a real-world scenario you must verify that these defaults align with your application's requirements. This method ensures that the application has a valid configuration to operate with, even in the absence of a pre-existing config file but always verify before deploying.
  
  private setDefaults(): void {
    this.config
      .set('server.host', '0.0.0.0')
      .set('server.port', '3000')
      .set('database.host', 'localhost')
      .set('database.port', '5432')
      .set('app.debug', 'false')
      .set('app.log_level', 'info');
  }
  // We use 'get' accessors to retrieve configuration values in a type-safe manner. This pattern encapsulates the retrieval logic and provides a clear interface for accessing configuration settings throughout the application. Each accessor corresponds to a specific configuration key, ensuring that the correct type is returned and reducing the risk of runtime errors due to type mismatches. However once again always validate that these accessors align with your actual configuration schema.
  get serverHost(): string {
    return this.config.get('server.host') ?? '0.0.0.0';
  }
  
  get serverPort(): number {
    return this.config.getInt('server.port');
  }
  
  get databaseUrl(): string {
    const host = this.config.get('database.host');
    const port = this.config.getInt('database.port');
    const name = this.config.get('database.name');
    return `postgres://${host}:${port}/${name}`;
  }
  
  get isDebug(): boolean {
    return this.config.getBool('app.debug');
  }
  
  save(): void {
    this.config.save(this.path);
  }
  
  close(): void {
    this.config.free();
  }
}

// Usage where config file path is known. 'const config = new AppConfig('/etc/myapp/config.ini');' creates an instance of the AppConfig class, loading the configuration from the specified INI file. If the file does not exist, it initializes a new configuration with default settings. The subsequent console.log statements demonstrate how to access various configuration values using the defined accessors. Finally, 'config.close();' ensures that any allocated resources are properly released when the configuration is no longer needed. We ALWAYS CLOSE CONFIGS TO AVOID MEMORY AND DATA BREACHES. If you forget to close configs you will leak memory and potentially leave sensitive data in memory longer than necessary thus exposing sensitive APIs and secrets. Config files often contain sensitive information such as database credentials, API keys, and other secrets. Failing to free these resources can lead to unintended data exposure and security vulnerabilities.


const config = new AppConfig('/etc/myapp/config.ini');
console.log(`Server: ${config.serverHost}:${config.serverPort}`); // Server address and port. this may be sensitive in some contexts.
console.log(`Database: ${config.databaseUrl}`); // Database connection string containing potentially sensitive info
config.close();
// In secure settings we would make sure console.log statements do not expose sensitive information and are removed or sanitized in production environments by using methods like: `if (!config.isDebug) { /* avoid logging sensitive info */ }` or serilizing logs to secure locations only. Where permissioned access is enforced. However in standard non sensitive contexts this is acceptable if the information is not sensitive. Remember we as developers are always responsible for protecting sensitive data. 

```

### Game Settings

```typescript
import { ini } from '@zoryacorporation/pulsar';

interface GameSettings {
  graphics: {
    resolution: string;
    fullscreen: boolean;
    vsync: boolean;
    quality: string;
  };
  audio: {
    masterVolume: number;
    musicVolume: number;
    sfxVolume: number;
  };
  controls: {
    sensitivity: number;
    invertY: boolean;
  };
}

class GameConfig {
  private config: ReturnType<typeof ini.INIDocument.create>;
  private path: string;
  
  constructor(path: string) {
    this.path = path;
    this.config = ini.INIDocument.load(path);
  }
  
  get settings(): GameSettings {
    return {
      graphics: {
        resolution: this.config.getDefault('graphics.resolution', '1920x1080'),
        fullscreen: this.config.getBool('graphics.fullscreen'),
        vsync: this.config.getBool('graphics.vsync'),
        quality: this.config.getDefault('graphics.quality', 'high')
      },
      audio: {
        masterVolume: this.config.getFloat('audio.master_volume'),
        musicVolume: this.config.getFloat('audio.music_volume'),
        sfxVolume: this.config.getFloat('audio.sfx_volume')
      },
      controls: {
        sensitivity: this.config.getFloat('controls.sensitivity'),
        invertY: this.config.getBool('controls.invert_y')
      }
    };
  }
  
  setResolution(width: number, height: number): void {
    this.config.set('graphics.resolution', `${width}x${height}`);
  }
  
  setVolume(type: 'master' | 'music' | 'sfx', value: number): void {
    const clamped = Math.max(0, Math.min(1, value));
    this.config.set(`audio.${type}_volume`, clamped.toString());
  }
  
  save(): void {
    this.config.save(this.path);
  }
  
  close(): void {
    this.config.free();
  }
}

// Example settings.ini:
// [graphics]
// resolution = 1920x1080
// fullscreen = true
// vsync = true
// quality = ultra
//
// [audio]
// master_volume = 0.8
// music_volume = 0.6
// sfx_volume = 1.0
//
// [controls]
// sensitivity = 0.5
// invert_y = false
```

### Environment-Based Configuration

```typescript
import { ini, fileops } from '@zoryacorporation/pulsar';

function loadConfig(env: string = 'development'): ReturnType<typeof ini.INIDocument.create> {
  const basePath = '/etc/myapp';
  
  // Load base config
  const config = ini.INIDocument.load(`${basePath}/config.ini`);
  
  // Override with environment-specific config
  const envPath = `${basePath}/config.${env}.ini`;
  if (fileops.exists(envPath)) {
    const envConfig = ini.INIDocument.load(envPath);
    
    // Copy all values from env config
    for (const section of envConfig.sections()) {
      // Would need to implement section key iteration
      // This is a conceptual demo; actual implementation may vary
      const keys = envConfig.stats().keys; // Placeholder for actual key retrieval
      for (let i = 0; i < keys; i++) {
        const key = ''; // Placeholder for actual key retrieval
        const value = envConfig.get(`${section}.${key}`);
        config.set(`${section}.${key}`, value);
      }
    }
    
    envConfig.free();
  }
  
  return config;
}

// usage where environment is set via NODE_ENV
const config = loadConfig(process.env.NODE_ENV ?? 'development');
```

### Multi-Language Strings

```ini
; strings_en.ini
[ui]
welcome = Welcome to our app!
goodbye = Goodbye!
error_not_found = Item not found

[messages]
save_success = Your changes have been saved.
delete_confirm = Are you sure you want to delete this?

; strings_es.ini
[ui]
welcome = ¡Bienvenido a nuestra aplicación!
goodbye = ¡Adiós!
error_not_found = Elemento no encontrado
```

```typescript
import { ini } from '@zoryacorporation/pulsar';

class I18n {
  private strings: Map<string, ReturnType<typeof ini.INIDocument.create>> = new Map();
  private currentLang = 'en';
  
  loadLanguage(lang: string, path: string): void {
    const config = ini.INIDocument.load(path);
    this.strings.set(lang, config);
  }
  
  setLanguage(lang: string): void {
    if (this.strings.has(lang)) {
      this.currentLang = lang;
    }
  }
  
  t(key: string): string {
    const config = this.strings.get(this.currentLang);
    return config?.get(key) ?? key;
  }
  
  close(): void {
    for (const config of this.strings.values()) {
      config.free();
    }
    this.strings.clear();
  }
}

// Usage
const i18n = new I18n();
i18n.loadLanguage('en', 'strings_en.ini');
i18n.loadLanguage('es', 'strings_es.ini');

console.log(i18n.t('ui.welcome')); // "Welcome to our app!"

i18n.setLanguage('es');
console.log(i18n.t('ui.welcome')); // "¡Bienvenido a nuestra aplicación!"
```

### Feature Flags

```ini
[features]
new_ui = true
dark_mode = true
beta_features = false
max_upload_mb = 100
allowed_types = jpg|png|gif|webp

[rollout]
new_ui_percentage = 50
beta_users = alice|bob|charlie
```

```typescript
import { ini, hash } from '@zoryacorporation/pulsar';

class FeatureFlags {
  private config: ReturnType<typeof ini.INIDocument.create>;
  
  constructor(path: string) {
    this.config = ini.INIDocument.load(path);
  }
  
  isEnabled(feature: string): boolean {
    return this.config.getBool(`features.${feature}`);
  }
  
  getLimit(feature: string): number {
    return this.config.getInt(`features.${feature}`);
  }
  
  getAllowedTypes(feature: string): string[] {
    return this.config.getArray(`features.${feature}`, '|');
  }
  
  // Percentage-based rollout
  isEnabledForUser(feature: string, userId: string): boolean {
    if (!this.isEnabled(feature)) return false;
    
    const percentage = this.config.getInt(`rollout.${feature}_percentage`);
    if (percentage >= 100) return true;
    if (percentage <= 0) return false;
    
    // Deterministic based on user ID
    const userHash = hash.nxhString(userId);
    const bucket = Number(userHash % 100n);
    return bucket < percentage;
  }
  
  // User whitelist
  isBetaUser(userId: string): boolean {
    const betaUsers = this.config.getArray('rollout.beta_users', '|');
    return betaUsers.includes(userId);
  }
  
  close(): void {
    this.config.free();
  }
}
```

---

## Best Practices

### 1. Group Related Settings

```ini
; Flat structure with mixed keys. It is best practice to group related settings into sections for better organization and readability. This approach makes it easier to locate and manage configuration options, especially as the number of settings grows. Additionally, grouping related settings can help prevent naming collisions and improve maintainability.

; NOT RECOMMENDED:
server_host = localhost
server_port = 8080
database_host = localhost
database_port = 5432

; Organized sections for related settings. This structure enhances clarity and makes it easier to manage configurations. It also allows for better scalability as new settings can be added to their respective sections without cluttering the global namespace.

; RECOMMENDED:
[server]
host = localhost
port = 8080

[database]
host = localhost
port = 5432
```

### 2. Use Descriptive Keys

```ini
; Non-descriptive & cryptic: this makes it hard to understand by others and yourself later. ini files by nature tend to be very simple, so clarity is key. However the mastering simplicity is often the hardest part. Dennis Ritche often advocated that simplicity is a great virtue but it requires hard work to achieve it, education to appreciate it, and to make matters worse: complexity sells better.

; We believe in simplicity and clarity. The following example is a where simplicity was sacrificed for brevity. While brevity can be a virtue in some contexts, in configuration files clarity should take precedence. 
[app]
mxc = 100
tmout = 30

; CORRECT: Clear concise and to the point. This pattern makes it easy to understand the purpose of each setting at a glance, reducing cognitive load and potential errors during configuration. Another benefit of this approach is that it facilitates collaboration among team members, as everyone can quickly grasp the configuration without needing extensive documentation or explanations. Another unintended but welcome side effect in this is AI Assistants benefit from clear human readable code. AI tools can more easily parse and understand well-structured configuration files, leading to better suggestions and automation capabilities.
[app]
max_connections = 100
timeout_seconds = 30
```

### 3. Document with Comments

```ini
; Application Configuration
; Last updated: 2025-01-15

[server]
; Bind address (0.0.0.0 for all interfaces)
host = 0.0.0.0

; Port to listen on
port = 8080

[database]

; Connection pool settings
max_connections = 20  ; Maximum concurrent connections
idle_timeout = 300    ; Seconds before closing idle connection
```

### 4. Always Free Resources

```typescript
// INCORRECT: Memory leak
function getConfig() {
  const config = ini.INIDocument.load('config.ini');
  return config.get('server.port');
  // config never freed thus this pattern would leak memory
  
  // This simple mistake can accumulate and cause issues in long-running operations. config files are often hundreds of bytes to several kilobytes in size in some contexts. If this function is called repeatedly, the memory usage will grow indefinitely.
}

// CORRECT: Proper cleanup
function getConfig() {
  const config = ini.INIDocument.load('config.ini');
  try {
    return config.get('server.port');
  } finally {
    config.free();
  }
}

// Or keep reference and free later if needed.
class App {
  private config: ReturnType<typeof ini.INIDocument.create>;
  
  init() {
    this.config = ini.INIDocument.load('config.ini');
  }
  
  shutdown() {
    this.config.free();
  } // This ensures memory is released when the application is done using the config. This pattern is an alternative to loading and freeing repeatedly.
}
```

---

## API Reference

### INIDocument Class

| Method | Description |
|--------|-------------|
| `INIDocument.create()` | Create empty document |
| `INIDocument.load(path)` | Load from file |
| `INIDocument.parse(content)` | Parse from string |
| `get(key)` | Get string value |
| `get(section, key)` | Get string (section, key) |
| `getDefault(key, default)` | Get with default |
| `getInt(key)` | Get as integer |
| `getFloat(key)` | Get as float |
| `getBool(key)` | Get as boolean |
| `getArray(key, delim?)` | Get as array |
| `has(key)` | Check if exists |
| `set(key, value)` | Set value |
| `toString()` | Serialize to string |
| `save(path)` | Save to file |
| `sections()` | List section names |
| `stats()` | Get statistics |
| `free()` | Release memory |

### Function API

| Function | Description |
|----------|-------------|
| `create()` | Create handle |
| `free(handle)` | Release handle |
| `load(handle, path)` | Load file |
| `loadString(handle, content)` | Parse string |
| `get(handle, key)` | Get string |
| `getDefault(handle, key, default)` | Get with default |
| `getInt(handle, key)` | Get integer |
| `getFloat(handle, key)` | Get float |
| `getBool(handle, key)` | Get boolean |
| `getArray(handle, key, delim?)` | Get array |
| `has(handle, key)` | Check exists |
| `set(handle, key, value)` | Set value |
| `toString(handle)` | Serialize |
| `save(handle, path)` | Save file |
| `sections(handle)` | List sections |
| `stats(handle)` | Get stats |
| `version()` | Module version |

### IniStats Interface

```typescript
interface IniStats {
  sections: number;    // Number of sections
  keys: number;        // Total number of keys
  memoryUsed: number;  // Memory in bytes
}
```

---

## Next Steps

- [FileOps Module](./fileops.md) - File operations for loading configs
- [Dagger Module](./dagger.md) - Alternative key-value storage
- [Getting Started Guide](./getting-started.md) - Full application tutorial
