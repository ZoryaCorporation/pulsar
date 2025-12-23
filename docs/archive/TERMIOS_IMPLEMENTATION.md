# @aspect/termios Implementation Plan

**Priority:** 🔥🔥🔥🔥 HIGH  
**Why:** VS Code uses Rust for terminal handling. We can do this natively in TSX.

---

## The Problem

```typescript
// Current TSX terminal "solutions"

// Option 1: blessed - Massive, buggy, abandoned
import blessed from 'blessed';
const screen = blessed.screen(); // 500KB+ bundle

// Option 2: ANSI escape sequences - Manual and fragile
process.stdout.write('\x1b[2J');     // Clear screen
process.stdout.write('\x1b[10;5H');  // Move cursor - did I get the order right?
process.stdout.write('\x1b[38;2;255;0;110m'); // RGB color - pray it works

// Option 3: readline - Limited
import readline from 'readline';
readline.emitKeypressEvents(process.stdin);
process.stdin.setRawMode(true); // Works... sometimes
```

**The pain:**
- `blessed` is abandoned (last update 2018) and massive
- ANSI codes are error-prone and non-portable
- No access to actual termios/tty APIs
- No reliable raw mode across platforms
- Mouse support is hacky at best

---

## Our Solution

```typescript
import { Terminal } from '@aspect/termios';

const term = new Terminal();

// Clean, typed API
term.clear();
term.moveTo(10, 5);
term.setColor(255, 0, 110);  // RGB, not escape codes
term.write('Hello, Terminal!');

// True raw mode (instant keypress)
term.raw();
const key = await term.readKey();
console.log(key); // { name: 'a', ctrl: false, shift: false, code: 97 }
term.restore();

// Mouse support
term.enableMouse();
const event = await term.readMouse();
console.log(event); // { x: 10, y: 5, button: 'left', action: 'click' }
```

---

## Technical Architecture

### Core POSIX APIs to Wrap

```c
// termios.h - Terminal control
struct termios {
    tcflag_t c_iflag;    // Input flags
    tcflag_t c_oflag;    // Output flags
    tcflag_t c_cflag;    // Control flags
    tcflag_t c_lflag;    // Local flags
    cc_t c_cc[NCCS];     // Control characters
};

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);

// ioctl for window size
struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

int ioctl(int fd, unsigned long request, ...);
```

### NAPI Bindings

```c
// src/napi/termios_napi.c

#include <node_api.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

static struct termios original_termios;
static bool raw_mode = false;

/**
 * Enable raw mode - disable line buffering, echo, signals
 */
static napi_value enable_raw_mode(napi_env env, napi_callback_info info) {
    if (raw_mode) return get_undefined(env);
    
    // Save original settings
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        napi_throw_error(env, NULL, "tcgetattr failed");
        return NULL;
    }
    
    struct termios raw = original_termios;
    
    // Input: no break, no CR->NL, no parity, no strip, no flow control
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    
    // Output: disable post-processing
    raw.c_oflag &= ~(OPOST);
    
    // Control: 8-bit chars
    raw.c_cflag |= (CS8);
    
    // Local: no echo, no canonical, no signals, no extended
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    
    // Read returns after 1 byte, no timeout
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        napi_throw_error(env, NULL, "tcsetattr failed");
        return NULL;
    }
    
    raw_mode = true;
    return get_undefined(env);
}

/**
 * Restore original terminal settings
 */
static napi_value disable_raw_mode(napi_env env, napi_callback_info info) {
    if (!raw_mode) return get_undefined(env);
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
    raw_mode = false;
    
    return get_undefined(env);
}

/**
 * Get terminal window size
 */
static napi_value get_window_size(napi_env env, napi_callback_info info) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        napi_throw_error(env, NULL, "ioctl TIOCGWINSZ failed");
        return NULL;
    }
    
    napi_value result;
    napi_create_object(env, &result);
    
    napi_value rows, cols;
    napi_create_uint32(env, ws.ws_row, &rows);
    napi_create_uint32(env, ws.ws_col, &cols);
    
    napi_set_named_property(env, result, "rows", rows);
    napi_set_named_property(env, result, "cols", cols);
    
    return result;
}

/**
 * Move cursor to position
 */
static napi_value move_cursor(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    uint32_t row, col;
    napi_get_value_uint32(env, argv[0], &row);
    napi_get_value_uint32(env, argv[1], &col);
    
    // ANSI escape: ESC[row;colH
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
    write(STDOUT_FILENO, buf, len);
    
    return get_undefined(env);
}

/**
 * Set foreground color (24-bit RGB)
 */
static napi_value set_fg_color(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value argv[3];
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    
    uint32_t r, g, b;
    napi_get_value_uint32(env, argv[0], &r);
    napi_get_value_uint32(env, argv[1], &g);
    napi_get_value_uint32(env, argv[2], &b);
    
    // ANSI 24-bit color: ESC[38;2;r;g;bm
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\x1b[38;2;%d;%d;%dm", r, g, b);
    write(STDOUT_FILENO, buf, len);
    
    return get_undefined(env);
}

/**
 * Clear screen
 */
static napi_value clear_screen(napi_env env, napi_callback_info info) {
    // Clear entire screen and move cursor home
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
    return get_undefined(env);
}

/**
 * Read single keypress (requires raw mode)
 */
static napi_value read_key(napi_env env, napi_callback_info info) {
    char c;
    ssize_t nread = read(STDIN_FILENO, &c, 1);
    
    if (nread == -1) {
        napi_throw_error(env, NULL, "read failed");
        return NULL;
    }
    
    if (nread == 0) {
        return get_null(env);
    }
    
    // Check for escape sequence
    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            // Just escape key
            return create_key_object(env, "escape", 27, false, false);
        }
        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return create_key_object(env, "escape", 27, false, false);
        }
        
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return create_key_object(env, "up", 0, false, false);
                case 'B': return create_key_object(env, "down", 0, false, false);
                case 'C': return create_key_object(env, "right", 0, false, false);
                case 'D': return create_key_object(env, "left", 0, false, false);
                case 'H': return create_key_object(env, "home", 0, false, false);
                case 'F': return create_key_object(env, "end", 0, false, false);
            }
        }
    }
    
    // Control character detection
    bool ctrl = (c >= 1 && c <= 26);
    
    napi_value result;
    if (ctrl) {
        char name[2] = { 'a' + c - 1, '\0' };
        result = create_key_object(env, name, c, true, false);
    } else {
        char name[2] = { c, '\0' };
        result = create_key_object(env, name, c, false, false);
    }
    
    return result;
}
```

