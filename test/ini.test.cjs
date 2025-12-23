/**
 * @file ini.test.cjs
 * @brief Test suite for Pulsar INI module
 */

'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const os = require('os');

/* Load native module */
const native = require('../lib/native/pulsar_ini.node');

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

console.log('\n Pulsar INI Tests\n');

/* Context Creation */
console.log(' Context Lifecycle\n');

test('create returns context', () => {
    const ctx = native.create();
    assert(ctx !== null);
    native.free(ctx);
});

/* Loading */
console.log('\n Loading\n');

test('loadString parses INI', () => {
    const ctx = native.create();
    native.loadString(ctx, '[section]\nkey=value');
    const val = native.get(ctx, 'section.key');
    assert.strictEqual(val, 'value');
    native.free(ctx);
});

test('loadString handles multiple sections', () => {
    const ctx = native.create();
    native.loadString(ctx, '[a]\nx=1\n[b]\ny=2');
    assert.strictEqual(native.get(ctx, 'a.x'), '1');
    assert.strictEqual(native.get(ctx, 'b.y'), '2');
    native.free(ctx);
});

test('load reads file', () => {
    const tmpFile = path.join(os.tmpdir(), 'pulsar_test.ini');
    fs.writeFileSync(tmpFile, '[test]\nfoo=bar');
    const ctx = native.create();
    native.load(ctx, tmpFile);
    assert.strictEqual(native.get(ctx, 'test.foo'), 'bar');
    native.free(ctx);
    fs.unlinkSync(tmpFile);
});

/* Getters */
console.log('\n Getters\n');

test('get returns string', () => {
    const ctx = native.create();
    native.loadString(ctx, '[s]\nk=hello');
    const val = native.get(ctx, 's.k');
    assert.strictEqual(val, 'hello');
    native.free(ctx);
});

test('getDefault returns default for missing', () => {
    const ctx = native.create();
    native.loadString(ctx, '[s]\nk=v');
    const val = native.getDefault(ctx, 's.missing', 'default');
    assert.strictEqual(val, 'default');
    native.free(ctx);
});

test('getInt returns number', () => {
    const ctx = native.create();
    native.loadString(ctx, '[s]\nnum=42');
    const val = native.getInt(ctx, 's.num');
    assert.strictEqual(val, 42);
    native.free(ctx);
});

test('getFloat returns number', () => {
    const ctx = native.create();
    native.loadString(ctx, '[s]\npi=3.14');
    const val = native.getFloat(ctx, 's.pi');
    assert(Math.abs(val - 3.14) < 0.001);
    native.free(ctx);
});

test('getBool returns boolean', () => {
    const ctx = native.create();
    native.loadString(ctx, '[s]\nenabled=true\ndisabled=false');
    assert.strictEqual(native.getBool(ctx, 's.enabled'), true);
    assert.strictEqual(native.getBool(ctx, 's.disabled'), false);
    native.free(ctx);
});

test('getArray returns array', () => {
    const ctx = native.create();
    // ZORYA-INI uses pipe (|) as array delimiter
    native.loadString(ctx, '[s]\nitems=a|b|c');
    const arr = native.getArray(ctx, 's.items');
    assert.deepStrictEqual(arr, ['a', 'b', 'c']);
    native.free(ctx);
});

/* Has */
console.log('\n Has\n');

test('has returns true for existing', () => {
    const ctx = native.create();
    native.loadString(ctx, '[s]\nk=v');
    assert.strictEqual(native.has(ctx, 's.k'), true);
    native.free(ctx);
});

test('has returns false for missing', () => {
    const ctx = native.create();
    native.loadString(ctx, '[s]\nk=v');
    assert.strictEqual(native.has(ctx, 's.missing'), false);
    native.free(ctx);
});

/* Set */
console.log('\n Set\n');

test('set creates key', () => {
    const ctx = native.create();
    native.set(ctx, 's.k', 'value');
    assert.strictEqual(native.get(ctx, 's.k'), 'value');
    native.free(ctx);
});

test('set overwrites existing', () => {
    const ctx = native.create();
    native.loadString(ctx, '[s]\nk=old');
    native.set(ctx, 's.k', 'new');
    assert.strictEqual(native.get(ctx, 's.k'), 'new');
    native.free(ctx);
});

/* Serialization */
console.log('\n Serialization\n');

test('toString returns string', () => {
    const ctx = native.create();
    native.loadString(ctx, '[section]\nkey=value');
    const str = native.toString(ctx);
    // Note: Full serialization pending - returns placeholder
    assert.strictEqual(typeof str, 'string');
    assert(str.length > 0);
    native.free(ctx);
});

test('save throws (not yet implemented)', () => {
    const tmpFile = path.join(os.tmpdir(), 'pulsar_save_test.ini');
    const ctx = native.create();
    native.set(ctx, 'test.key', 'value');
    // save() is not yet implemented in zorya_ini.c
    assert.throws(() => native.save(ctx, tmpFile));
    native.free(ctx);
});

/* Sections */
console.log('\n Sections\n');

test('sections returns list', () => {
    const ctx = native.create();
    native.loadString(ctx, '[a]\nx=1\n[b]\ny=2');
    const sections = native.sections(ctx);
    assert(sections.includes('a'));
    assert(sections.includes('b'));
    native.free(ctx);
});

/* Version */
console.log('\n Version\n');

test('version returns string', () => {
    assert.strictEqual(typeof native.version, 'string');
});

/* Summary */
console.log('\n' + '─'.repeat(40));
console.log(`\n Results: ${passCount}/${testCount} tests passed\n`);

if (passCount === testCount) {
    console.log(' All INI tests passed!\n');
    process.exit(0);
} else {
    process.exit(1);
}
