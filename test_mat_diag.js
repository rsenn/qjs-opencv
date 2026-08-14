import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing Mat.diag()...\n');

// Test 1: diag() with no arguments (main diagonal)
console.log('Test 1: diag() with no arguments');
const mat1 = new cv.Mat(3, 3, cv.CV_32F);
mat1.set(0, 0, 1.0);
mat1.set(0, 1, 2.0);
mat1.set(0, 2, 3.0);
mat1.set(1, 0, 4.0);
mat1.set(1, 1, 5.0);
mat1.set(1, 2, 6.0);
mat1.set(2, 0, 7.0);
mat1.set(2, 1, 8.0);
mat1.set(2, 2, 9.0);

const diag1 = mat1.diag();
console.log('  Original matrix: 3x3');
console.log('  diag() size:', diag1.rows, 'x', diag1.cols);
console.log('  diag() values:', diag1.at(0, 0), diag1.at(1, 0), diag1.at(2, 0));

if (diag1.rows !== 3 || diag1.cols !== 1) {
    throw new Error(`Expected 3x1, got ${diag1.rows}x${diag1.cols}`);
}
if (diag1.at(0, 0) !== 1.0 || diag1.at(1, 0) !== 5.0 || diag1.at(2, 0) !== 9.0) {
    throw new Error(`Expected [1, 5, 9], got [${diag1.at(0, 0)}, ${diag1.at(1, 0)}, ${diag1.at(2, 0)}]`);
}
console.log('  ✓ Test 1 passed\n');

// Test 2: diag(1) - upper diagonal
console.log('Test 2: diag(1) - upper diagonal');
const diag2 = mat1.diag(1);
console.log('  diag(1) size:', diag2.rows, 'x', diag2.cols);
console.log('  diag(1) values:', diag2.at(0, 0), diag2.at(1, 0));

if (diag2.rows !== 2 || diag2.cols !== 1) {
    throw new Error(`Expected 2x1, got ${diag2.rows}x${diag2.cols}`);
}
if (diag2.at(0, 0) !== 2.0 || diag2.at(1, 0) !== 6.0) {
    throw new Error(`Expected [2, 6], got [${diag2.at(0, 0)}, ${diag2.at(1, 0)}]`);
}
console.log('  ✓ Test 2 passed\n');

// Test 3: diag(-1) - lower diagonal
console.log('Test 3: diag(-1) - lower diagonal');
const diag3 = mat1.diag(-1);
console.log('  diag(-1) size:', diag3.rows, 'x', diag3.cols);
console.log('  diag(-1) values:', diag3.at(0, 0), diag3.at(1, 0));

if (diag3.rows !== 2 || diag3.cols !== 1) {
    throw new Error(`Expected 2x1, got ${diag3.rows}x${diag3.cols}`);
}
if (diag3.at(0, 0) !== 4.0 || diag3.at(1, 0) !== 8.0) {
    throw new Error(`Expected [4, 8], got [${diag3.at(0, 0)}, ${diag3.at(1, 0)}]`);
}
console.log('  ✓ Test 3 passed\n');

// Test 4: Non-square matrix
console.log('Test 4: Non-square matrix');
const mat2 = new cv.Mat(2, 4, cv.CV_32F);
mat2.set(0, 0, 1.0);
mat2.set(0, 1, 2.0);
mat2.set(0, 2, 3.0);
mat2.set(0, 3, 4.0);
mat2.set(1, 0, 5.0);
mat2.set(1, 1, 6.0);
mat2.set(1, 2, 7.0);
mat2.set(1, 3, 8.0);

const diag4 = mat2.diag();
console.log('  Original matrix: 2x4');
console.log('  diag() size:', diag4.rows, 'x', diag4.cols);
console.log('  diag() values:', diag4.at(0, 0), diag4.at(1, 0));

if (diag4.rows !== 2 || diag4.cols !== 1) {
    throw new Error(`Expected 2x1, got ${diag4.rows}x${diag4.cols}`);
}
if (diag4.at(0, 0) !== 1.0 || diag4.at(1, 0) !== 6.0) {
    throw new Error(`Expected [1, 6], got [${diag4.at(0, 0)}, ${diag4.at(1, 0)}]`);
}
console.log('  ✓ Test 4 passed\n');

console.log('✓✓✓ All diag() tests passed! ✓✓✓');
