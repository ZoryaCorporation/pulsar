# Zorya POSIX Toolkit & ZoryaWrite Platform Strategy

**Version:** 1.0  
**Date:** December 20, 2025  
**Author:** Anthony Taliento, Zorya Corp  
**Status:** Planning Phase

---

## Executive Summary

**Mission:** Upstream proven POSIX/UNIX tools into TypeScript to give modern applications 40+ years of battle-tested functionality. Build ZoryaWrite as the flagship product demonstrating the power of this approach.

**Core Philosophy:**
> "Don't reinvent what UNIX perfected. Upstream it, wrap it in TypeScript, ship it cross-platform."

**Revenue Streams:**
1. **Open-source toolkit** → GitHub sponsorships ($36K-$180K/year projected)
2. **ZoryaWrite application** → One-time purchases ($8-$20, target 10K-50K sales)
3. **Future PaaS** → Optional enhancement services (Year 2+)

---

## Part 1: Zorya POSIX Toolkit (Open Source)

### Overview

**GitHub Repository:** `github.com/zorya-corp/posix-toolkit`  
**License:** MIT (wrappers) + respective tool licenses (GPL/BSD/Apache)  
**Target Audience:** TypeScript/Node.js developers tired of reinventing text processing, compression, and system tools

### Package Architecture

**Monorepo Structure:**
```
zorya-corp/posix-toolkit/
├── packages/
│   ├── awk/              # @zorya/awk - Text processing powerhouse
│   ├── ripgrep/          # @zorya/ripgrep - Fast search (Rust-powered)
│   ├── compression/      # @zorya/compression - zstd, lz4, brotli
│   ├── sed/              # @zorya/sed - Stream editing
│   ├── grep/             # @zorya/grep - Pattern matching
│   ├── archive/          # @zorya/archive - unzip, pax, 7z
│   ├── termios/          # @zorya/termios - Terminal control
│   ├── io-uring/         # @zorya/io-uring - Kernel-level async I/O
│   ├── pipes/            # @zorya/posix-pipes - Pipeline composition
│   ├── mmap/             # @zorya/mmap - Memory-mapped files (advanced)
│   └── shared-memory/    # @zorya/shared-memory - IPC (advanced)
├── binaries/
│   ├── linux-x64/
│   ├── darwin-arm64/
│   ├── darwin-x64/
│   └── win32-x64/
├── examples/
├── docs/
└── scripts/
```

---

## Development Roadmap

### Phase 1: Foundation (Weeks 1-8)

#### **Week 1-2: @zorya/ripgrep** 
**Priority:** 🔥🔥🔥🔥🔥 SHIP FIRST

**Why ripgrep first:**
- Binary already exists (just wrap it)
- Massive performance win (10x faster than grep, 50x faster than JS)
- Huge community appeal (VS Code uses it internally)
- HackerNews front page potential

**Implementation:**
```typescript
// packages/ripgrep/src/index.ts
import { execFile } from 'child_process';
import { promisify } from 'util';
import path from 'path';

const execFileAsync = promisify(execFile);

export interface SearchOptions {
  smartCase?: boolean;
  ignoreCase?: boolean;
  hidden?: boolean;
  followLinks?: boolean;
  maxDepth?: number;
  fileType?: string[];
}

export interface SearchResult {
  file: string;
  line: number;
  column: number;
  match: string;
  context: string;
}

export async function search(
  pattern: string,
  searchPath: string,
  options: SearchOptions = {}
): Promise<SearchResult[]> {
  const rgPath = getRipgrepBinary(); // Auto-detect platform
  
  const args = [
    '--json',
    options.smartCase !== false ? '--smart-case' : '',
    options.hidden ? '--hidden' : '',
    pattern,
    searchPath
  ].filter(Boolean);
  
  const { stdout } = await execFileAsync(rgPath, args);
  return parseRipgrepJson(stdout);
}

export async function replaceAll(
  pattern: string,
  replacement: string,
  searchPath: string,
  options: { dryRun?: boolean } = {}
): Promise<ReplaceResult[]> {
  // Implementation using rg --replace
}

function getRipgrepBinary(): string {
  const platform = process.platform;
  const arch = process.arch;
  const binName = platform === 'win32' ? 'rg.exe' : 'rg';
  
  return path.join(__dirname, '..', 'bin', `${platform}-${arch}`, binName);
}
```

