import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing DMatch distance precision...\n');

// Test 3-arg constructor: DMatch(queryIdx, trainIdx, distance)
const dm1 = new cv.DMatch(0, 1, 123.456);
console.log('3-arg constructor: DMatch(0, 1, 123.456)');
console.log('  queryIdx:', dm1.queryIdx);
console.log('  trainIdx:', dm1.trainIdx);
console.log('  imgIdx:', dm1.imgIdx);
console.log('  distance:', dm1.distance);

// Test 4-arg constructor: DMatch(queryIdx, trainIdx, imgIdx, distance)
const dm2 = new cv.DMatch(2, 3, 4, 789.012);
console.log('\n4-arg constructor: DMatch(2, 3, 4, 789.012)');
console.log('  queryIdx:', dm2.queryIdx);
console.log('  trainIdx:', dm2.trainIdx);
console.log('  imgIdx:', dm2.imgIdx);
console.log('  distance:', dm2.distance);

// Test pushing to vector and retrieving
console.log('\nTesting vector operations...');
const vec = new cv.DMatchVector();
vec.push_back(dm1);
vec.push_back(dm2);
console.log('Vector size:', vec.size());

const dm1_retrieved = vec.get(0);
const dm2_retrieved = vec.get(1);

console.log('\nRetrieved from vector:');
console.log('First DMatch:');
console.log('  queryIdx:', dm1_retrieved.queryIdx);
console.log('  trainIdx:', dm1_retrieved.trainIdx);
console.log('  imgIdx:', dm1_retrieved.imgIdx);
console.log('  distance:', dm1_retrieved.distance);

console.log('Second DMatch:');
console.log('  queryIdx:', dm2_retrieved.queryIdx);
console.log('  trainIdx:', dm2_retrieved.trainIdx);
console.log('  imgIdx:', dm2_retrieved.imgIdx);
console.log('  distance:', dm2_retrieved.distance);

vec.delete();

console.log('\n✓ All tests completed');