### Windows Support

Windows doesn't have termios - uses Console API instead:

```c
#ifdef _WIN32
#include <windows.h>
#include <conio.h>

static DWORD original_mode;

static napi_value enable_raw_mode(napi_env env, napi_callback_info info) {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &original_mode);
    
    DWORD raw_mode = original_mode;
    raw_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    raw_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    
    SetConsoleMode(hStdin, raw_mode);
    
    // Enable ANSI on Windows 10+
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD out_mode;
    GetConsoleMode(hStdout, &out_mode);
    SetConsoleMode(hStdout, out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    
    return get_undefined(env);
}
#endif
```

---

## TypeScript API

```typescript
// src/ts/index.ts

export interface KeyPress {
  name: string;        // 'a', 'enter', 'up', 'escape', etc.
  code: number;        // ASCII/keycode
  ctrl: boolean;       // Ctrl modifier
  shift: boolean;      // Shift modifier
  alt: boolean;        // Alt modifier
  meta: boolean;       // Meta/Cmd modifier
}

export interface MouseEvent {
  x: number;
  y: number;
  button: 'left' | 'right' | 'middle' | 'none';
  action: 'press' | 'release' | 'move' | 'scroll';
  scrollDelta?: number;
}

export interface TerminalSize {
  rows: number;
  cols: number;
}

export class Terminal {
  private isRaw = false;
  
  /**
   * Enter raw mode - key presses are available immediately
   */
  raw(): this {
    native.enableRawMode();
    this.isRaw = true;
    return this;
  }
  
  /**
   * Restore normal terminal mode
   */
  restore(): this {
    native.disableRawMode();
    this.isRaw = false;
    return this;
  }
  
  /**
   * Clear entire screen
   */
  clear(): this {
    native.clearScreen();
    return this;
  }
  
  /**
   * Move cursor to position (1-indexed)
   */
  moveTo(row: number, col: number): this {
    native.moveCursor(row, col);
    return this;
  }
  
  /**
   * Set foreground color (24-bit RGB)
   */
  setColor(r: number, g: number, b: number): this {
    native.setFgColor(r, g, b);
    return this;
  }
  
  /**
   * Set background color (24-bit RGB)
   */
  setBgColor(r: number, g: number, b: number): this {
    native.setBgColor(r, g, b);
    return this;
  }
  
  /**
   * Reset all attributes
   */
  reset(): this {
    native.resetAttributes();
    return this;
  }
  
  /**
   * Write text at current cursor position
   */
  write(text: string): this {
    process.stdout.write(text);
    return this;
  }
  
  /**
   * Get terminal dimensions
   */
  get size(): TerminalSize {
    return native.getWindowSize();
  }
  
  /**
   * Read single keypress (async, requires raw mode)
   */
  async readKey(): Promise<KeyPress> {
    if (!this.isRaw) {
      throw new Error('Must be in raw mode to read keys. Call terminal.raw() first.');
    }
    return native.readKey();
  }
  
  /**
   * Hide cursor
   */
  hideCursor(): this {
    process.stdout.write('\x1b[?25l');
    return this;
  }
  
  /**
   * Show cursor
   */
  showCursor(): this {
    process.stdout.write('\x1b[?25h');
    return this;
  }
  
  /**
   * Switch to alternate screen buffer (like vim does)
   */
  alternateScreen(): this {
    process.stdout.write('\x1b[?1049h');
    return this;
  }
  
  /**
   * Switch back to main screen buffer
   */
  mainScreen(): this {
    process.stdout.write('\x1b[?1049l');
    return this;
  }
  
  /**
   * Enable mouse tracking
   */
  enableMouse(): this {
    process.stdout.write('\x1b[?1000h\x1b[?1006h');
    return this;
  }
  
  /**
   * Disable mouse tracking
   */
  disableMouse(): this {
    process.stdout.write('\x1b[?1000l\x1b[?1006l');
    return this;
  }
}

// Convenience exports
export const cursor = {
  move: (row: number, col: number) => native.moveCursor(row, col),
  hide: () => process.stdout.write('\x1b[?25l'),
  show: () => process.stdout.write('\x1b[?25h'),
  save: () => process.stdout.write('\x1b[s'),
  restore: () => process.stdout.write('\x1b[u'),
};

export const color = {
  rgb: (r: number, g: number, b: number) => native.setFgColor(r, g, b),
  bgRgb: (r: number, g: number, b: number) => native.setBgColor(r, g, b),
  reset: () => native.resetAttributes(),
  
  // Named colors (Zorya palette)
  zoryaPink: () => native.setFgColor(255, 0, 110),
  zoryaBlue: () => native.setFgColor(0, 180, 255),
};

export const screen = {
  clear: () => native.clearScreen(),
  size: () => native.getWindowSize(),
  alternate: () => process.stdout.write('\x1b[?1049h'),
  main: () => process.stdout.write('\x1b[?1049l'),
};
```