**Binary Distribution:**
- Download from: https://github.com/BurntSushi/ripgrep/releases
- Bundle in npm package (post-install script)
- Platform detection: Linux (x64, arm64), macOS (Intel, ARM), Windows (x64)

**Testing:**
```bash
npm install @zorya/ripgrep
node -e "require('@zorya/ripgrep').search('TODO', '.')"
```

**Launch Strategy:**
- Publish to npm
- Post on HackerNews: "We made TypeScript search 50x faster by upstreaming ripgrep"
- Reddit r/typescript, r/programming
- Twitter: Tag @BurntSushi (ripgrep author) for visibility

**Success Metrics:**
- Target: 5K GitHub stars in first 3 months
- Target: 50+ sponsors ($1K/month) by month 6

---

#### **Week 3-4: @zorya/compression (zstd)**
**Priority:** 🔥🔥🔥🔥 CRITICAL FOR ZORYAWRITE

**Why zstd:**
- 50x faster than Node.js `zlib`
- Critical for ZoryaWrite document compression
- Facebook/Meta uses it (proven at scale)

**Implementation Options:**

**Option A: Binary wrapper (faster to ship)**
```typescript
import { spawn } from 'child_process';

export async function compress(
  input: Buffer,
  level: number = 3
): Promise<Buffer> {
  return new Promise((resolve, reject) => {
    const zstd = spawn('zstd', [`-${level}`, '-c']);
    
    const chunks: Buffer[] = [];
    zstd.stdout.on('data', chunk => chunks.push(chunk));
    zstd.stdout.on('end', () => resolve(Buffer.concat(chunks)));
    zstd.on('error', reject);
    
    zstd.stdin.write(input);
    zstd.stdin.end();
  });
}

export async function decompress(input: Buffer): Promise<Buffer> {
  // Similar implementation with zstd -d
}
```

**Option B: Native bindings (better performance)**
```typescript
// Use existing battle-tested package
import zstd from '@mongodb/zstd';

export const compress = zstd.compress;
export const decompress = zstd.decompress;

// Add convenience methods
export async function compressFile(src: string, dest: string) {
  const data = await fs.readFile(src);
  const compressed = await compress(data);
  await fs.writeFile(dest, compressed);
}
```

**Recommendation:** Start with Option B (leverage @mongodb/zstd), add custom wrappers for convenience.

**Additional formats:**
- **lz4:** Ultra-fast (2GB/s), low compression ratio
- **brotli:** Best compression, slower (good for static assets)

**Testing:**
```bash
# Benchmark vs native zlib
node benchmark.js
# Input: 100MB file
# zlib:  10 MB/s
# zstd:  500 MB/s (50x faster!)
```

---

#### **Week 5-6: @zorya/awk**
**Priority:** 🔥🔥🔥🔥 FLAGSHIP PACKAGE

**Why awk:**
- Handles CSV, JSON conversion, log parsing, data transformation
- TSX has NO native solution (everyone uses 10+ packages)
- Proven since 1977 (48 years of reliability)

**Implementation:**
```typescript
export class AwkProcessor {
  private gawkPath: string;
  
  constructor() {
    this.gawkPath = getGawkBinary(); // Platform-specific
  }
  
  async csvToJson(csvFile: string): Promise<object[]> {
    const script = path.join(__dirname, '..', 'scripts', 'csv_to_json.awk');
    const { stdout } = await execFileAsync(this.gawkPath, ['-f', script, csvFile]);
    
    return stdout.split('\n')
      .filter(Boolean)
      .map(line => JSON.parse(line));
  }
  
  async findPattern(
    pattern: string | RegExp,
    files: string[]
  ): Promise<Match[]> {
    const script = path.join(__dirname, '..', 'scripts', 'filepos_finder.awk');
    const regex = pattern instanceof RegExp ? pattern.source : pattern;
    
    const { stdout } = await execFileAsync(
      this.gawkPath,
      ['-f', script, regex, ...files]
    );
    
    return parseAwkMatches(stdout);
  }
  
  async execute(scriptPath: string, input: string): Promise<string> {
    // Run custom awk scripts
    const { stdout } = await execFileAsync(
      this.gawkPath,
      ['-f', scriptPath, input]
    );
    return stdout;
  }
  
  async transform(
    data: string,
    transformer: (line: string) => string
  ): Promise<string> {
    // Generate inline awk script from TypeScript function
    const awkScript = generateAwkFromFunction(transformer);
    // Execute via temp script file
  }
}
```

