import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing Point and Size 1-arg constructors...\n');

// Test Point 1-arg constructor
console.log('Test 1: Point(x) constructor');
const p1 = new cv.Point(5);
console.log('  Point(5):', p1.x, p1.y);
if (p1.x !== 5 || p1.y !== 0) {
    throw new Error(`Point(5) failed: expected (5, 0), got (${p1.x}, ${p1.y})`);
}
console.log('  ✓ Point(x) works\n');

// Test Point 2-arg constructor
console.log('Test 2: Point(x, y) constructor');
const p2 = new cv.Point(3, 7);
console.log('  Point(3, 7):', p2.x, p2.y);
if (p2.x !== 3 || p2.y !== 7) {
    throw new Error(`Point(3, 7) failed: expected (3, 7), got (${p2.x}, ${p2.y})`);
}
console.log('  ✓ Point(x, y) works\n');

// Test Point 0-arg constructor
console.log('Test 3: Point() constructor');
const p3 = new cv.Point();
console.log('  Point():', p3.x, p3.y);
if (p3.x !== 0 || p3.y !== 0) {
    throw new Error(`Point() failed: expected (0, 0), got (${p3.x}, ${p3.y})`);
}
console.log('  ✓ Point() works\n');

// Test Size 1-arg constructor
console.log('Test 4: Size(width) constructor');
const s1 = new cv.Size(10);
console.log('  Size(10):', s1.width, s1.height);
if (s1.width !== 10 || s1.height !== 0) {
    throw new Error(`Size(10) failed: expected (10, 0), got (${s1.width}, ${s1.height})`);
}
console.log('  ✓ Size(width) works\n');

// Test Size 2-arg constructor
console.log('Test 5: Size(width, height) constructor');
const s2 = new cv.Size(4, 6);
console.log('  Size(4, 6):', s2.width, s2.height);
if (s2.width !== 4 || s2.height !== 6) {
    throw new Error(`Size(4, 6) failed: expected (4, 6), got (${s2.width}, ${s2.height})`);
}
console.log('  ✓ Size(width, height) works\n');

// Test Size 0-arg constructor
console.log('Test 6: Size() constructor');
const s3 = new cv.Size();
console.log('  Size():', s3.width, s3.height);
if (s3.width !== 0 || s3.height !== 0) {
    throw new Error(`Size() failed: expected (0, 0), got (${s3.width}, ${s3.height})`);
}
console.log('  ✓ Size() works\n');

console.log('✓✓✓ All Point/Size 1-arg constructor tests passed! ✓✓✓');
