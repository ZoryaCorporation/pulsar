# Pulsar Framework Status

**Updated:** December 22, 2025  
**Phase:** Core Primitives Complete → POSIX Toolkit Next

---

## Mission Reminder

> "Why use Rust when you can use TSX and have great UIs AND great performance?"

**Business Model:** Give away the engine, sell cars.

---

## ✅ Completed Core Primitives

### @aspect/ordinal - Foundation Layer
| Component | Status | Performance |
|-----------|--------|-------------|
| **NXH** | ✅ DONE | 64-bit hashing, 2.9M ops/sec |
| **DAGGER** | ✅ DONE | O(1) hash tables, 1.9M ops/sec |
| **PCM** | ✅ DONE | Bit operations, 21M ops/sec |
| **ZORYA-INI** | ✅ DONE | Native config parsing |

### @aspect/gawk - Text Processing
| Feature | Status | Tests |
|---------|--------|-------|
| NAPI bindings | ✅ DONE | 12/12 passing |
| CSV → JSON | ✅ DONE | Native speed |
| Pattern matching | ✅ DONE | Full AWK syntax |
| TypeScript API | ✅ DONE | Clean class wrappers |

### @aspect/weave - Industrial Strings
| Component | Status | Purpose |
|-----------|--------|---------|
| **Weave** | ✅ DONE | Mutable fat strings, O(1) length |
| **Cord** | ✅ DONE | Deferred concat, single allocation |
| **Tablet** | ✅ DONE | String interning, O(1) comparison |
| **Arena** | ✅ DONE | Bump allocator with temp scopes |

**Test Results:** 25/25 passing, clean exit (no memory leaks)

---

## 🔥 Next Phase: POSIX Toolkit

### Priority 1: @aspect/compress
**Why:** Compression in TSX is a nightmare. Everyone uses different packages, performance varies wildly, API is inconsistent.

| Algorithm | Speed | Ratio | Use Case |
|-----------|-------|-------|----------|
| **zstd** | 500 MB/s | 3x | General purpose (ZoryaWrite docs) |
| **lz4** | 2 GB/s | 2x | Real-time (game assets, logs) |
| **brotli** | 50 MB/s | 4x | Static assets (web deployment) |
| **gzip** | 100 MB/s | 2.5x | Compatibility (legacy systems) |

**Implementation approach:**
- Native NAPI bindings (like weave-napi)
- Unified API: `compress(data, algorithm, level)`
- Streaming support for large files
- Zero-copy where possible

```typescript
import { compress, decompress, Algorithm } from '@aspect/compress';

// Simple API
const packed = await compress(data, Algorithm.ZSTD, { level: 3 });
const unpacked = await decompress(packed);

// Streaming for large files
const stream = createCompressStream(Algorithm.LZ4);
fs.createReadStream('huge.log').pipe(stream).pipe(fs.createWriteStream('huge.log.lz4'));
```

### Priority 2: @aspect/archive
**Why:** DOCX files are just zips. Can't edit Word docs without zip handling.

| Format | Read | Write | Notes |
|--------|------|-------|-------|
| **ZIP** | ✅ | ✅ | DOCX, XLSX, JAR, EPUB |
| **tar** | ✅ | ✅ | Unix archives |
| **tar.gz** | ✅ | ✅ | Combined with compress |
| **7z** | ✅ | ⚠️ | Read priority, write later |

```typescript
import { Archive } from '@aspect/archive';

// Extract DOCX internals
const docx = await Archive.open('document.docx');
const content = await docx.readText('word/document.xml');

// Create archive
const archive = Archive.create('backup.tar.gz');
await archive.addDir('./src');
await archive.finalize();
```

### Priority 3: @aspect/termios
**Why:** VS Code uses a Rust implementation for their terminal. We can do better natively.

| Capability | Status | Impact |
|------------|--------|--------|
| **Raw mode** | 🎯 Target | Instant key response |
| **Cursor control** | 🎯 Target | TUI apps |
| **24-bit color** | 🎯 Target | Modern terminal aesthetics |
| **Mouse events** | 🎯 Target | Interactive CLIs |
| **Window size** | 🎯 Target | Responsive layouts |
| **Alternate buffer** | 🎯 Target | Full-screen apps |

```typescript
import { Terminal } from '@aspect/termios';

const term = new Terminal();

// Go raw for instant input
term.raw();

// TUI rendering
term.clear();
term.moveTo(10, 5);
term.setColor(255, 0, 110); // Zorya Pink
term.write('ZoryaWrite');

// Read single keypress
const key = await term.readKey();
if (key.name === 'q') term.restore();
```

**Real impact:** Enables Terminal Zero game, CLI tools that feel native.

---

