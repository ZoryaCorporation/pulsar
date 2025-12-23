/**
 * @file run-all.cjs
 * @brief Run all Pulsar tests
 */

'use strict';

const { execSync } = require('child_process');
const path = require('path');

const TESTS = [
    'weave.test.cjs',
    'compress.test.cjs',
    'fileops.test.cjs',
    'hash.test.cjs',
    'dagger.test.cjs',
    'pcm.test.cjs',
    'ini.test.cjs',
];

console.log('\n  PULSAR Test Suite v1.0.0');
console.log('  ' + '-'.repeat(40));

let passed = 0;
let failed = 0;

for (const test of TESTS) {
    const testPath = path.join(__dirname, test);
    console.log(`\n  Running ${test}...`);
    
    try {
        execSync(`node "${testPath}"`, { 
            stdio: 'inherit',
            cwd: path.join(__dirname, '..')
        });
        passed++;
    } catch {
        failed++;
    }
}

console.log('\n  ' + '-'.repeat(40));
console.log(`
  Results
    Passed: ${passed}
    Failed: ${failed}
    Total:  ${TESTS.length}
`);

if (failed === 0) {
    console.log('  All tests passed.\n');
    process.exit(0);
} else {
    console.log('  Some tests failed.\n');
    process.exit(1);
}
