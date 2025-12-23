# PULSAR FRAMEWORK SPECIFICATION

**Version**: 0.1.0 (Pre-Alpha Specification)  
**Author**: Anthony Taliento / ZORYA Corporation  
**Date**: December 21, 2025  
**Status**: SPECIFICATION DRAFT

---

## EXECUTIVE SUMMARY

**Pulsar** is an augmentation framework for Electron applications. It enhances without replacing, adds without taking away — like TypeScript to JavaScript.

### Core Philosophy

```
PULSAR DOES NOT BREAK ANYTHING.
PULSAR ONLY ADDS CAPABILITIES.
ADOPT AT YOUR LEISURE.
```

### What Pulsar Is NOT

- ❌ NOT a fork of Electron
- ❌ NOT a replacement for npm/yarn
- ❌ NOT a mandatory migration
- ❌ NOT dependent on ZORYA-INI (optional enhancement)
- ❌ NOT dependent on Ordinal (optional enhancement)

### What Pulsar IS

- ✅ A performance augmentation layer for Electron
- ✅ A native acceleration bridge (ZORYA C SDK via NAPI)
- ✅ An optional configuration simplification system
- ✅ A POSIX toolchain accessible from JavaScript/TypeScript
- ✅ 100% backwards compatible with existing Electron apps

---

## TABLE OF CONTENTS

