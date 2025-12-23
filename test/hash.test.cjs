/**
 * @file hash.test.cjs
 * @brief Test suite for Pulsar Hash module (NXH)
 */

'use strict';

const assert = require('assert');

/* Load native module */
const native = require('../lib/native/pulsar_hash.node');

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

console.log('\n Pulsar Hash (NXH) Tests\n');

/* 64-bit Hash */
console.log(' NXH64\n');

test('nxh64 returns bigint', () => {
    const hash = native.nxh64(Buffer.from('hello'));
    assert.strictEqual(typeof hash, 'bigint');
});

test('nxh64 is deterministic', () => {
    const h1 = native.nxh64(Buffer.from('test'));
    const h2 = native.nxh64(Buffer.from('test'));
    assert.strictEqual(h1, h2);
});

test('nxh64 differs for different inputs', () => {
    const h1 = native.nxh64(Buffer.from('hello'));
    const h2 = native.nxh64(Buffer.from('world'));
    assert.notStrictEqual(h1, h2);
});

test('nxh64 with seed', () => {
    const h1 = native.nxh64(Buffer.from('test'), 1n);
    const h2 = native.nxh64(Buffer.from('test'), 2n);
    assert.notStrictEqual(h1, h2);
});

/* 32-bit Hash */
console.log('\n NXH32\n');

test('nxh32 returns number', () => {
    const hash = native.nxh32(Buffer.from('hello'));
    assert.strictEqual(typeof hash, 'number');
});

test('nxh32 is deterministic', () => {
    const h1 = native.nxh32(Buffer.from('test'));
    const h2 = native.nxh32(Buffer.from('test'));
    assert.strictEqual(h1, h2);
});

/* String Hashing */
console.log('\n String Hashing\n');

test('nxhString returns bigint', () => {
    const hash = native.nxhString('hello');
    assert.strictEqual(typeof hash, 'bigint');
});

test('nxhString is deterministic', () => {
    const h1 = native.nxhString('test');
    const h2 = native.nxhString('test');
    assert.strictEqual(h1, h2);
});

test('nxhString32 returns number', () => {
    const hash = native.nxhString32('hello');
    assert.strictEqual(typeof hash, 'number');
});

/* Integer Hashing */
console.log('\n Integer Hashing\n');

test('nxhInt64 returns bigint', () => {
    const hash = native.nxhInt64(12345n);
    assert.strictEqual(typeof hash, 'bigint');
});

test('nxhInt32 returns bigint', () => {
    const hash = native.nxhInt32(12345);
    assert.strictEqual(typeof hash, 'bigint');
});

/* Combine */
console.log('\n Hash Combining\n');

test('nxhCombine returns bigint', () => {
    const h1 = native.nxh64(Buffer.from('a'));
    const h2 = native.nxh64(Buffer.from('b'));
    const combined = native.nxhCombine(h1, h2);
    assert.strictEqual(typeof combined, 'bigint');
});

test('nxhCombine is deterministic', () => {
    const h1 = native.nxh64(Buffer.from('a'));
    const h2 = native.nxh64(Buffer.from('b'));
    const c1 = native.nxhCombine(h1, h2);
    const c2 = native.nxhCombine(h1, h2);
    assert.strictEqual(c1, c2);
});

/* Constants */
console.log('\n Constants\n');

test('SEED_DEFAULT is bigint', () => {
    assert.strictEqual(typeof native.SEED_DEFAULT, 'bigint');
});

test('SEED_ALT is bigint', () => {
    assert.strictEqual(typeof native.SEED_ALT, 'bigint');
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
    console.log(' All hash tests passed!\n');
    process.exit(0);
} else {
    process.exit(1);
}
