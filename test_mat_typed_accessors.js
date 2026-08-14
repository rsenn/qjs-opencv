import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing Mat typed element accessors...\n');

// Test 1: ucharAt on CV_8UC1
console.log('Test 1: ucharAt on CV_8UC1');
const mat1 = new cv.Mat(2, 2, cv.CV_8UC1);
mat1.set(0, 0, 100);
mat1.set(0, 1, 150);
mat1.set(1, 0, 200);
mat1.set(1, 1, 250);

console.log('  mat1.ucharAt(0, 0):', mat1.ucharAt(0, 0));
console.log('  mat1.ucharAt(0, 1):', mat1.ucharAt(0, 1));
console.log('  mat1.ucharAt(1, 0):', mat1.ucharAt(1, 0));
console.log('  mat1.ucharAt(1, 1):', mat1.ucharAt(1, 1));

if (mat1.ucharAt(0, 0) !== 100 || mat1.ucharAt(0, 1) !== 150 ||
    mat1.ucharAt(1, 0) !== 200 || mat1.ucharAt(1, 1) !== 250) {
    throw new Error('ucharAt failed');
}
console.log('  ✓ Test 1 passed\n');

// Test 2: intAt on CV_32SC1
console.log('Test 2: intAt on CV_32SC1');
const mat2 = new cv.Mat(2, 2, cv.CV_32SC1);
mat2.set(0, 0, -1000);
mat2.set(0, 1, 2000);
mat2.set(1, 0, -3000);
mat2.set(1, 1, 4000);

console.log('  mat2.intAt(0, 0):', mat2.intAt(0, 0));
console.log('  mat2.intAt(0, 1):', mat2.intAt(0, 1));
console.log('  mat2.intAt(1, 0):', mat2.intAt(1, 0));
console.log('  mat2.intAt(1, 1):', mat2.intAt(1, 1));

if (mat2.intAt(0, 0) !== -1000 || mat2.intAt(0, 1) !== 2000 ||
    mat2.intAt(1, 0) !== -3000 || mat2.intAt(1, 1) !== 4000) {
    throw new Error('intAt failed');
}
console.log('  ✓ Test 2 passed\n');

// Test 3: floatAt on CV_32FC1
console.log('Test 3: floatAt on CV_32FC1');
const mat3 = new cv.Mat(2, 2, cv.CV_32FC1);
mat3.set(0, 0, 1.5);
mat3.set(0, 1, 2.5);
mat3.set(1, 0, 3.5);
mat3.set(1, 1, 4.5);

console.log('  mat3.floatAt(0, 0):', mat3.floatAt(0, 0));
console.log('  mat3.floatAt(0, 1):', mat3.floatAt(0, 1));
console.log('  mat3.floatAt(1, 0):', mat3.floatAt(1, 0));
console.log('  mat3.floatAt(1, 1):', mat3.floatAt(1, 1));

if (Math.abs(mat3.floatAt(0, 0) - 1.5) > 0.001 || Math.abs(mat3.floatAt(0, 1) - 2.5) > 0.001 ||
    Math.abs(mat3.floatAt(1, 0) - 3.5) > 0.001 || Math.abs(mat3.floatAt(1, 1) - 4.5) > 0.001) {
    throw new Error('floatAt failed');
}
console.log('  ✓ Test 3 passed\n');

// Test 4: doubleAt on CV_64FC1
console.log('Test 4: doubleAt on CV_64FC1');
const mat4 = new cv.Mat(2, 2, cv.CV_64FC1);
mat4.set(0, 0, 1.23456789);
mat4.set(0, 1, 2.34567890);
mat4.set(1, 0, 3.45678901);
mat4.set(1, 1, 4.56789012);

console.log('  mat4.doubleAt(0, 0):', mat4.doubleAt(0, 0));
console.log('  mat4.doubleAt(0, 1):', mat4.doubleAt(0, 1));
console.log('  mat4.doubleAt(1, 0):', mat4.doubleAt(1, 0));
console.log('  mat4.doubleAt(1, 1):', mat4.doubleAt(1, 1));

if (Math.abs(mat4.doubleAt(0, 0) - 1.23456789) > 0.0000001 || Math.abs(mat4.doubleAt(0, 1) - 2.34567890) > 0.0000001 ||
    Math.abs(mat4.doubleAt(1, 0) - 3.45678901) > 0.0000001 || Math.abs(mat4.doubleAt(1, 1) - 4.56789012) > 0.0000001) {
    throw new Error('doubleAt failed');
}
console.log('  ✓ Test 4 passed\n');

// Test 5: shortAt on CV_16SC1
console.log('Test 5: shortAt on CV_16SC1');
const mat5 = new cv.Mat(2, 2, cv.CV_16SC1);
mat5.set(0, 0, -100);
mat5.set(0, 1, 200);
mat5.set(1, 0, -300);
mat5.set(1, 1, 400);

console.log('  mat5.shortAt(0, 0):', mat5.shortAt(0, 0));
console.log('  mat5.shortAt(0, 1):', mat5.shortAt(0, 1));
console.log('  mat5.shortAt(1, 0):', mat5.shortAt(1, 0));
console.log('  mat5.shortAt(1, 1):', mat5.shortAt(1, 1));

if (mat5.shortAt(0, 0) !== -100 || mat5.shortAt(0, 1) !== 200 ||
    mat5.shortAt(1, 0) !== -300 || mat5.shortAt(1, 1) !== 400) {
    throw new Error('shortAt failed');
}
console.log('  ✓ Test 5 passed\n');

// Test 6: ushortAt on CV_16UC1
console.log('Test 6: ushortAt on CV_16UC1');
const mat6 = new cv.Mat(2, 2, cv.CV_16UC1);
mat6.set(0, 0, 1000);
mat6.set(0, 1, 2000);
mat6.set(1, 0, 3000);
mat6.set(1, 1, 4000);

console.log('  mat6.ushortAt(0, 0):', mat6.ushortAt(0, 0));
console.log('  mat6.ushortAt(0, 1):', mat6.ushortAt(0, 1));
console.log('  mat6.ushortAt(1, 0):', mat6.ushortAt(1, 0));
console.log('  mat6.ushortAt(1, 1):', mat6.ushortAt(1, 1));

if (mat6.ushortAt(0, 0) !== 1000 || mat6.ushortAt(0, 1) !== 2000 ||
    mat6.ushortAt(1, 0) !== 3000 || mat6.ushortAt(1, 1) !== 4000) {
    throw new Error('ushortAt failed');
}
console.log('  ✓ Test 6 passed\n');

console.log('✓✓✓ All typed element accessor tests passed! ✓✓✓');
