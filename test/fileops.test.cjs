/**
 * @file fileops.test.cjs
 * @brief Test suite for Pulsar FileOps module
 */

'use strict';

const assert = require('assert');
const path = require('path');
const os = require('os');

/* Load native module directly */
const native = require('../lib/native/pulsar_fileops.node');

const TEST_DIR = path.join(os.tmpdir(), 'pulsar-fileops-test-' + process.pid);
let testCount = 0;
let passCount = 0;

function test(name, fn) {
    testCount++;
    try {
        fn();
        passCount++;
        console.log(`   ${name}`);
    } catch (err) {
        console.log(`   ${name}`);
        console.log(`     ${err.message}`);
    }
}

function setup() {
    try { native.removeRecursive(TEST_DIR); } catch { /* ignore */ }
    native.mkdir(TEST_DIR, true);
}

function cleanup() {
    try { native.removeRecursive(TEST_DIR); } catch { /* ignore */ }
}

console.log('\nPulsar FileOps Tests\n');
setup();

/* System Paths */
console.log('System Paths\n');
test('cwd returns string', () => {
    const cwd = native.cwd();
    assert.strictEqual(typeof cwd, 'string');
    assert(cwd.length > 0);
});

test('homedir returns home', () => {
    const home = native.homedir();
    assert.strictEqual(home, os.homedir());
});

test('tmpdir returns string', () => {
    const tmp = native.tmpdir();
    assert.strictEqual(typeof tmp, 'string');
});

/* Path Operations */
console.log('\n Path Operations\n');

test('basename extracts filename', () => {
    assert.strictEqual(native.basename('/foo/bar/baz.txt'), 'baz.txt');
});

test('dirname extracts directory', () => {
    assert.strictEqual(native.dirname('/foo/bar/baz.txt'), '/foo/bar');
});

test('extname extracts extension', () => {
    assert.strictEqual(native.extname('/foo/bar.txt'), '.txt');
});

test('join combines paths', () => {
    assert.strictEqual(native.join('/foo', 'bar'), '/foo/bar');
});

test('normalize resolves . and ..', () => {
    assert.strictEqual(native.normalize('/foo/bar/../baz'), '/foo/baz');
});

/* File Operations */
console.log('\n File Operations\n');

test('writeFile creates file', () => {
    const file = path.join(TEST_DIR, 'test.txt');
    native.writeFile(file, Buffer.from('Hello'));
    assert(native.exists(file));
});

test('readFile reads content', () => {
    const file = path.join(TEST_DIR, 'read.txt');
    native.writeFile(file, Buffer.from('Content'));
    const data = native.readFile(file);
    assert.strictEqual(data.toString(), 'Content');
});

test('appendFile appends data', () => {
    const file = path.join(TEST_DIR, 'append.txt');
    native.writeFile(file, Buffer.from('A'));
    native.appendFile(file, Buffer.from('B'));
    assert.strictEqual(native.readFile(file).toString(), 'AB');
});

test('copyFile copies file', () => {
    const src = path.join(TEST_DIR, 'src.txt');
    const dst = path.join(TEST_DIR, 'dst.txt');
    native.writeFile(src, Buffer.from('Copy me'));
    native.copyFile(src, dst);
    assert.strictEqual(native.readFile(dst).toString(), 'Copy me');
});

test('moveFile moves file', () => {
    const src = path.join(TEST_DIR, 'move-src.txt');
    const dst = path.join(TEST_DIR, 'move-dst.txt');
    native.writeFile(src, Buffer.from('Move me'));
    native.moveFile(src, dst);
    assert(!native.exists(src));
    assert.strictEqual(native.readFile(dst).toString(), 'Move me');
});

test('remove deletes file', () => {
    const file = path.join(TEST_DIR, 'del.txt');
    native.writeFile(file, Buffer.from('x'));
    native.remove(file);
    assert(!native.exists(file));
});

/* Stat Operations */
console.log('\n Stat Operations\n');

test('exists returns true for existing', () => {
    assert(native.exists(TEST_DIR));
});

