import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Checking exported vector types:');
console.log('MatVector:', typeof cv.MatVector);
console.log('DMatchVector:', typeof cv.DMatchVector);
console.log('PointVector:', typeof cv.PointVector);
console.log('RectVector:', typeof cv.RectVector);
console.log('KeyPointVector:', typeof cv.KeyPointVector);
console.log('IntVector:', typeof cv.IntVector);

// Check if constructors work
try {
    const mv = new cv.MatVector();
    console.log('MatVector constructor works');
} catch(e) {
    console.log('MatVector constructor failed:', e.message);
}

try {
    const dv = new cv.DMatchVector();
    console.log('DMatchVector constructor works');
} catch(e) {
    console.log('DMatchVector constructor failed:', e.message);
}