**Bundled awk scripts:**
```
packages/awk/scripts/
├── csv_to_json.awk        # CSV → JSON Lines
├── filepos_finder.awk     # Regex search with line:col
├── json_to_csv.awk        # JSON → CSV
├── log_parser.awk         # Parse common log formats
└── data_aggregator.awk    # Sum/avg/count operations
```

**Cross-platform binary compilation:**
```bash
# Compile gawk for all platforms using MinGW (on Fedora 42)

# Linux (native)
./configure && make
mv gawk binaries/linux-x64/gawk

# Windows (cross-compile)
sudo dnf install mingw64-gcc
./configure --host=x86_64-w64-mingw32
make
mv gawk.exe binaries/win32-x64/gawk.exe

# macOS (requires macOS SDK)
# Use GitHub Actions for actual macOS build
```

---

#### **Week 7-8: @zorya/termios**
**Priority:** 🔥🔥🔥 (Enables Terminal Zero game, TUI apps)

**Why termios:**
- TSX has NO low-level terminal control
- Enables: cursor positioning, colors (RGB), raw input mode
- Foundation for Terminal Zero cyberpunk game

**Implementation:**
```typescript
// Node.js native addon (C++)
export const terminal = {
  cursor: {
    move(x: number, y: number): void,
    hide(): void,
    show(): void,
    save(): void,
    restore(): void
  },
  
  color: {
    rgb(r: number, g: number, b: number): void,
    reset(): void,
    background(r: number, g: number, b: number): void
  },
  
  input: {
    raw(): void,        // Disable line buffering
    cooked(): void,     // Restore normal mode
    readKey(): Promise<KeyPress>
  },
  
  screen: {
    clear(): void,
    size(): { width: number, height: number },
    alternateBuffer(): void  // Switch to alternate screen
  }
};
```

**Use case example:**
```typescript
import { terminal } from '@zorya/termios';

// Build TUI app
terminal.screen.clear();
terminal.cursor.move(10, 5);
terminal.color.rgb(255, 0, 110); // Zorya Pink
console.log('ZoryaWrite');
terminal.input.raw();

const key = await terminal.input.readKey();
if (key === 'q') process.exit(0);
```

---

### Phase 2: Advanced Tools (Weeks 9-16)

#### **Week 9-10: @zorya/archive**
- unzip, pax, 7z support
- Extract DOCX internals (DOCX is zip of XML files)
- Import/export document collections

#### **Week 11-12: @zorya/sed & @zorya/grep**
- Stream editing, bulk find/replace
- Complement ripgrep with POSIX grep for compatibility

#### **Week 13-14: @zorya/posix-pipes**
- UNIX-style pipeline composition in TSX
- Stream data through tools without temp files

#### **Week 15-16: @zorya/io-uring**
- Kernel-level async I/O (Linux only initially)
- 10x faster file operations than Node.js libuv
- Makes TSX faster than Rust for file I/O

---

### Phase 3: Expert-Level (Months 4-6)

#### **@zorya/mmap** (Memory-Mapped Files)
**Complexity:** ⭐⭐⭐⭐⭐  
**Impact:** 🔥🔥🔥🔥🔥 MASSIVE

**Enables:**
- Edit 10GB+ files that don't fit in RAM
- Zero-copy operations (OS handles paging)
- Instant file "loading" (mmap is immediate)

**Implementation:** Requires C++ native addons, platform-specific code

#### **@zorya/shared-memory**
- Zero-copy IPC between Electron main/renderer processes
- Real-time collaboration infrastructure
- Faster than JSON serialization

---

## Testing Strategy

### Cross-Platform Testing

**Development:** Fedora 42 (native Linux)  
**CI/CD:** GitHub Actions (free for open source)

**GitHub Actions Workflow:**
```yaml
# .github/workflows/test.yml
name: Cross-Platform Tests

on: [push, pull_request]

jobs:
  test-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-node@v3
        with:
          node-version: 20
      - run: npm ci
      - run: npm test
      
  test-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-node@v3
      - run: npm ci
      - run: npm test
      
  test-macos:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-node@v3
      - run: npm ci
      - run: npm test
```