## 📦 Package Structure

```
@aspect/pulsar (umbrella package)
├── @aspect/ordinal      ✅ DONE - Core primitives
├── @aspect/gawk         ✅ DONE - Text processing  
├── @aspect/weave        ✅ DONE - Industrial strings
├── @aspect/compress     🔜 NEXT - Compression
├── @aspect/archive      🔜 NEXT - Zip/tar handling
├── @aspect/termios      🔜 NEXT - Terminal control
├── @aspect/ripgrep      📋 PLANNED - Fast search
├── @aspect/mmap         📋 PLANNED - Memory-mapped files
└── @aspect/io-uring     📋 PLANNED - Kernel async I/O
```

---

## 🎮 Demo Applications

### Demo 1: `zorya-csv` - CSV Swiss Army Knife
**Showcase:** gawk-napi + weave-napi performance

```bash
# Convert 1GB CSV to JSON in seconds (not minutes)
zorya-csv convert data.csv --to json --output data.json

# Filter and transform
zorya-csv filter data.csv --where "age > 21" --select "name,email"

# Aggregate
zorya-csv stats data.csv --group-by country --sum population
```

**Why impressive:** Node.js native CSV handling chokes on large files. This demonstrates industrial-grade performance.

### Demo 2: `zorya-pack` - Universal Archiver
**Showcase:** @aspect/compress + @aspect/archive

```bash
# Create ultra-compressed archive
zorya-pack create backup.zst ./project --algorithm zstd --level 19

# Extract any format
zorya-pack extract document.docx --to ./extracted

# Stream compression (pipe-friendly)
cat huge.log | zorya-pack compress --lz4 > huge.log.lz4
```

**Why impressive:** Single tool handles all formats. Faster than system tools.

### Demo 3: `zorya-term` - TUI Framework Demo
**Showcase:** @aspect/termios raw power

- Interactive file browser
- Real-time log viewer with color
- System monitor (htop-style)

**Why impressive:** Pure TSX doing what people think requires ncurses/Rust.

### Demo 4: `zorya-search` - Ripgrep for Node
**Showcase:** Binary wrapper with TypeScript ergonomics

```typescript
import { search } from 'zorya-search';

// Find all TODOs in project
const todos = await search('TODO|FIXME|HACK', './src', {
  fileTypes: ['ts', 'tsx'],
  context: 2
});

// Interactive replace
await search.replace('oldApi', 'newApi', './src', { confirm: true });
```

**Why impressive:** VS Code search speed, available as npm package.

---

## 💰 Revenue Roadmap

### Phase 1: Open Source Traction (Months 1-6)
- Ship @aspect/pulsar with all primitives
- HackerNews launches for each major component
- Target: 10K stars, 100 sponsors

### Phase 2: Demo Apps (Months 3-6)
- Release polished demo apps
- Each demo is genuinely useful (not just tech demos)
- Community building around the tools

### Phase 3: ZoryaWrite (Months 6-12)
- Word processor built on Pulsar
- Targets Adobe/Microsoft frustrated users
- One-time purchase: $8 Standard, $20 Pro
- Target: 10K-50K sales Year 1

### Revenue Projections (Conservative)

| Source | Year 1 | Year 2 |
|--------|--------|--------|
| GitHub Sponsors | $12K | $36K |
| ZoryaWrite Sales | $40K | $100K |
| **Total** | **$52K** | **$136K** |

*Covers living expenses, enables full-time development.*

---

## Technical Advantages Summary

| Problem | JavaScript Solution | Pulsar Solution | Improvement |
|---------|--------------------|-----------------| ------------|
| String building | Immutable concat | Weave + Cord | 10-50x faster |
| CSV parsing | papaparse (JS) | gawk-napi | 20x faster |
| Compression | zlib (slow) | zstd native | 50x faster |
| File search | fs.readdir + regex | ripgrep | 100x faster |
| Terminal control | blessed (hacky) | termios native | True raw mode |
| Hash tables | Map (slow) | DAGGER | 5x faster |
| Config parsing | JSON.parse | ZORYA-INI | Native + readable |

**The pitch:** "TSX + Pulsar = Rust performance with Electron convenience."

---

## Next Actions

1. **[ ] @aspect/compress** - Start with zstd NAPI bindings
2. **[ ] @aspect/archive** - Zip reading (critical for DOCX)
3. **[ ] @aspect/termios** - Raw mode + cursor control
4. **[ ] zorya-csv demo** - First showpiece app
5. **[ ] GitHub org setup** - zorya-corp organization
6. **[ ] npm org setup** - @aspect scope

---

*"Don't reinvent what UNIX perfected. Upstream it, wrap it in TypeScript, ship it cross-platform."*
