import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('DMatch:', typeof cv.DMatch);
console.log('DMatchVector:', typeof cv.DMatchVector);

try {
    const d = new cv.DMatch(0, 1, 0.5);
    console.log('DMatch constructor works:', d);
} catch(e) {
    console.log('DMatch constructor failed:', e.message);
}

try {
    const dv = new cv.DMatchVector();
    console.log('DMatchVector constructor works');
} catch(e) {
    console.log('DMatchVector constructor failed:', e.message);
}
