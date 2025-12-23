/**
 * @file compress.test.cjs
 * @brief Test suite for Pulsar Compress module
 */

'use strict';

const assert = require('assert');

/* Load native module directly */
const native = require('../lib/native/pulsar_compress.node');

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

console.log('\n Pulsar Compress Tests\n');

/* Test data */
const testData = Buffer.from('Hello, Pulsar! This is test data for compression. '.repeat(100));

/* Zstd Tests */
console.log(' Zstandard (zstd)\n');

test('zstd_compress returns buffer', () => {
    const compressed = native.zstdCompress(testData, 3);
    assert(Buffer.isBuffer(compressed));
});

test('zstd_compress reduces size', () => {
    const compressed = native.zstdCompress(testData, 3);
    assert(compressed.length < testData.length);
});

test('zstd roundtrip preserves data', () => {
    const compressed = native.zstdCompress(testData, 3);
    const decompressed = native.zstdDecompress(compressed);
    assert(decompressed.equals(testData));
});

test('zstd higher levels compress better', () => {
    const level3 = native.zstdCompress(testData, 3);
    const level19 = native.zstdCompress(testData, 19);
    assert(level19.length <= level3.length);
});

test('zstdCompressBound returns valid size', () => {
    const bound = native.zstdCompressBound(1000);
    assert.strictEqual(typeof bound, 'number');
    assert(bound >= 1000);
});

/* LZ4 Tests */
console.log('\n LZ4\n');

test('lz4_compress returns buffer', () => {
    const compressed = native.lz4Compress(testData);
    assert(Buffer.isBuffer(compressed));
});

test('lz4_compress reduces size', () => {
    const compressed = native.lz4Compress(testData);
    assert(compressed.length < testData.length);
});

test('lz4 roundtrip preserves data', () => {
    const compressed = native.lz4Compress(testData);
    const decompressed = native.lz4Decompress(compressed, testData.length);
    assert(decompressed.equals(testData));
});

test('lz4_compress_hc compresses better', () => {
    const regular = native.lz4Compress(testData);
    const hc = native.lz4CompressHC(testData, 12);
    assert(hc.length <= regular.length);
});

test('lz4_compress_hc roundtrip works', () => {
    const compressed = native.lz4CompressHC(testData, 9);
    const decompressed = native.lz4Decompress(compressed, testData.length);
    assert(decompressed.equals(testData));
});

/* Version */
console.log('\n Version\n');

test('version returns string', () => {
    const v = native.version();
    assert.strictEqual(typeof v, 'string');
});

/* Summary */
console.log('\n' + '─'.repeat(40));
console.log(`\n Results: ${passCount}/${testCount} tests passed\n`);

if (passCount === testCount) {
    console.log('All compress tests passed!\n');
    process.exit(0);
} else {
    process.exit(1);
}