**Local Windows testing:**
- Wine (quick sanity checks)
- Windows VM (full GUI testing if needed)
- Community beta testers

---

## GitHub Strategy & Community Growth

### Launch Plan

**Phase 1: ripgrep launch (Week 2)**
- Publish `@zorya/ripgrep` to npm
- HackerNews post: "We made TypeScript search 50x faster"
- Reddit: r/typescript, r/node, r/programming
- Twitter: Tag ripgrep author for visibility

**Phase 2: Toolkit announcement (Week 8)**
- Full monorepo reveal
- Blog post: "Why we upstream POSIX instead of rewriting in JavaScript"
- Show benchmarks: awk vs papaparse, zstd vs zlib, etc.

**Phase 3: Ongoing engagement**
- Weekly "Tool Spotlight" blog posts
- Example projects using the toolkit
- Community contributions (accept PRs for new tools)

### GitHub Sponsors

**Tier Structure:**
- **$5/month:** Supporter badge, name in README
- **$25/month:** Priority bug fixes
- **$100/month:** Feature requests prioritized
- **$500/month:** Corporate sponsor (logo on website)

**Revenue Projections:**

| Timeline | Stars | Sponsors | Monthly Revenue | Annual Revenue |
|----------|-------|----------|-----------------|----------------|
| Month 3 | 3,000 | 25 | $500 | $6,000 |
| Month 6 | 8,000 | 75 | $1,500 | $18,000 |
| Year 1 | 15,000 | 150 | $3,000 | $36,000 |
| Year 2 | 30,000 | 400 | $8,000 | $96,000 |

**Optimistic (if viral):** $180K/year by Year 2

---

## Part 2: ZoryaWrite Application

### Overview

**Product:** Cross-platform document editor with PDF generation  
**Platform:** Electron + React/TSX + Zorya POSIX Toolkit  
**Target Market:** Burn-phase startups, privacy-conscious users, Microsoft ecosystem complementers  
**Positioning:** Adobe InDesign alternative for $8, Google Docs alternative with privacy

### Architecture

**Technology Stack:**
```
ZoryaWrite/
├── Frontend (Electron Renderer)
│   ├── React + TypeScript
│   ├── WYSIWYG Editor: ProseMirror or TipTap
│   ├── UI Framework: Tailwind CSS
│   └── State: Zustand or Jotai
├── Backend (Electron Main)
│   ├── Quarto CLI (bundled) - PDF generation
│   ├── @zorya/compression - Document compression (zstd)
│   ├── @zorya/ripgrep - Find in files
│   ├── @zorya/awk - Data import (CSV → tables)
│   └── @zorya/archive - DOCX import/export
├── AI (Optional - Pro tier)
│   ├── Phi-3 Mini (2.3GB) - Grammar, summarization
│   ├── node-llama-cpp - GGUF model runner
│   └── GPU acceleration (Metal/CUDA)
└── Cloud Integration (Optional)
    ├── Microsoft OAuth (MSAL)
    ├── OneDrive API (sync)
    └── Google Drive API (migration tool)
```

### Core Features

#### **Free Tier:**
- Basic WYSIWYG editing
- Local file storage
- Export to TXT, Markdown
- Limited to 10 documents

#### **Standard ($8 one-time):**
- ✅ Unlimited documents
- ✅ PDF export (via Quarto)
- ✅ DOCX import/export (MS Word compatibility)
- ✅ Real-time compression (zstd - 3x smaller files)
- ✅ Fast search (ripgrep - 50x faster)
- ✅ CSV import (awk - instant tables)
- ✅ Cloud sync (OneDrive, Google Drive)
- ✅ Google Docs migration tool
- ✅ Offline-first (data lives locally)

#### **Pro ($20 one-time):**
- ✅ Everything in Standard
- ✅ Local AI assistant (Phi-3 Mini, 100% offline)
  - Grammar checking (no Grammarly subscription)
  - Summarization
  - Writing suggestions
  - Conversational chat
- ✅ LaTeX support (full typesetting)
- ✅ Advanced templates (reports, proposals, books)
- ✅ Python scripting API (automate workflows)
- ✅ Mobile Pro unlock (iOS/Android)
- ✅ Interactive presentations (HTML + JS calculators)

### Competitive Positioning