---

## Example: Simple TUI App

```typescript
import { Terminal, color } from '@aspect/termios';

async function main() {
  const term = new Terminal();
  
  // Switch to alternate buffer (preserves scrollback)
  term.alternateScreen();
  term.hideCursor();
  term.raw();
  
  // Draw UI
  term.clear();
  term.moveTo(1, 1);
  color.zoryaPink();
  term.write('╔══════════════════════════════════════╗');
  term.moveTo(2, 1);
  term.write('║     Welcome to Zorya Terminal       ║');
  term.moveTo(3, 1);
  term.write('╚══════════════════════════════════════╝');
  
  color.reset();
  term.moveTo(5, 1);
  term.write('Press any key... (q to quit)');
  
  // Event loop
  while (true) {
    const key = await term.readKey();
    
    term.moveTo(7, 1);
    term.write(`Key pressed: ${key.name} (code: ${key.code})   `);
    
    if (key.name === 'q' || (key.ctrl && key.name === 'c')) {
      break;
    }
  }
  
  // Cleanup
  term.showCursor();
  term.restore();
  term.mainScreen();
  
  console.log('Goodbye!');
}

main().catch(console.error);
```

---

## Use Cases Enabled

### 1. Terminal Zero Game
Your cyberpunk terminal game - true raw mode, smooth animations, real input handling.

### 2. CLI Tools
Beautiful, responsive command-line tools that feel native.

### 3. Log Viewers
Real-time log viewing with color and filtering.

### 4. System Monitors
htop-style dashboards in pure TSX.

### 5. Interactive Installers
npm-style spinners and progress bars that actually work.

---

## Timeline

| Day | Task |
|-----|------|
| 1 | Scaffold project, basic termios bindings |
| 2-3 | Raw mode + key reading (Linux) |
| 4 | Cursor control + colors |
| 5 | Windows Console API port |
| 6 | TypeScript wrapper |
| 7 | Mouse support |
| 8-9 | Tests + examples |
| 10 | Documentation |

**Total:** ~2 weeks for production-ready @aspect/termios

---

## Success Metrics

- [ ] Raw mode works on Linux/macOS/Windows
- [ ] Key reading is instant (< 1ms latency)
- [ ] 24-bit color verified on modern terminals
- [ ] Arrow keys / function keys parsed correctly
- [ ] Mouse tracking works
- [ ] Clean restoration on exit (no broken terminals)
- [ ] Example TUI app runs smoothly
