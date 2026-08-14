import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing MatVector...');
try {
    const vec = new cv.MatVector();
    console.log('MatVector created:', typeof vec);
    
    const mat1 = new cv.Mat(2, 2, cv.CV_8U, 1);
    console.log('Mat created:', typeof mat1);
    console.log('Mat rows:', mat1.rows);
    
    vec.push_back(mat1);
    console.log('push_back succeeded, size:', vec.size());
    
    const got = vec.get(0);
    console.log('get(0) returned:', got);
    console.log('get(0) type:', typeof got);
    console.log('get(0) rows:', got?.rows);
} catch (e) {
    console.log('ERROR:', e.message);
    console.log('Stack:', e.stack);
}
