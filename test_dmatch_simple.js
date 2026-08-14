import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Test 1: Check exports');
console.log('DMatch:', typeof cv.DMatch);
console.log('DMatchVector:', typeof cv.DMatchVector);

console.log('Test 2: Create DMatchVector');
try {
    const dv = new cv.DMatchVector();
    console.log('DMatchVector constructor works');
} catch(e) {
    console.log('DMatchVector constructor failed:', e.message);
    console.log(e.stack);
}

console.log('Test 3: Create DMatch');
try {
    const d = new cv.DMatch(0, 1, 0.5);
    console.log('DMatch constructor works:', d);
} catch(e) {
    console.log('DMatch constructor failed:', e.message);
    console.log(e.stack);
}

console.log('All tests completed');
