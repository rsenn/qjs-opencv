import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('=== MatVector Test ===');
const vec = new cv.MatVector();
const mat1 = new cv.Mat(2, 2, cv.CV_8U, 1);
const mat2 = new cv.Mat(3, 3, cv.CV_8U, 2);

vec.push_back(mat1);
vec.push_back(mat2);

console.log('size:', vec.size());
const got = vec.get(0);
console.log('got.rows:', got.rows, 'got.cols:', got.cols);

const mat3 = new cv.Mat(4, 4, cv.CV_8U, 3);
vec.set(1, mat3);
const got2 = vec.get(1);
console.log('got2.rows:', got2.rows);

console.log('Checking iterator...');
let count = 0;
for (const m of vec) {
    count++;
    console.log('  m is Mat?', m instanceof cv.Mat);
}
console.log('count:', count);

console.log('Checking delete...');
console.log('vec.delete type:', typeof vec.delete);
vec.delete();
mat1.delete();
mat2.delete();
mat3.delete();
console.log('MatVector test passed!\n');

console.log('=== DMatchVector Test ===');
const dvec = new cv.DMatchVector();
const dm1 = new cv.DMatch(0, 1, 0.5);
const dm2 = new cv.DMatch(2, 3, 0.8);

dvec.push_back(dm1);
dvec.push_back(dm2);

console.log('size:', dvec.size());
const dgot = dvec.get(0);
console.log('dgot.queryIdx:', dgot.queryIdx);
console.log('dgot.trainIdx:', dgot.trainIdx);
console.log('dgot.distance:', dgot.distance);

if (dgot.queryIdx !== 0 || dgot.trainIdx !== 1 || dgot.distance !== 0.5) {
    console.log('ERROR: get(0) values wrong');
} else {
    console.log('DMatchVector test passed!');
}
