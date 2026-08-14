import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing DMatch directly...');
const dm = new cv.DMatch(0, 1, 0.5);
console.log('Created DMatch:');
console.log('  queryIdx:', dm.queryIdx);
console.log('  trainIdx:', dm.trainIdx);
console.log('  imgIdx:', dm.imgIdx);
console.log('  distance:', dm.distance);

console.log('\nPushing to vector...');
const vec = new cv.DMatchVector();
vec.push_back(dm);
console.log('Vector size:', vec.size());

console.log('\nRetrieving from vector...');
const dm2 = vec.get(0);
console.log('Retrieved DMatch:');
console.log('  queryIdx:', dm2.queryIdx);
console.log('  trainIdx:', dm2.trainIdx);
console.log('  imgIdx:', dm2.imgIdx);
console.log('  distance:', dm2.distance);

vec.delete();
