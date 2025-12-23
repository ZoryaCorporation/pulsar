/**
 * @file pcm.test.cjs
 * @brief Test suite for Pulsar PCM module (Bit Operations)
 */

'use strict';

const assert = require('assert');

/* Load native module */
const native = require('../lib/native/pulsar_pcm.node');

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

console.log('\n Pulsar PCM (Bit Operations) Tests\n');

/* Population Count */
console.log(' Population Count\n');

test('popcount32 counts bits', () => {
    assert.strictEqual(native.popcount32(0), 0);
    assert.strictEqual(native.popcount32(1), 1);
    assert.strictEqual(native.popcount32(0xFF), 8);
    assert.strictEqual(native.popcount32(0xFFFFFFFF), 32);
});

test('popcount64 counts bits', () => {
    assert.strictEqual(native.popcount64(0n), 0);
    assert.strictEqual(native.popcount64(1n), 1);
    assert.strictEqual(native.popcount64(0xFFn), 8);
    assert.strictEqual(native.popcount64(0xFFFFFFFFFFFFFFFFn), 64);
});

/* Count Leading Zeros */
console.log('\n Count Leading Zeros\n');

test('clz32 counts leading zeros', () => {
    assert.strictEqual(native.clz32(0), 32);
    assert.strictEqual(native.clz32(1), 31);
    assert.strictEqual(native.clz32(0x80000000), 0);
});

test('clz64 counts leading zeros', () => {
    assert.strictEqual(native.clz64(0n), 64);
    assert.strictEqual(native.clz64(1n), 63);
    assert.strictEqual(native.clz64(0x8000000000000000n), 0);
});

/* Count Trailing Zeros */
console.log('\n Count Trailing Zeros\n');

test('ctz32 counts trailing zeros', () => {
    assert.strictEqual(native.ctz32(1), 0);
    assert.strictEqual(native.ctz32(2), 1);
    assert.strictEqual(native.ctz32(8), 3);
    assert.strictEqual(native.ctz32(0x80000000), 31);
});

test('ctz64 counts trailing zeros', () => {
    assert.strictEqual(native.ctz64(1n), 0);
    assert.strictEqual(native.ctz64(2n), 1);
    assert.strictEqual(native.ctz64(0x8000000000000000n), 63);
});

/* Rotation */
console.log('\n Rotation\n');

test('rotl32 rotates left', () => {
    assert.strictEqual(native.rotl32(1, 1), 2);
    assert.strictEqual(native.rotl32(0x80000000, 1), 1);
});

test('rotr32 rotates right', () => {
    assert.strictEqual(native.rotr32(2, 1), 1);
    assert.strictEqual(native.rotr32(1, 1), 0x80000000);
});

test('rotl64 rotates left', () => {
    assert.strictEqual(native.rotl64(1n, 1), 2n);
});

test('rotr64 rotates right', () => {
    assert.strictEqual(native.rotr64(2n, 1), 1n);
});

/* Byte Swap */
console.log('\n Byte Swap\n');

test('bswap16 swaps bytes', () => {
    assert.strictEqual(native.bswap16(0x1234), 0x3412);
});

test('bswap32 swaps bytes', () => {
    assert.strictEqual(native.bswap32(0x12345678), 0x78563412);
});

test('bswap64 swaps bytes', () => {
    assert.strictEqual(native.bswap64(0x123456789ABCDEFn), 0xEFCDAB8967452301n);
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
    console.log(' All PCM tests passed!\n');
    process.exit(0);
} else {
    process.exit(1);
}