| Feature | Google Docs | MS Word | Notion | ZoryaWrite Pro |
|---------|-------------|---------|--------|----------------|
| **Price** | Free (data) | $70/year | $10/month | **$20 once** |
| **Offline** | ❌ | ✅ | ❌ | ✅ |
| **Privacy** | ❌ Scans all | ✅ | ❌ Cloud | ✅ **Local-only** |
| **AI Grammar** | Basic | ❌ | $10/mo extra | ✅ **Included** |
| **PDF Export** | Basic | ✅ | Limited | ✅ **Pro-grade** |
| **DOCX Support** | ⚠️ Lossy | ✅ Native | ⚠️ Basic | ✅ **Full** |
| **Batch Processing** | ❌ | Limited | ❌ | ✅ **POSIX power** |
| **Search Speed** | Slow | Slow | Database | ✅ **ripgrep** |
| **API Costs** | N/A | N/A | Hidden | **$0 (local AI)** |

**Key Differentiators:**
1. **One-time purchase** (anti-subscription positioning)
2. **Local AI** (no Grammarly/ChatGPT bills, perfect privacy)
3. **POSIX power** (batch operations competitors can't match)
4. **Privacy-first** (data never leaves your device)
5. **Microsoft-friendly** (complements M365, doesn't compete)

### Development Roadmap

#### **Month 1-2: MVP**
- Electron + React setup
- Basic WYSIWYG editor (TipTap)
- Local file save/load
- Markdown export

#### **Month 3: Standard Features**
- Quarto integration (PDF export)
- DOCX import/export
- @zorya/compression (zstd)
- @zorya/ripgrep (search)

#### **Month 4: Polish & Beta**
- UI/UX refinement
- Dark mode
- Templates (5 starter templates)
- Beta testing (50 users)

#### **Month 5: Pro Features**
- Phi-3 Mini integration (local AI)
- LaTeX support
- Interactive presentations

#### **Month 6: Launch**
- Marketing campaign
- Product Hunt launch
- HackerNews post
- Reddit r/productivity

### Revenue Projections

**Conservative:**
- Month 1-3: 200 sales × $8 = $1,600
- Month 4-6: 500 sales × $8 = $4,000
- Year 1 total: 2,000 sales = $16,000 (Standard) + $4,000 (Pro) = **$20,000**

**Optimistic (if viral):**
- Year 1: 10,000 sales × $10 avg = **$100,000**
- Year 2: 20,000 sales × $10 avg = **$200,000**

**Plus:** Toolkit sponsorships ($36K-$96K/year)

**Total potential:** $136K-$296K/year by Year 2

---

## Marketing Strategy

### ZoryaWrite Positioning

**Primary Message:**
> "Adobe charges $660/year for document tools. Google mines your data. We give you professional publishing for $8, once. Your data stays yours."

**Target Audiences:**

1. **Burn-phase startups** ($0-$2M revenue)
   - Pain: Can't afford M365 ($144/year/user)
   - Current: Using Google Docs (privacy nightmare)
   - Pitch: "Use us until you can afford Microsoft"

2. **Privacy-conscious professionals**
   - Lawyers, doctors, journalists
   - Pain: Grammarly reads confidential documents
   - Pitch: "Local AI, HIPAA-friendly by design"

3. **Students & writers**
   - Pain: Can't afford Adobe ($660/year) or Scrivener ($50)
   - Pitch: "Professional tools for $8"

4. **Microsoft ecosystem users**
   - Pain: Word is overkill, Google Docs conflicts with M365
   - Pitch: "Lightweight Word alternative that plays nice with M365"

### Launch Channels

**Pre-launch (Months 1-5):**
- Build in public (Twitter/X)
- Dev blog (weekly updates)
- Email list (beta signup)

**Launch week:**
- Product Hunt (aim for #1 Product of the Day)
- HackerNews: "We built Adobe alternative using POSIX tools"
- Reddit: r/productivity, r/writing, r/privacy, r/selfhosted
- Twitter: Tag Microsoft, tag privacy advocates

**Post-launch:**
- YouTube reviewers (tech channels)
- Press outreach (TechCrunch, Ars Technica)
- Affiliate program (20% commission)

---

## Future: Platform-as-a-Service (Year 2+)

### Zorya Cloud (Optional Enhancement)

**Philosophy:** Data lives on user's device (source of truth). Cloud is OPTIONAL sync/collab layer.

**Free Tier:**
- WebAssembly POSIX tools (run in browser)
- Real-time collaboration (E2E encrypted)
- Document sharing links (encrypted, 7-day expiry)

**Pro Tier ($5/month):**
- Sync between devices (zero-knowledge encryption)
- Version history (encrypted server-side)
- Team workspaces

**Enterprise ($50/user/month):**
- Self-hosted option (run on customer infrastructure)
- SSO (SAML, Active Directory)
- Audit logs
- Priority support

**Key:** User can ALWAYS export to local. Cloud is convenience, not lock-in.

---

## Risk Mitigation

### Technical Risks

**Risk:** Windows binary compatibility issues  
**Mitigation:** GitHub Actions testing, Wine for quick checks, community beta testers

**Risk:** POSIX tools don't work on all platforms  
**Mitigation:** Graceful fallbacks (e.g., JS CSV parser if awk unavailable)

**Risk:** Electron app size too large  
**Mitigation:** Users don't care (Notion is 200MB, VS Code is 400MB)

### Business Risks

**Risk:** Low adoption (toolkit gets ignored)  
**Mitigation:** Launch ripgrep first (easy win), build credibility before full toolkit

**Risk:** GitHub Sponsors revenue lower than projected  
**Mitigation:** ZoryaWrite app provides primary revenue, sponsors are bonus

**Risk:** Competitors copy approach  
**Mitigation:** First-mover advantage, community moat, mmap/io-uring too complex to replicate quickly

---

## Success Metrics

### Toolkit (Year 1)
- ✅ 15,000+ GitHub stars
- ✅ 150+ sponsors
- ✅ $36K+ annual sponsorship revenue
- ✅ 10,000+ npm downloads/month

### ZoryaWrite (Year 1)
- ✅ 2,000+ sales
- ✅ $20K+ revenue
- ✅ 4.5+ star rating
- ✅ Featured on Product Hunt

### Combined (Year 2)
- ✅ Toolkit: 30K stars, $96K/year
- ✅ ZoryaWrite: 20K sales, $200K revenue
- ✅ Total: $296K annual revenue
- ✅ Runway for Terminal Zero game development

---

## Next Immediate Actions

### This Week:
1. ✅ Create GitHub organization: `zorya-corp`
2. ✅ Set up monorepo: `posix-toolkit`
3. ✅ Build @zorya/ripgrep (Week 1-2 focus)
4. ✅ Write viral HackerNews post draft

### Next Month:
1. Launch ripgrep on npm + HackerNews
2. Build @zorya/compression (zstd)
3. Start ZoryaWrite MVP (Electron setup)
4. Open GitHub Sponsors

### Quarter 1 (Months 1-3):
1. Ship 4 toolkit packages (ripgrep, compression, awk, termios)
2. Reach 5K GitHub stars
3. ZoryaWrite MVP functional
4. $1K/month toolkit sponsorships

---

## Appendix: Philosophy & Manifesto

### Why Upstream Instead of Rewrite?

**The Problem with JavaScript Rewrites:**

Every text processing task has been rewritten in JavaScript:
- CSV parsing: 47 packages on npm
- Compression: 23 packages
- Search: 156 packages
- Archive handling: 31 packages

**Result:** Reinventing wheels, introducing bugs, slower performance.

**The POSIX Alternative:**

- **awk:** 48 years of optimization (1977-2025)
- **gzip:** 32 years (1993)
- **ripgrep:** 9 years, powers VS Code (2016)
- **zstd:** 9 years, powers Facebook infrastructure (2016)

**These tools have processed EXABYTES of data. They work.**

### Our Principles

1. **Reliability over novelty** - awk has fewer bugs than any JS CSV parser
2. **Performance over purity** - io_uring beats JavaScript every time
3. **Proven over popular** - 40 years of production use > GitHub trends
4. **Upstream over rewrite** - Package C binaries, don't recreate in JS
5. **Local over cloud** - User's data belongs on user's device
6. **One-time over subscription** - Respect users' wallets

### The Zorya Way

> "We didn't build a word processor. We built a UNIX-powered document engine that your grandma can use."

---

**END OF PLAN**

---

## Contacts & Links

**GitHub:** github.com/zorya-corp  
**Email:** anthony@zoryacorp.com  
**Twitter:** @ZoryaCorp  
**Website:** zoryacorp.com (to be built)

**Built by engineers who believe UNIX got it right the first time.**
