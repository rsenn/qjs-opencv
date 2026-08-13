import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('=== Testing MatVector API ===');
const mv = new cv.MatVector();
console.log('Created MatVector');

const mat1 = new cv.Mat(2, 2, cv.CV_8U, 1);
const mat2 = new cv.Mat(3, 3, cv.CV_8U, 2);

mv.push_back(mat1);
mv.push_back(mat2);
console.log('push_back works, size:', mv.size());

const got = mv.get(0);
console.log('get(0) works, rows:', got.rows, 'cols:', got.cols);

mv.set(1, new cv.Mat(4, 4, cv.CV_8U, 3));
console.log('set works');

console.log('Iterating with for-of:');
for (const m of mv) {
    console.log('  Mat:', m.rows, 'x', m.cols);
}

console.log('\n=== Testing PointVector API ===');
const pv = new cv.PointVector();
const p1 = new cv.Point(1, 2);
const p2 = new cv.Point(3, 4);

pv.push_back(p1);
pv.push_back(p2);
console.log('push_back works, size:', pv.size());

const pgot = pv.get(0);
console.log('get(0) works, x:', pgot.x, 'y:', pgot.y);

pv.set(1, new cv.Point(5, 6));
console.log('set works');

console.log('Iterating with for-of:');
for (const p of pv) {
    console.log('  Point:', p.x, ',', p.y);
}

console.log('\n=== Testing RectVector API ===');
const rv = new cv.RectVector();
const r1 = new cv.Rect(0, 0, 10, 20);
const r2 = new cv.Rect(5, 5, 15, 25);

rv.push_back(r1);
rv.push_back(r2);
console.log('push_back works, size:', rv.size());

const rgot = rv.get(0);
console.log('get(0) works, x:', rgot.x, 'y:', rgot.y, 'w:', rgot.width, 'h:', rgot.height);

rv.set(1, new cv.Rect(10, 10, 20, 30));
console.log('set works');

console.log('Iterating with for-of:');
for (const r of rv) {
    console.log('  Rect:', r.x, ',', r.y, ',', r.width, 'x', r.height);
}

console.log('\n=== Testing KeyPointVector API ===');
const kv = new cv.KeyPointVector();
const k1 = new cv.KeyPoint(10, 20, 5);
const k2 = new cv.KeyPoint(30, 40, 10);

kv.push_back(k1);
kv.push_back(k2);
console.log('push_back works, size:', kv.size());

const kgot = kv.get(0);
console.log('get(0) works, pt:', kgot.pt.x, ',', kgot.pt.y, 'size:', kgot.size);

kv.set(1, new cv.KeyPoint(50, 60, 15));
console.log('set works');

console.log('Iterating with for-of:');
for (const k of kv) {
    console.log('  KeyPoint: (', k.pt.x, ',', k.pt.y, ') size:', k.size);
}

console.log('\n=== Testing DMatchVector API ===');
const dv = new cv.DMatchVector();
const d1 = new cv.DMatch(0, 1, 0.5);
const d2 = new cv.DMatch(2, 3, 0.8);

dv.push_back(d1);
dv.push_back(d2);
console.log('push_back works, size:', dv.size());

const dgot = dv.get(0);
console.log('get(0) works, queryIdx:', dgot.queryIdx, 'trainIdx:', dgot.trainIdx, 'distance:', dgot.distance);

dv.set(1, new cv.DMatch(4, 5, 1.0));
console.log('set works');

console.log('Iterating with for-of:');
for (const d of dv) {
    console.log('  DMatch:', d.queryIdx, '->', d.trainIdx, 'dist:', d.distance);
}

console.log('\n=== Testing IntVector API ===');
const iv = new cv.IntVector();
iv.push_back(1);
iv.push_back(2);
iv.push_back(3);
console.log('push_back works, size:', iv.size());
console.log('get(0):', iv.get(0));
iv.set(1, 10);
console.log('set(1, 10), get(1):', iv.get(1));

console.log('Iterating with for-of:');
for (const i of iv) {
    console.log('  int:', i);
}

console.log('\n=== Testing FloatVector API ===');
const fv = new cv.FloatVector();
fv.push_back(1.5);
fv.push_back(2.5);
fv.push_back(3.5);
console.log('push_back works, size:', fv.size());
console.log('get(0):', fv.get(0));
fv.set(1, 10.5);
console.log('set(1, 10.5), get(1):', fv.get(1));

console.log('\n=== Testing DoubleVector API ===');
const ddv = new cv.DoubleVector();
ddv.push_back(1.5);
ddv.push_back(2.5);
ddv.push_back(3.5);
console.log('push_back works, size:', ddv.size());
console.log('get(0):', ddv.get(0));
ddv.set(1, 10.5);
console.log('set(1, 10.5), get(1):', ddv.get(1));

console.log('\n=== Testing CharVector API ===');
const cv2 = new cv.CharVector();
cv2.push_back(65);  // 'A'
cv2.push_back(66);  // 'B'
cv2.push_back(67);  // 'C'
console.log('push_back works, size:', cv2.size());
console.log('get(0):', cv2.get(0));
cv2.set(1, 90);  // 'Z'
console.log('set(1, 90), get(1):', cv2.get(1));

console.log('\n=== Testing StringVector API ===');
const sv = new cv.StringVector();
sv.push_back('hello');
sv.push_back('world');
sv.push_back('test');
console.log('push_back works, size:', sv.size());
console.log('get(0):', sv.get(0));
sv.set(1, 'opencv');
console.log('set(1, "opencv"), get(1):', sv.get(1));

console.log('\n=== All tests passed! ===');
