# Pulsar Documentation

Comprehensive guides for building high-performance Node.js applications with Pulsar.

## Quick Start

```bash
npm install @zoryacorporation/pulsar
```

```typescript
import { weave, compress, hash, fileops, dagger, pcm, ini } from '@zoryacorporation/pulsar';
```

## Guides

### Getting Started

- **[Getting Started](./getting-started.md)** - Installation, setup, and your first Pulsar program: **Where you are now!**

### Module Guides

| Module | Guide | Description |
|--------|-------|-------------|
| weave | [Weave Guide](./weave.md) | String primitives with arena allocation and interning |
| compress | [Compress Guide](./compress.md) | Zstandard and LZ4 compression |
| fileops | [FileOps Guide](./fileops.md) | Native file operations and watching |
| hash | [Hash Guide](./hash.md) | NXH64 non-cryptographic hashing |
| dagger | [Dagger Guide](./dagger.md) | Robin Hood hash tables |
| pcm | [PCM Guide](./pcm.md) | Hardware-accelerated bit manipulation |
| ini | [INI Guide](./ini.md) | Fast INI configuration parsing |

## Performance

Benchmarks vs pure JavaScript equivalents (Node.js 22, Linux x64):

| Operation | JS | Pulsar | Speedup |
|-----------|---:|-------:|--------:|
| String hash (1KB) | 45µs | 0.8µs | **56x** |
| Zstd compress (1MB) | N/A | 12ms | — |
| LZ4 compress (1MB) | N/A | 3ms | — |
| File read (1MB) | 2.1ms | 0.4ms | **5x** |
| Hash table insert (100K) | 89ms | 12ms | **7x** |

## Requirements

**Runtime:**
- Node.js >= 18.0.0
- Linux (glibc 2.17+) or macOS 11+
- x64 or arm64 architecture

**Build (from source):**
- C compiler (gcc, clang)
- Python 3 (for node-gyp)
- make

## See Also

- [Main README](../README.md) - Package overview and API reference
- [GitHub Repository](https://github.com/ZoryaCorporation/pulsar) - Source code and issues
