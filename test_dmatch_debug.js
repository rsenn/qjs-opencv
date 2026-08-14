import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing DMatchVector...');
const vec = new cv.DMatchVector();
console.log('DMatchVector created');

const dm = new cv.DMatch(0, 1, 0.5);
console.log('DMatch created:', dm);
console.log('DMatch instanceof cv.DMatch:', dm instanceof cv.DMatch);

vec.push_back(dm);
console.log('push_back succeeded, size:', vec.size());

const got = vec.get(0);
console.log('get(0) returned:', got);
console.log('got type:', typeof got);
console.log('got instanceof cv.DMatch:', got instanceof cv.DMatch);
console.log('got.queryIdx:', got?.queryIdx);
console.log('got.trainIdx:', got?.trainIdx);
console.log('got.distance:', got?.distance);

// Check if it's a function
if (typeof got === 'function') {
    console.log('ERROR: got is a function, not an object!');
}
