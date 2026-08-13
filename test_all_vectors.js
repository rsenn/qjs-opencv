import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing all vector bindings...\n');

let passed = 0;
let failed = 0;

function test(name, fn) {
    try {
        fn();
        console.log(`✓ ${name}`);
        passed++;
    } catch (e) {
        console.log(`✗ ${name}: ${e.message}`);
        failed++;
    }
}

// Test MatVector
test('MatVector basic operations', () => {
    const vec = new cv.MatVector();
    const mat1 = new cv.Mat(2, 2, cv.CV_8U, 1);
    const mat2 = new cv.Mat(3, 3, cv.CV_8U, 2);
    
    vec.push_back(mat1);
    vec.push_back(mat2);
    
    if (vec.size() !== 2) throw new Error('size should be 2');
    
    const got = vec.get(0);
    if (got.rows !== 2 || got.cols !== 2) throw new Error('get(0) failed');
    
    const mat3 = new cv.Mat(4, 4, cv.CV_8U, 3);
    vec.set(1, mat3);
    const got2 = vec.get(1);
    if (got2.rows !== 4) throw new Error('set failed');
    
    let count = 0;
    for (const m of vec) {
        count++;
        if (!(m instanceof cv.Mat)) throw new Error('iterator failed');
    }
    if (count !== 2) throw new Error('iterator count wrong');
    
    vec.delete();
    mat1.delete();
    mat2.delete();
    mat3.delete();
});

// Test PointVector
test('PointVector basic operations', () => {
    const vec = new cv.PointVector();
    const p1 = new cv.Point(1, 2);
    const p2 = new cv.Point(3, 4);
    
    vec.push_back(p1);
    vec.push_back(p2);
    
    if (vec.size() !== 2) throw new Error('size should be 2');
    
    const got = vec.get(0);
    if (got.x !== 1 || got.y !== 2) throw new Error('get(0) failed');
    
    const p3 = new cv.Point(5, 6);
    vec.set(1, p3);
    const got2 = vec.get(1);
    if (got2.x !== 5) throw new Error('set failed');
    
    let count = 0;
    for (const p of vec) {
        count++;
        if (!(p instanceof cv.Point)) throw new Error('iterator failed');
    }
    if (count !== 2) throw new Error('iterator count wrong');
    
    vec.delete();
});

// Test RectVector
test('RectVector basic operations', () => {
    const vec = new cv.RectVector();
    const r1 = new cv.Rect(0, 0, 10, 20);
    const r2 = new cv.Rect(5, 5, 15, 25);
    
    vec.push_back(r1);
    vec.push_back(r2);
    
    if (vec.size() !== 2) throw new Error('size should be 2');
    
    const got = vec.get(0);
    if (got.x !== 0 || got.y !== 0 || got.width !== 10 || got.height !== 20) {
        throw new Error('get(0) failed');
    }
    
    const r3 = new cv.Rect(10, 10, 20, 30);
    vec.set(1, r3);
    const got2 = vec.get(1);
    if (got2.width !== 20) throw new Error('set failed');
    
    let count = 0;
    for (const r of vec) {
        count++;
        if (!(r instanceof cv.Rect)) throw new Error('iterator failed');
    }
    if (count !== 2) throw new Error('iterator count wrong');
    
    vec.delete();
});

// Test KeyPointVector
test('KeyPointVector basic operations', () => {
    const vec = new cv.KeyPointVector();
    const kp1 = new cv.KeyPoint(10, 20, 5);
    const kp2 = new cv.KeyPoint(30, 40, 10);
    
    vec.push_back(kp1);
    vec.push_back(kp2);
    
    if (vec.size() !== 2) throw new Error('size should be 2');
    
    const got = vec.get(0);
    if (got.pt.x !== 10 || got.pt.y !== 20 || got.size !== 5) {
        throw new Error('get(0) failed');
    }
    
    const kp3 = new cv.KeyPoint(50, 60, 15);
    vec.set(1, kp3);
    const got2 = vec.get(1);
    if (got2.pt.x !== 50) throw new Error('set failed');
    
    let count = 0;
    for (const kp of vec) {
        count++;
        if (!(kp instanceof cv.KeyPoint)) throw new Error('iterator failed');
    }
    if (count !== 2) throw new Error('iterator count wrong');
    
    vec.delete();
});