1. [Architecture Overview](#1-architecture-overview)
2. [Core Components](#2-core-components)
3. [ZORYA-INI Integration (Optional)](#3-zorya-ini-integration-optional)
4. [Ordinal Integration (Optional)](#4-ordinal-integration-optional)
5. [NAPI Bindings](#5-napi-bindings)
6. [POSIX Tools Integration](#6-posix-tools-integration)
7. [Configuration Mapping](#7-configuration-mapping)
8. [API Reference](#8-api-reference)
9. [Implementation Phases](#9-implementation-phases)
10. [Success Metrics](#10-success-metrics)

---

## 1. ARCHITECTURE OVERVIEW

### 1.1 Layer Model

```
┌─────────────────────────────────────────────────────────────┐
│                    YOUR ELECTRON APP                        │
│                  (unchanged, works as-is)                   │
├─────────────────────────────────────────────────────────────┤
│                     PULSAR LAYER                            │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   Config    │  │   Build     │  │   Native Accel      │  │
│  │  (optional) │  │  (optional) │  │   (opt-in)          │  │
│  │             │  │             │  │                     │  │
│  │ ZORYA-INI   │  │  Ordinal    │  │  ZORYA C SDK        │  │
│  │ pulsar.ini  │  │  tasks      │  │  via NAPI           │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                      ELECTRON                               │
│              (Chromium + Node.js + V8)                      │
├─────────────────────────────────────────────────────────────┤
│                    OPERATING SYSTEM                         │
│                 (Linux / macOS / Windows)                   │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 Opt-In Philosophy

Every Pulsar feature is opt-in. Developers choose their adoption level:

| Level | Description | Config Format | Build System | Native Accel |
|-------|-------------|---------------|--------------|--------------|
| 0 | Vanilla Electron | JSON | npm scripts | None |
| 1 | Pulsar Lite | JSON | npm scripts | NAPI available |
| 2 | Pulsar Config | pulsar.ini | npm scripts | NAPI available |
| 3 | Pulsar Build | pulsar.ini | Ordinal | NAPI available |
| 4 | Pulsar Full | pulsar.ini | Ordinal | Full ZORYA SDK |

**ALL LEVELS ARE VALID. NO JUDGMENT. NO FORCED MIGRATION.**

### 1.3 Backwards Compatibility Guarantee

```typescript
// This ALWAYS works, forever:
const { app } = require('electron');

// Pulsar augments, never replaces:
const { app } = require('pulsar/electron'); // Same API, more capabilities
```

---

## 2. CORE COMPONENTS

### 2.1 Component Matrix

| Component | Source | Purpose | Status |
|-----------|--------|---------|--------|
| `pulsar-core` | New | Runtime augmentation layer | TO BUILD |
| `pulsar-config` | ZORYA-INI | Optional INI config parser | READY (C SDK) |
| `pulsar-build` | Ordinal | Optional task runner | READY (C SDK) |
| `pulsar-napi` | ZORYA SDK | Native bindings | PARTIAL |
| `pulsar-posix` | New | POSIX tools wrapper | TO BUILD |

### 2.2 Dependency Graph

```
pulsar-core (standalone, no deps)
    │
    ├── pulsar-config (optional)
    │       └── @pulsar/zorya-ini (NAPI binding)
    │
    ├── pulsar-build (optional)
    │       └── @pulsar/ordinal (NAPI binding)
    │
    ├── pulsar-napi (optional)
    │       ├── @pulsar/nxh (hashing)
    │       ├── @pulsar/dagger (hash tables)
    │       ├── @pulsar/weave (strings)
    │       └── @pulsar/pcm (intrinsics)
    │
    └── pulsar-posix (optional, future)
            ├── @pulsar/awk
            ├── @pulsar/grep
            ├── @pulsar/sed
            └── @pulsar/find
```

### 2.3 Package Structure

```
@aspect/pulsar/                    # Main package (scoped under @aspect or @zorya)
├── core/                          # Runtime augmentation
├── config/                        # ZORYA-INI integration
├── build/                         # Ordinal integration
├── napi/                          # Native bindings
│   ├── nxh/
│   ├── dagger/
│   ├── weave/
│   └── pcm/
└── posix/                         # POSIX tools (future)
    ├── awk/
    ├── grep/
    ├── sed/
    └── find/
```

---

## 3. ZORYA-INI INTEGRATION (OPTIONAL)

### 3.1 Design Principle

**If `pulsar.ini` exists, use it. If not, fall back to JSON. No errors. No warnings.**

```typescript
// Pulsar config resolution order:
// 1. pulsar.ini (if exists)
// 2. package.json (always works)
// 3. tsconfig.json (for TypeScript)
// 4. Individual JSON configs
```

### 3.2 pulsar.ini Format

A single `pulsar.ini` can replace multiple JSON files:

```ini
; =============================================================================
; PULSAR.INI - Unified Configuration
; Replaces: package.json, tsconfig.json, .eslintrc, jest.config.js
; =============================================================================

[project]
name = my-electron-app
version = 1.0.0
description = My amazing Electron application
author = Developer Name
license = MIT

[project.repository]
type = git
url = https://github.com/user/repo

; -----------------------------------------------------------------------------
; DEPENDENCIES
; -----------------------------------------------------------------------------
[dependencies]
electron:semver = ^28.0.0
react:semver = ^18.2.0
typescript:semver = ^5.3.0

[dependencies.dev]
@types/node:semver = ^20.0.0
jest:semver = ^29.0.0
eslint:semver = ^8.0.0

; -----------------------------------------------------------------------------
; SCRIPTS / TASKS (Ordinal format)
; -----------------------------------------------------------------------------
[tasks]
dev = electron .
build = tsc && electron-builder
test = jest
lint = eslint src/

[tasks.build.config]
outDir = dist
platform = linux,darwin,win32

; -----------------------------------------------------------------------------
; TYPESCRIPT CONFIG
; Replaces: tsconfig.json
; -----------------------------------------------------------------------------
[typescript]
target = ES2022
module = NodeNext
moduleResolution = NodeNext
strict:bool = true
esModuleInterop:bool = true
skipLibCheck:bool = true
outDir = ./dist
rootDir = ./src

[typescript.include]
0 = src/**/*

[typescript.exclude]
0 = node_modules
1 = dist

; -----------------------------------------------------------------------------
; ESLINT CONFIG
; Replaces: .eslintrc.json
; -----------------------------------------------------------------------------
[eslint]
root:bool = true

[eslint.env]
browser:bool = true
node:bool = true
es2022:bool = true

[eslint.extends]
0 = eslint:recommended
1 = @typescript-eslint/recommended

[eslint.rules]
no-unused-vars = warn
@typescript-eslint/no-explicit-any = off

; -----------------------------------------------------------------------------
; JEST CONFIG
; Replaces: jest.config.js
; -----------------------------------------------------------------------------
[jest]
preset = ts-jest
testEnvironment = node

[jest.testMatch]
0 = **/*.test.ts

[jest.coverageThreshold.global]
branches:int = 80
functions:int = 80
lines:int = 80

; -----------------------------------------------------------------------------
; ELECTRON BUILDER
; Replaces: electron-builder.json
; -----------------------------------------------------------------------------
[electron-builder]
appId = com.zorya.myapp
productName = My Electron App

[electron-builder.linux]
target = AppImage
category = Development

[electron-builder.mac]
target = dmg
category = public.app-category.developer-tools

[electron-builder.win]
target = nsis

; -----------------------------------------------------------------------------
; PULSAR-SPECIFIC OPTIONS
; -----------------------------------------------------------------------------
[pulsar]
; Enable native acceleration (opt-in)
napi_acceleration:bool = true

; Use Ordinal for builds instead of npm scripts
use_ordinal:bool = true

; Content-addressed config caching
config_cache:bool = true
config_cache_dir = .pulsar/cache

; Parallel build workers
build_workers:int = ${env:PULSAR_WORKERS:-4}
```

### 3.3 JSON → INI Translation Table

| JSON Source | INI Section | Notes |
|-------------|-------------|-------|
| `package.json` → `name` | `[project] name` | Direct mapping |
| `package.json` → `version` | `[project] version` | Direct mapping |
| `package.json` → `dependencies` | `[dependencies]` | With `:semver` type |
| `package.json` → `devDependencies` | `[dependencies.dev]` | With `:semver` type |
| `package.json` → `scripts` | `[tasks]` | Ordinal format |
| `tsconfig.json` → `compilerOptions` | `[typescript]` | Flat mapping |
| `tsconfig.json` → `include` | `[typescript.include]` | Array syntax |
| `.eslintrc` → `rules` | `[eslint.rules]` | Direct mapping |
| `jest.config.js` → `*` | `[jest]` | All options |

### 3.4 Config API

```typescript
import { PulsarConfig } from '@aspect/pulsar/config';

// Automatic detection (pulsar.ini OR package.json)
const config = await PulsarConfig.load();

// Explicit INI
const iniConfig = await PulsarConfig.loadIni('./pulsar.ini');

// Read with type safety
const name: string = config.get('project.name');
const version: string = config.get('project.version');
const strict: boolean = config.getBool('typescript.strict');
const workers: number = config.getInt('pulsar.build_workers');

// Check source
console.log(config.source); // 'pulsar.ini' or 'package.json'

// Export to JSON (for tool compatibility)
await config.exportJson('./tsconfig.json', 'typescript');
```

### 3.5 Migration Helper

```bash
# Generate pulsar.ini from existing JSON configs
npx pulsar migrate

# Output:
# ✓ Read package.json
# ✓ Read tsconfig.json
# ✓ Read .eslintrc.json
# ✓ Read jest.config.js
# ✓ Generated pulsar.ini
# 
# Your JSON files are UNTOUCHED.
# Pulsar will use pulsar.ini when present.
# Delete JSON files at your leisure (or keep them forever).
```

---

## 4. ORDINAL INTEGRATION (OPTIONAL)

### 4.1 Design Principle

**Ordinal replaces `npm scripts` for those who want it. npm scripts always work.**

### 4.2 Ordinal vs npm scripts

| Feature | npm scripts | Ordinal |
|---------|-------------|---------|
| Syntax | JSON (no comments) | INI (with comments) |
| Dependencies | Manual ordering | Automatic DAG |
| Parallelism | External tools | Built-in |
| Caching | None | Content-addressed |
| Variables | Limited `$npm_*` | Full `${var}` interpolation |
| Includes | None | `::include` support |
| Performance | Node.js overhead | Native C execution |

### 4.3 Ordinal Task Format

```ini
; In pulsar.ini [tasks] section or separate Ordinal file

[tasks]
; Simple commands
clean = rm -rf dist/
compile = tsc

; With dependencies
build = electron-builder
build.depends = compile, clean

; With parallelism  
lint = eslint src/
test = jest
check.parallel = lint, test

; With variables
dev = electron . --port=${env:DEV_PORT:-3000}

; With conditions
prod = NODE_ENV=production npm run build
prod.condition = ${env:CI} == true
```

### 4.4 Ordinal CLI

```bash
# Run task (falls back to npm if Ordinal not configured)
npx pulsar run build

# List available tasks
npx pulsar tasks

# Show task graph
npx pulsar graph

# Run with parallelism
npx pulsar run check --parallel
```

### 4.5 Ordinal API

```typescript
import { Ordinal } from '@aspect/pulsar/build';

const ordinal = await Ordinal.load('./pulsar.ini');

// Run single task
await ordinal.run('build');

// Run with options
await ordinal.run('test', {
  parallel: true,
  verbose: true,
  env: { CI: 'true' }
});

// Get task info
const task = ordinal.getTask('build');
console.log(task.depends);  // ['compile', 'clean']
console.log(task.command);  // 'electron-builder'
```

---

## 5. NAPI BINDINGS

### 5.1 Existing Bindings (from ZORYA C SDK)

| Module | C Source | Node Binding | Status |
|--------|----------|--------------|--------|
| NXH | `nxh.c` (351 LOC) | `@pulsar/nxh` | EXISTS |
| DAGGER | `dagger.c` (774 LOC) | `@pulsar/dagger` | EXISTS |
| PCM | `pcm.h` (522 LOC) | `@pulsar/pcm` | EXISTS |
| WEAVE | `weave.c` (1301 LOC) | `@pulsar/weave` | TO BUILD |
| ZORYA-INI | `zorya_ini.c` (1397 LOC) | `@pulsar/zorya-ini` | TO BUILD |
| Ordinal | `ordinal.c` (1106 LOC) | `@pulsar/ordinal` | TO BUILD |

### 5.2 NAPI Binding API Examples

```typescript
// @pulsar/nxh - Fast hashing
import { nxh64, nxh32 } from '@pulsar/nxh';

const hash64 = nxh64('hello world');      // Returns BigInt
const hash32 = nxh32('hello world');      // Returns number
const hashBuf = nxh64(Buffer.from(...));  // Works with buffers

// @pulsar/dagger - O(1) hash tables
import { Dagger } from '@pulsar/dagger';

const table = new Dagger();
table.set('key', 'value');
const value = table.get('key');           // O(1) lookup
table.delete('key');

// @pulsar/weave - Fast string operations
import { Weave, Tablet } from '@pulsar/weave';

const str = Weave.from('hello');
const replaced = str.replace('l', 'L');   // Native speed

const intern = new Tablet();
const s1 = intern.intern('repeated');
const s2 = intern.intern('repeated');     // Same reference, O(1)

// @pulsar/zorya-ini - Config parsing
import { ZoryaIni } from '@pulsar/zorya-ini';

const ini = ZoryaIni.load('./config.ini');
const value = ini.get('section', 'key');
const num = ini.getInt('section', 'count');
const bool = ini.getBool('section', 'enabled');
```

### 5.3 Binding Implementation Priority

```
Priority 1 (Core Pulsar):
├── @pulsar/zorya-ini    ← Config layer (enables pulsar.ini)
└── @pulsar/ordinal      ← Build layer (enables task runner)

Priority 2 (Performance):
├── @pulsar/weave        ← String acceleration
└── @pulsar/dagger       ← Hash table acceleration

Priority 3 (Existing, polish):
├── @pulsar/nxh          ← Already exists, needs packaging
└── @pulsar/pcm          ← Already exists, needs packaging
```

---

## 6. POSIX TOOLS INTEGRATION

### 6.1 Vision

Bring battle-tested UNIX tools to JavaScript with native performance.

### 6.2 Target Tools

| Tool | Use Case | Implementation |
|------|----------|----------------|
| `awk` | Text processing, data extraction | NAPI wrapper around gawk |
| `grep` | Pattern searching | NAPI wrapper (or pure C impl) |
| `sed` | Stream editing | NAPI wrapper |
| `find` | File discovery | NAPI wrapper |
| `xargs` | Parallel execution | NAPI wrapper |
| `sort` | Sorting | NAPI wrapper |
| `uniq` | Deduplication | NAPI wrapper |
| `wc` | Counting | NAPI wrapper |

### 6.3 POSIX API Design

```typescript
import { awk, grep, sed, find } from '@aspect/pulsar/posix';

// AWK - The big win
const result = await awk(`
  BEGIN { count = 0 }
  /error/ { count++; print $0 }
  END { print "Total errors:", count }
`, './logs/*.log');

// GREP - Pattern search
const matches = await grep({
  pattern: /TODO|FIXME/,
  files: './src/**/*.ts',
  recursive: true,
  lineNumbers: true
});

// SED - Stream editing
const transformed = await sed('s/old/new/g', inputText);

// FIND - File discovery
const files = await find('./src', {
  name: '*.ts',
  type: 'file',
  maxDepth: 5
});

// Pipelines (like shell)
import { pipe } from '@aspect/pulsar/posix';

const result = await pipe(
  find('./logs', { name: '*.log' }),
  grep(/ERROR/),
  awk('{ print $3, $4 }'),
  sort(),
  uniq({ count: true })
);
```

### 6.4 Why POSIX in JavaScript?

**Current state (painful):**
```typescript
import { exec } from 'child_process';

// Spawning shell for every operation
exec('grep -r "pattern" ./src', (err, stdout) => {
  // Parse stdout string manually
  // Handle errors
  // Deal with platform differences
});
```

**With Pulsar POSIX (elegant):**
```typescript
import { grep } from '@aspect/pulsar/posix';

// Native speed, typed results, cross-platform
const matches = await grep({
  pattern: /pattern/,
  path: './src',
  recursive: true
});

// Returns typed objects, not strings to parse
matches.forEach(m => {
  console.log(`${m.file}:${m.line}: ${m.content}`);
});
```

---

## 7. CONFIGURATION MAPPING

### 7.1 Complete JSON → INI Mapping

This section provides exhaustive mapping for common Electron/TypeScript project configs.

#### 7.1.1 package.json

```json
{
  "name": "my-app",
  "version": "1.0.0",
  "main": "dist/main.js",
  "scripts": {
    "dev": "electron .",
    "build": "tsc && electron-builder"
  },
  "dependencies": {
    "electron": "^28.0.0"
  },
  "devDependencies": {
    "typescript": "^5.3.0"
  }
}
```

**Becomes:**

```ini
[project]
name = my-app
version = 1.0.0
main = dist/main.js

[tasks]
dev = electron .
build = tsc && electron-builder

[dependencies]
electron:semver = ^28.0.0

[dependencies.dev]
typescript:semver = ^5.3.0
```

#### 7.1.2 tsconfig.json

```json
{
  "compilerOptions": {
    "target": "ES2022",
    "module": "NodeNext",
    "strict": true,
    "outDir": "./dist",
    "rootDir": "./src"
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules"]
}
```

**Becomes:**

```ini
[typescript]
target = ES2022
module = NodeNext
strict:bool = true
outDir = ./dist
rootDir = ./src

[typescript.include]
0 = src/**/*

[typescript.exclude]
0 = node_modules
```

#### 7.1.3 .eslintrc.json

```json
{
  "root": true,
  "env": {
    "node": true,
    "es2022": true
  },
  "extends": [
    "eslint:recommended"
  ],
  "rules": {
    "no-unused-vars": "warn"
  }
}
```

**Becomes:**

```ini
[eslint]
root:bool = true

[eslint.env]
node:bool = true
es2022:bool = true

[eslint.extends]
0 = eslint:recommended

[eslint.rules]
no-unused-vars = warn
```

### 7.2 Lock File Strategy

**Problem:** `package-lock.json` is massive and auto-generated.

**Solution:** Pulsar does NOT replace lock files. They remain JSON.

```ini
[pulsar.lockfile]
; Lock files stay as JSON (auto-generated, not human-edited)
; Pulsar reads them but doesn't try to replace them
use_npm_lock:bool = true
```

**Rationale:**
- Lock files are machine-generated
- No human reads/edits them
- JSON is fine for machines
- Focus INI on human-edited configs

---

## 8. API REFERENCE

### 8.1 Core API

```typescript
// Main entry point
import Pulsar from '@aspect/pulsar';

// Initialize Pulsar (detects config automatically)
const pulsar = await Pulsar.init({
  // All options are optional
  configPath: './pulsar.ini',  // Default: auto-detect
  useOrdinal: true,            // Default: true if available
  napiAcceleration: true,      // Default: true
  cacheDir: '.pulsar/cache'    // Default: .pulsar/cache
});

// Access sub-modules
pulsar.config;   // PulsarConfig instance
pulsar.build;    // Ordinal instance (if enabled)
pulsar.napi;     // Native bindings
pulsar.posix;    // POSIX tools (future)
```

### 8.2 Config API

```typescript
import { PulsarConfig } from '@aspect/pulsar/config';

interface PulsarConfig {
  // Loading
  static load(path?: string): Promise<PulsarConfig>;
  static loadIni(path: string): Promise<PulsarConfig>;
  static loadJson(path: string): Promise<PulsarConfig>;
  
  // Reading
  get(key: string): string | undefined;
  getString(key: string, defaultValue?: string): string;
  getInt(key: string, defaultValue?: number): number;
  getBool(key: string, defaultValue?: boolean): boolean;
  getArray(key: string): string[];
  getSection(name: string): Record<string, string>;
  
  // Metadata
  readonly source: 'pulsar.ini' | 'package.json' | 'mixed';
  readonly sections: string[];
  
  // Export (for tool compatibility)
  exportJson(path: string, section?: string): Promise<void>;
  toJSON(): object;
}
```

### 8.3 Build API (Ordinal)

```typescript
import { Ordinal } from '@aspect/pulsar/build';

interface Ordinal {
  // Loading
  static load(path?: string): Promise<Ordinal>;
  
  // Task execution
  run(task: string, options?: RunOptions): Promise<TaskResult>;
  runParallel(tasks: string[]): Promise<TaskResult[]>;
  
  // Task info
  getTask(name: string): Task | undefined;
  listTasks(): Task[];
  getGraph(): TaskGraph;
  
  // Caching
  invalidateCache(task?: string): void;
  getCacheStats(): CacheStats;
}

interface RunOptions {
  parallel?: boolean;
  verbose?: boolean;
  env?: Record<string, string>;
  cwd?: string;
  timeout?: number;
}

interface Task {
  name: string;
  command: string;
  depends: string[];
  parallel: string[];
  condition?: string;
}
```

### 8.4 NAPI API

```typescript
// Hash functions
import { nxh64, nxh32, nxh64_seed } from '@aspect/pulsar/napi/nxh';

function nxh64(input: string | Buffer): bigint;
function nxh32(input: string | Buffer): number;
function nxh64_seed(input: string | Buffer, seed: bigint): bigint;

// Hash tables
import { Dagger } from '@aspect/pulsar/napi/dagger';

class Dagger<V = string> {
  constructor(options?: { capacity?: number });
  set(key: string, value: V): void;
  get(key: string): V | undefined;
  has(key: string): boolean;
  delete(key: string): boolean;
  clear(): void;
  readonly size: number;
  keys(): IterableIterator<string>;
  values(): IterableIterator<V>;
  entries(): IterableIterator<[string, V]>;
}

// String operations
import { Weave, Tablet } from '@aspect/pulsar/napi/weave';

class Weave {
  static from(str: string): Weave;
  toString(): string;
  readonly length: number;
  replace(pattern: string, replacement: string): Weave;
  replaceAll(pattern: string, replacement: string): Weave;
  split(separator: string): Weave[];
  concat(...others: Weave[]): Weave;
}

class Tablet {
  constructor();
  intern(str: string): Weave;
  has(str: string): boolean;
  readonly size: number;
  clear(): void;
}
```

---

## 9. IMPLEMENTATION PHASES

### Phase 1: Foundation (Week 1-2)

**Goal:** Core Pulsar package with basic config support

```
Tasks:
├── [ ] Create @aspect/pulsar monorepo structure
├── [ ] Implement pulsar-core runtime detection
├── [ ] Build @pulsar/zorya-ini NAPI binding
├── [ ] Implement PulsarConfig with INI + JSON fallback
├── [ ] Create `npx pulsar init` command
├── [ ] Create `npx pulsar migrate` command
└── [ ] Write comprehensive tests
```

**Deliverable:** `npm install @aspect/pulsar` works, `pulsar.ini` detected

### Phase 2: Build System (Week 3-4)

**Goal:** Ordinal integration for task running

```
Tasks:
├── [ ] Build @pulsar/ordinal NAPI binding
├── [ ] Implement task parser from [tasks] section
├── [ ] Implement dependency resolution (DAG)
├── [ ] Implement parallel execution
├── [ ] Implement content-addressed caching
├── [ ] Create `npx pulsar run <task>` command
├── [ ] Create `npx pulsar tasks` command
└── [ ] Create `npx pulsar graph` command
```

**Deliverable:** `npx pulsar run build` works with caching

### Phase 3: Native Acceleration (Week 5-6)

**Goal:** Full ZORYA C SDK accessible from JavaScript

```
Tasks:
├── [ ] Polish @pulsar/nxh binding
├── [ ] Polish @pulsar/dagger binding
├── [ ] Build @pulsar/weave binding
├── [ ] Polish @pulsar/pcm binding
├── [ ] Create unified @pulsar/napi entry point
├── [ ] Benchmark against pure JS equivalents
└── [ ] Document performance characteristics
```

**Deliverable:** All ZORYA SDK components usable from Node.js

### Phase 4: POSIX Tools (Week 7-8)

**Goal:** Battle-tested UNIX tools in JavaScript

```
Tasks:
├── [ ] Design POSIX API interfaces
├── [ ] Implement @pulsar/awk (high priority)
├── [ ] Implement @pulsar/grep
├── [ ] Implement @pulsar/sed
├── [ ] Implement @pulsar/find
├── [ ] Implement pipe() composition
└── [ ] Cross-platform testing (Linux, macOS, Windows)
```

**Deliverable:** POSIX tools work cross-platform with typed results

### Phase 5: Polish & Launch (Week 9-10)

**Goal:** Production-ready release

```
Tasks:
├── [ ] Documentation site
├── [ ] Example projects
├── [ ] VS Code extension for pulsar.ini syntax
├── [ ] Performance benchmarks published
├── [ ] Migration guides (CRA, Vite, Next.js)
├── [ ] npm publish all packages
├── [ ] GitHub release
└── [ ] Launch announcement
```

**Deliverable:** Pulsar 1.0.0 released

---

## 10. SUCCESS METRICS

### 10.1 Technical Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| Config parse time | <1ms | Benchmark vs JSON.parse |
| Task cache hit | >80% | Cache statistics |
| NAPI overhead | <5% | vs pure C benchmark |
| Memory usage | <50MB | Typical project |
| Startup time | <100ms | Cold start measurement |

### 10.2 Adoption Metrics

| Metric | 3 Month | 6 Month | 12 Month |
|--------|---------|---------|----------|
| npm downloads/week | 1,000 | 10,000 | 100,000 |
| GitHub stars | 500 | 2,000 | 10,000 |
| Production users | 10 | 100 | 1,000 |
| Contributors | 5 | 20 | 50 |

### 10.3 Quality Metrics

| Metric | Target |
|--------|--------|
| Test coverage | >90% |
| Zero critical CVEs | Always |
| Documentation completeness | >95% |
| Issue response time | <48 hours |

---

## APPENDIX A: ZORYA C SDK STATUS

All components ready for NAPI binding:

| Component | Version | LOC | Tests | Valgrind | Status |
|-----------|---------|-----|-------|----------|--------|
| NXH | 2.0.0 | 351 | PASS | CLEAN | READY |
| DAGGER | 2.0.0 | 774 | PASS | CLEAN | READY |
| WEAVE | 1.0.0 | 1301 | PASS | CLEAN | READY |
| ZORYA-INI | 1.0.0 | 1397 | PASS | CLEAN | READY |
| Ordinal | 1.0.0 | 1106 | PASS | CLEAN | READY |
| PCM | 1.0.0 | 522 | PASS | CLEAN | READY |

**Total: 5,451 LOC of battle-tested C ready for NAPI**

---

## APPENDIX B: COMPETITIVE ANALYSIS

| Tool | Config Format | Extensibility | Native Perf | Simplicity |
|------|---------------|---------------|-------------|------------|
| npm | JSON | Low | No | Medium |
| yarn | JSON/YAML | Medium | No | Medium |
| pnpm | JSON/YAML | Medium | No | Medium |
| Bun | JSON/TOML | High | Partial | Medium |
| **Pulsar** | **INI/JSON** | **High** | **Yes** | **High** |

**Pulsar's differentiators:**
1. INI is simpler than JSON, YAML, TOML
2. Full native acceleration (ZORYA C SDK)
3. 100% backwards compatible (JSON always works)
4. "Adopt at your leisure" philosophy
5. POSIX tools in JavaScript (unique)

---

## APPENDIX C: FAQ

**Q: Do I have to learn INI to use Pulsar?**
A: No. Pulsar works with your existing JSON configs. INI is optional.

**Q: Will Pulsar break my existing project?**
A: No. Pulsar only adds capabilities. Your project works exactly as before.

**Q: Is Pulsar a fork of npm/yarn/pnpm?**
A: No. Pulsar augments package managers, doesn't replace them.

**Q: Why INI instead of TOML or YAML?**
A: INI is the simplest format that supports our needs. Learnable in 5 minutes.

**Q: Does Pulsar work on Windows?**
A: Yes. All NAPI bindings are cross-platform.

**Q: Is Pulsar production-ready?**
A: The C SDK is production-ready. JavaScript bindings are pre-alpha.

---

**ZORYA CORPORATION**  
*Engineering Excellence, Democratized*

---

*Document Version: 0.1.0*  
*Last Updated: December 21, 2025*  
*Next Review: January 2026*