test('exists returns false for missing', () => {
    assert(!native.exists('/nonexistent/path/12345'));
});

test('isFile detects files', () => {
    const file = path.join(TEST_DIR, 'isfile.txt');
    native.writeFile(file, Buffer.from('x'));
    assert(native.isFile(file));
    assert(!native.isFile(TEST_DIR));
});

test('isDirectory detects directories', () => {
    assert(native.isDirectory(TEST_DIR));
});

test('fileSize returns correct size', () => {
    const file = path.join(TEST_DIR, 'size.txt');
    native.writeFile(file, Buffer.from('12345'));
    assert.strictEqual(native.fileSize(file), 5);
});

test('stat returns metadata', () => {
    const file = path.join(TEST_DIR, 'stat.txt');
    native.writeFile(file, Buffer.from('test'));
    const st = native.stat(file);
    assert.strictEqual(st.size, 4);
    assert(st.isFile);
});

/* Directory Operations */
console.log('\n Directory Operations\n');

test('mkdir creates directory', () => {
    const dir = path.join(TEST_DIR, 'newdir');
    native.mkdir(dir);
    assert(native.isDirectory(dir));
});

test('mkdir recursive creates nested', () => {
    const nested = path.join(TEST_DIR, 'a/b/c');
    native.mkdir(nested, true);
    assert(native.isDirectory(nested));
});

test('readdir lists contents', () => {
    const dir = path.join(TEST_DIR, 'listdir');
    native.mkdir(dir);
    native.writeFile(path.join(dir, 'a.txt'), Buffer.from('a'));
    native.writeFile(path.join(dir, 'b.txt'), Buffer.from('b'));
    const entries = native.readdir(dir);
    assert(entries.includes('a.txt'));
    assert(entries.includes('b.txt'));
});

test('removeRecursive removes tree', () => {
    const dir = path.join(TEST_DIR, 'rmtree');
    native.mkdir(path.join(dir, 'sub'), true);
    native.writeFile(path.join(dir, 'sub', 'f.txt'), Buffer.from('x'));
    native.removeRecursive(dir);
    assert(!native.exists(dir));
});

/* Symlinks */
console.log('\n Symlinks\n');

test('symlink creates link', () => {
    const target = path.join(TEST_DIR, 'link-target.txt');
    const link = path.join(TEST_DIR, 'link');
    native.writeFile(target, Buffer.from('target'));
    native.symlink(target, link);
    assert(native.isSymlink(link));
});

test('readlink reads target', () => {
    const target = path.join(TEST_DIR, 'readlink-target.txt');
    const link = path.join(TEST_DIR, 'readlink');
    native.writeFile(target, Buffer.from('x'));
    native.symlink(target, link);
    assert.strictEqual(native.readlink(link), target);
});

/* Glob */
console.log('\n Glob\n');

test('glob finds matches', () => {
    const dir = path.join(TEST_DIR, 'glob');
    native.mkdir(dir);
    native.writeFile(path.join(dir, 'a.txt'), Buffer.from('a'));
    native.writeFile(path.join(dir, 'b.txt'), Buffer.from('b'));
    const matches = native.glob(path.join(dir, '*.txt'));
    assert.strictEqual(matches.length, 2);
});

/* Watch */
console.log('\n Watch\n');

test('watch lifecycle works', () => {
    native.watchInit();
    const dir = path.join(TEST_DIR, 'watch');
    native.mkdir(dir);
    const wd = native.watchAdd(dir);
    assert(typeof wd === 'number');
    native.watchRemove(wd);
    native.watchClose();
});

/* Version */
console.log('\n Version\n');

test('version returns string', () => {
    const v = native.version();
    assert.strictEqual(typeof v, 'string');
});

/* Cleanup */
cleanup();

/* Summary */
console.log('\n' + '─'.repeat(40));
console.log(`\n Results: ${passCount}/${testCount} tests passed\n`);

if (passCount === testCount) {
    console.log('All fileops tests passed!\n');
    process.exit(0);
} else {
    process.exit(1);
}
