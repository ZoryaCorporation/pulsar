/**
 * @file dagger.test.cjs
 * @brief Test suite for Pulsar Dagger module (Hash Table)
 */

'use strict';

const assert = require('assert');

/* Load native module */
const native = require('../lib/native/pulsar_dagger.node');

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

console.log('\n Pulsar Dagger (Hash Table) Tests\n');

/* Creation */
console.log(' Creation\n');

test('DaggerTable creates instance', () => {
    const table = new native.DaggerTable();
    assert(table !== null);
});

test('new table is empty', () => {
    const table = new native.DaggerTable();
    assert.strictEqual(table.size, 0);
});

/* Set/Get */
console.log('\n Set/Get\n');

test('set and get string value', () => {
    const table = new native.DaggerTable();
    table.set('key', 'value');
    assert.strictEqual(table.get('key'), 'value');
});

test('set and get number value', () => {
    const table = new native.DaggerTable();
    table.set('num', 42);
    assert.strictEqual(table.get('num'), 42);
});

test('get missing key returns undefined', () => {
    const table = new native.DaggerTable();
    assert.strictEqual(table.get('missing'), undefined);
});

test('set overwrites existing key', () => {
    const table = new native.DaggerTable();
    table.set('key', 'first');
    table.set('key', 'second');
    assert.strictEqual(table.get('key'), 'second');
});

/* Has/Delete */
console.log('\n Has/Delete\n');

test('has returns true for existing key', () => {
    const table = new native.DaggerTable();
    table.set('key', 'value');
    assert.strictEqual(table.has('key'), true);
});

test('has returns false for missing key', () => {
    const table = new native.DaggerTable();
    assert.strictEqual(table.has('missing'), false);
});

test('delete removes key', () => {
    const table = new native.DaggerTable();
    table.set('key', 'value');
    table.delete('key');
    assert.strictEqual(table.has('key'), false);
});

test('delete returns true for existing key', () => {
    const table = new native.DaggerTable();
    table.set('key', 'value');
    assert.strictEqual(table.delete('key'), true);
});

test('delete returns false for missing key', () => {
    const table = new native.DaggerTable();
    assert.strictEqual(table.delete('missing'), false);
});

/* Size */
console.log('\n Size/Clear\n');

test('size increases with set', () => {
    const table = new native.DaggerTable();
    table.set('a', 1);
    table.set('b', 2);
    table.set('c', 3);
    assert.strictEqual(table.size, 3);
});

test('size decreases with delete', () => {
    const table = new native.DaggerTable();
    table.set('a', 1);
    table.set('b', 2);
    table.delete('a');
    assert.strictEqual(table.size, 1);
});

test('clear empties table', () => {
    const table = new native.DaggerTable();
    table.set('a', 1);
    table.set('b', 2);
    table.clear();
    assert.strictEqual(table.size, 0);
});

/* Iteration */
console.log('\n Iteration\n');

test('keys returns all keys', () => {
    const table = new native.DaggerTable();
    table.set('a', 1);
    table.set('b', 2);
    const keys = table.keys();
    assert.strictEqual(keys.length, 2);
    assert(keys.includes('a'));
    assert(keys.includes('b'));
});

test('values returns all values', () => {
    const table = new native.DaggerTable();
    table.set('a', 1);
    table.set('b', 2);
    const values = table.values();
    assert.strictEqual(values.length, 2);
    assert(values.includes(1));
    assert(values.includes(2));
});

test('entries returns key-value pairs', () => {
    const table = new native.DaggerTable();
    table.set('a', 1);
    const entries = table.entries();
    assert.strictEqual(entries.length, 1);
    assert.deepStrictEqual(entries[0], ['a', 1]);
});

test('forEach iterates all pairs', () => {
    const table = new native.DaggerTable();
    table.set('a', 1);
    table.set('b', 2);
    const seen = [];
    table.forEach((value, key) => {
        seen.push([key, value]);
    });
    assert.strictEqual(seen.length, 2);
});

/* Stats */
console.log('\n Stats\n');

test('stats returns info', () => {
    const table = new native.DaggerTable();
    table.set('a', 1);
    const stats = table.stats();
    assert.strictEqual(typeof stats.count, 'number');
    assert.strictEqual(typeof stats.capacity, 'number');
    assert.strictEqual(typeof stats.loadFactor, 'number');
});

test('capacity returns number', () => {
    const table = new native.DaggerTable();
    assert.strictEqual(typeof table.capacity, 'number');
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
    console.log(' All dagger tests passed!\n');
    process.exit(0);
} else {
    process.exit(1);
}
