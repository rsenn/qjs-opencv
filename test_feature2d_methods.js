import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing Feature2D methods for opencv.js compatibility...\n');

// Test ORB detector
const orb = new cv.ORB();

// Test getDefaultName() method
console.log('Test 1: getDefaultName() method');
const name = orb.getDefaultName();
console.log('  ORB default name:', name);
if (typeof name !== 'string' || !name.includes('ORB')) {
    throw new Error(`getDefaultName() failed: expected string containing 'ORB', got ${name}`);
}
console.log('  ✓ getDefaultName() works\n');

// Test descriptorSize() method
console.log('Test 2: descriptorSize() method');
const size = orb.descriptorSize();
console.log('  ORB descriptor size:', size);
if (typeof size !== 'number' || size <= 0) {
    throw new Error(`descriptorSize() failed: expected positive number, got ${size}`);
}
console.log('  ✓ descriptorSize() works\n');

// Test descriptorType() method
console.log('Test 3: descriptorType() method');
const type = orb.descriptorType();
console.log('  ORB descriptor type:', type);
if (typeof type !== 'number') {
    throw new Error(`descriptorType() failed: expected number, got ${type}`);
}
console.log('  ✓ descriptorType() works\n');

// Test with SIFT detector
console.log('Test 4: SIFT detector methods');
const sift = new cv.SIFT();
const siftName = sift.getDefaultName();
const siftSize = sift.descriptorSize();
const siftType = sift.descriptorType();
console.log('  SIFT default name:', siftName);
console.log('  SIFT descriptor size:', siftSize);
console.log('  SIFT descriptor type:', siftType);

if (typeof siftName !== 'string' || !siftName.includes('SIFT')) {
    throw new Error(`SIFT getDefaultName() failed: expected string containing 'SIFT', got ${siftName}`);
}
if (typeof siftSize !== 'number' || siftSize <= 0) {
    throw new Error(`SIFT descriptorSize() failed: expected positive number, got ${siftSize}`);
}
if (typeof siftType !== 'number') {
    throw new Error(`SIFT descriptorType() failed: expected number, got ${siftType}`);
}
console.log('  ✓ SIFT methods work\n');

console.log('✓✓✓ All Feature2D method tests passed! ✓✓✓');