// Test DMatchVector
test('DMatchVector basic operations', () => {
    const vec = new cv.DMatchVector();
    const dm1 = new cv.DMatch(0, 1, 0.5);
    const dm2 = new cv.DMatch(2, 3, 0.8);
    
    vec.push_back(dm1);
    vec.push_back(dm2);
    
    if (vec.size() !== 2) throw new Error('size should be 2');
    
    const got = vec.get(0);
    if (got.queryIdx !== 0 || got.trainIdx !== 1 || got.distance !== 0.5) {
        throw new Error('get(0) failed');
    }
    
    const dm3 = new cv.DMatch(4, 5, 1.0);
    vec.set(1, dm3);
    const got2 = vec.get(1);
    if (got2.queryIdx !== 4) throw new Error('set failed');
    
    let count = 0;
    for (const dm of vec) {
        count++;
        if (!(dm instanceof cv.DMatch)) throw new Error('iterator failed');
    }
    if (count !== 2) throw new Error('iterator count wrong');
    
    vec.delete();
});

// Test IntVector
test('IntVector basic operations', () => {
    const vec = new cv.IntVector();
    
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    
    if (vec.size() !== 3) throw new Error('size should be 3');
    
    const got = vec.get(0);
    if (got !== 1) throw new Error('get(0) failed');
    
    vec.set(1, 10);
    const got2 = vec.get(1);
    if (got2 !== 10) throw new Error('set failed');
    
    let sum = 0;
    for (const val of vec) {
        sum += val;
    }
    if (sum !== 14) throw new Error('iterator sum wrong');
    
    vec.delete();
});

// Test FloatVector
test('FloatVector basic operations', () => {
    const vec = new cv.FloatVector();
    
    vec.push_back(1.5);
    vec.push_back(2.5);
    vec.push_back(3.5);
    
    if (vec.size() !== 3) throw new Error('size should be 3');
    
    const got = vec.get(0);
    if (Math.abs(got - 1.5) > 0.001) throw new Error('get(0) failed');
    
    vec.set(1, 10.5);
    const got2 = vec.get(1);
    if (Math.abs(got2 - 10.5) > 0.001) throw new Error('set failed');
    
    let sum = 0;
    for (const val of vec) {
        sum += val;
    }
    if (Math.abs(sum - 15.5) > 0.001) throw new Error('iterator sum wrong');
    
    vec.delete();
});

// Test DoubleVector
test('DoubleVector basic operations', () => {
    const vec = new cv.DoubleVector();
    
    vec.push_back(1.5);
    vec.push_back(2.5);
    vec.push_back(3.5);
    
    if (vec.size() !== 3) throw new Error('size should be 3');
    
    const got = vec.get(0);
    if (Math.abs(got - 1.5) > 0.001) throw new Error('get(0) failed');
    
    vec.set(1, 10.5);
    const got2 = vec.get(1);
    if (Math.abs(got2 - 10.5) > 0.001) throw new Error('set failed');
    
    let sum = 0;
    for (const val of vec) {
        sum += val;
    }
    if (Math.abs(sum - 15.5) > 0.001) throw new Error('iterator sum wrong');
    
    vec.delete();
});

// Test CharVector
test('CharVector basic operations', () => {
    const vec = new cv.CharVector();
    
    vec.push_back(65);  // 'A'
    vec.push_back(66);  // 'B'
    vec.push_back(67);  // 'C'
    
    if (vec.size() !== 3) throw new Error('size should be 3');
    
    const got = vec.get(0);
    if (got !== 65) throw new Error('get(0) failed');
    
    vec.set(1, 90);  // 'Z'
    const got2 = vec.get(1);
    if (got2 !== 90) throw new Error('set failed');
    
    let count = 0;
    for (const val of vec) {
        count++;
    }
    if (count !== 3) throw new Error('iterator count wrong');
    
    vec.delete();
});

// Test StringVector
test('StringVector basic operations', () => {
    const vec = new cv.StringVector();
    
    vec.push_back('hello');
    vec.push_back('world');
    vec.push_back('test');
    
    if (vec.size() !== 3) throw new Error('size should be 3');
    
    const got = vec.get(0);
    if (got !== 'hello') throw new Error('get(0) failed');
    
    vec.set(1, 'opencv');
    const got2 = vec.get(1);
    if (got2 !== 'opencv') throw new Error('set failed');
    
    let result = '';
    for (const val of vec) {
        result += val + ' ';
    }
    if (result !== 'hello opencv test ') throw new Error('iterator failed');
    
    vec.delete();
});

console.log(`\n${passed} passed, ${failed} failed`);
process.exit(failed > 0 ? 1 : 0);
