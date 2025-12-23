/**
 * @file weave.test.cjs
 * @brief Test suite for Pulsar Weave module (string primitives)
 */

'use strict';

const assert = require('assert');

/* Load native module */
const native = require('../lib/native/pulsar_weave.node');

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

console.log('\n Pulsar Weave Tests\n');

/* Weave (Mutable String Buffer) */
console.log(' Weave (String Buffer)\n');

test('weaveCreate returns handle', () => {
    const handle = native.weaveCreate();
    assert(handle !== null && handle !== undefined);
});

test('weaveCreate with initial string', () => {
    const handle = native.weaveCreate('hello');
    const str = native.weaveToString(handle);
    assert.strictEqual(str, 'hello');
});

test('weaveAppend adds text', () => {
    const handle = native.weaveCreate('hello');
    native.weaveAppend(handle, ' world');
    const str = native.weaveToString(handle);
    assert.strictEqual(str, 'hello world');
});

test('weavePrepend adds to front', () => {
    const handle = native.weaveCreate('world');
    native.weavePrepend(handle, 'hello ');
    const str = native.weaveToString(handle);
    assert.strictEqual(str, 'hello world');
});

test('weaveLength returns correct size', () => {
    const handle = native.weaveCreate('hello');
    const len = native.weaveLength(handle);
    assert.strictEqual(len, 5);
});

test('weaveFind locates substring', () => {
    const handle = native.weaveCreate('hello world');
    const pos = native.weaveFind(handle, 'world');
    assert.strictEqual(pos, 6);
});

test('weaveContains checks for substring', () => {
    const handle = native.weaveCreate('hello world');
    assert.strictEqual(native.weaveContains(handle, 'world'), 1);
    assert.strictEqual(native.weaveContains(handle, 'xyz'), 0);
});

test('weaveReplace replaces text', () => {
    const handle = native.weaveCreate('hello world');
    native.weaveReplace(handle, 'world', 'pulsar');
    const str = native.weaveToString(handle);
    assert.strictEqual(str, 'hello pulsar');
});

test('weaveTrim removes whitespace', () => {
    const handle = native.weaveCreate('  hello  ');
    native.weaveTrim(handle);
    const str = native.weaveToString(handle);
    assert.strictEqual(str, 'hello');
});

test('weaveSplit breaks into parts', () => {
    const handle = native.weaveCreate('a,b,c');
    const parts = native.weaveSplit(handle, ',');
    assert.deepStrictEqual(parts, ['a', 'b', 'c']);
});

/* Cord (Rope-like) */
console.log('\n Cord (Rope Structure)\n');

test('cordCreate returns handle', () => {
    const handle = native.cordCreate();
    assert(handle !== null && handle !== undefined);
});

test('cordAppend adds chunks', () => {
    const handle = native.cordCreate();
    native.cordAppend(handle, 'hello');
    native.cordAppend(handle, ' world');
    const str = native.cordToString(handle);
    assert.strictEqual(str, 'hello world');
});

test('cordLength returns total size', () => {
    const handle = native.cordCreate();
    native.cordAppend(handle, 'hello');
    native.cordAppend(handle, ' world');
    const len = native.cordLength(handle);
    assert.strictEqual(len, 11);
});

test('cordChunkCount returns chunks', () => {
    const handle = native.cordCreate();
    native.cordAppend(handle, 'a');
    native.cordAppend(handle, 'b');
    native.cordAppend(handle, 'c');
    const count = native.cordChunkCount(handle);
    assert.strictEqual(count, 3);
});

/* Tablet (String Interning) */
console.log('\n Tablet (String Interning)\n');

test('tabletCreate returns handle', () => {
    const handle = native.tabletCreate();
    assert(handle !== null && handle !== undefined);
});

test('tabletIntern returns ID', () => {
    const handle = native.tabletCreate();
    const id = native.tabletIntern(handle, 'hello');
    assert.strictEqual(typeof id, 'bigint');
});

test('tabletIntern same string same ID', () => {
    const handle = native.tabletCreate();
    const id1 = native.tabletIntern(handle, 'hello');
    const id2 = native.tabletIntern(handle, 'hello');
    assert.strictEqual(id1, id2);
});

test('tabletGet retrieves string', () => {
    const handle = native.tabletCreate();
    const id = native.tabletIntern(handle, 'hello');
    const str = native.tabletGet(handle, id);
    assert.strictEqual(str, 'hello');
});

test('tabletCount returns unique strings', () => {
    const handle = native.tabletCreate();
    native.tabletIntern(handle, 'a');
    native.tabletIntern(handle, 'b');
    native.tabletIntern(handle, 'a'); // duplicate
    const count = native.tabletCount(handle);
    assert.strictEqual(count, 2);
});

/* Arena (Bump Allocator) */
console.log('\n Arena (Bump Allocator)\n');

test('arenaCreate returns handle', () => {
    const handle = native.arenaCreate(4096);
    assert(handle !== null && handle !== undefined);
});

test('arenaAllocString allocates', () => {
    const handle = native.arenaCreate(4096);
    const id = native.arenaAllocString(handle, 'hello');
    assert.strictEqual(typeof id, 'bigint');
});

test('arenaGetString retrieves string', () => {
    const handle = native.arenaCreate(4096);
    const id = native.arenaAllocString(handle, 'hello');
    const str = native.arenaGetString(handle, id);
    assert.strictEqual(str, 'hello');
});

test('arenaStats returns info', () => {
    const handle = native.arenaCreate(4096);
    native.arenaAllocString(handle, 'hello');
    const stats = native.arenaStats(handle);
    assert.strictEqual(typeof stats.allocated, 'number');
    assert.strictEqual(typeof stats.capacity, 'number');
    assert.strictEqual(typeof stats.chunks, 'number');
    assert.strictEqual(typeof stats.utilization, 'number');
});

test('arenaReset clears allocations', () => {
    const handle = native.arenaCreate(4096);
    native.arenaAllocString(handle, 'hello');
    native.arenaReset(handle);
    const stats = native.arenaStats(handle);
    assert.strictEqual(stats.allocated, 0);
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
    console.log(' All weave tests passed!\n');
    process.exit(0);
} else {
    process.exit(1);
}
