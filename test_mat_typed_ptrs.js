import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing Mat typed pointer accessors...\n');

// Test 1: ucharPtr on CV_8UC1
console.log('Test 1: ucharPtr on CV_8UC1');
const mat1 = new cv.Mat(3, 4, cv.CV_8UC1);
mat1.set(0, 0, 10);
mat1.set(0, 1, 20);
mat1.set(0, 2, 30);
mat1.set(0, 3, 40);
mat1.set(1, 0, 50);
mat1.set(1, 1, 60);
mat1.set(1, 2, 70);
mat1.set(1, 3, 80);
mat1.set(2, 0, 90);
mat1.set(2, 1, 100);
mat1.set(2, 2, 110);
mat1.set(2, 3, 120);

const row0 = mat1.ucharPtr(0);
const row1 = mat1.ucharPtr(1);
const row2 = mat1.ucharPtr(2);

console.log('  row0:', Array.from(row0));
console.log('  row1:', Array.from(row1));
console.log('  row2:', Array.from(row2));

if (row0.length !== 4 || row0[0] !== 10 || row0[3] !== 40) {
    throw new Error('ucharPtr row 0 failed');
}
if (row1.length !== 4 || row1[0] !== 50 || row1[3] !== 80) {
    throw new Error('ucharPtr row 1 failed');
}
if (row2.length !== 4 || row2[0] !== 90 || row2[3] !== 120) {
    throw new Error('ucharPtr row 2 failed');
}
console.log('  ✓ Test 1 passed\n');

// Test 2: intPtr on CV_32SC1
console.log('Test 2: intPtr on CV_32SC1');
const mat2 = new cv.Mat(2, 3, cv.CV_32SC1);
mat2.set(0, 0, -100);
mat2.set(0, 1, 200);
mat2.set(0, 2, -300);
mat2.set(1, 0, 400);
mat2.set(1, 1, -500);
mat2.set(1, 2, 600);

const intRow0 = mat2.intPtr(0);
const intRow1 = mat2.intPtr(1);

console.log('  intRow0:', Array.from(intRow0));
console.log('  intRow1:', Array.from(intRow1));

if (intRow0.length !== 3 || intRow0[0] !== -100 || intRow0[2] !== -300) {
    throw new Error('intPtr row 0 failed');
}
if (intRow1.length !== 3 || intRow1[0] !== 400 || intRow1[2] !== 600) {
    throw new Error('intPtr row 1 failed');
}
console.log('  ✓ Test 2 passed\n');

// Test 3: floatPtr on CV_32FC1
console.log('Test 3: floatPtr on CV_32FC1');
const mat3 = new cv.Mat(2, 2, cv.CV_32FC1);
mat3.set(0, 0, 1.5);
mat3.set(0, 1, 2.5);
mat3.set(1, 0, 3.5);
mat3.set(1, 1, 4.5);

const floatRow0 = mat3.floatPtr(0);
const floatRow1 = mat3.floatPtr(1);

console.log('  floatRow0:', Array.from(floatRow0));
console.log('  floatRow1:', Array.from(floatRow1));

if (floatRow0.length !== 2 || Math.abs(floatRow0[0] - 1.5) > 0.001 || Math.abs(floatRow0[1] - 2.5) > 0.001) {
    throw new Error('floatPtr row 0 failed');
}
if (floatRow1.length !== 2 || Math.abs(floatRow1[0] - 3.5) > 0.001 || Math.abs(floatRow1[1] - 4.5) > 0.001) {
    throw new Error('floatPtr row 1 failed');
}
console.log('  ✓ Test 3 passed\n');

// Test 4: doublePtr on CV_64FC1
console.log('Test 4: doublePtr on CV_64FC1');
const mat4 = new cv.Mat(2, 2, cv.CV_64FC1);
mat4.set(0, 0, 1.23456789);
mat4.set(0, 1, 2.34567890);
mat4.set(1, 0, 3.45678901);
mat4.set(1, 1, 4.56789012);

const doubleRow0 = mat4.doublePtr(0);
const doubleRow1 = mat4.doublePtr(1);

console.log('  doubleRow0:', Array.from(doubleRow0));
console.log('  doubleRow1:', Array.from(doubleRow1));

if (doubleRow0.length !== 2 || Math.abs(doubleRow0[0] - 1.23456789) > 0.0000001) {
    throw new Error('doublePtr row 0 failed');
}
if (doubleRow1.length !== 2 || Math.abs(doubleRow1[0] - 3.45678901) > 0.0000001) {
    throw new Error('doublePtr row 1 failed');
}
console.log('  ✓ Test 4 passed\n');

console.log('✓✓✓ All typed pointer accessor tests passed! ✓✓✓');
