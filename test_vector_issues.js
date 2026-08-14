import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing MatVector issue...');
try {
    const vec = new cv.MatVector();
    console.log('MatVector created:', typeof vec);
    console.log('MatVector.push_back:', typeof vec.push_back);
    
    const mat = new cv.Mat(2, 2, cv.CV_8U, 1);
    console.log('Mat created:', typeof mat);
    console.log('Mat.rows:', mat.rows);
    
    vec.push_back(mat);
    console.log('push_back succeeded, size:', vec.size());
    
    const got = vec.get(0);
    console.log('get(0) returned:', typeof got);
    console.log('got.rows:', got?.rows);
} catch (e) {
    console.log('MatVector test failed:', e.message);
    console.log('Stack:', e.stack);
}

console.log('\nTesting DMatchVector issue...');
try {
    const vec = new cv.DMatchVector();
    console.log('DMatchVector created:', typeof vec);
    
    const dm = new cv.DMatch(0, 1, 0.5);
    console.log('DMatch created:', dm);
    
    vec.push_back(dm);
    console.log('push_back succeeded, size:', vec.size());
    
    const got = vec.get(0);
    console.log('get(0) returned:', typeof got);
    console.log('got.queryIdx:', got?.queryIdx);
    console.log('got.trainIdx:', got?.trainIdx);
    console.log('got.distance:', got?.distance);
} catch (e) {
    console.log('DMatchVector test failed:', e.message);
    console.log('Stack:', e.stack);
}
