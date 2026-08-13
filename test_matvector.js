import { Mat, CV_8UC1, CV_32SC2, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE } from './build/x86_64-linux-debug/opencv.so';
import { findContours, drawContours } from './build/x86_64-linux-debug/opencv.so';
import { MatVector } from './build/x86_64-linux-debug/opencv.so';

// Create a simple binary image with two rectangles
const img = new Mat(200, 200, CV_8UC1, [0]);

// Draw two rectangles
const rect1 = new Mat(50, 50, CV_8UC1, [255]);
const rect2 = new Mat(40, 40, CV_8UC1, [255]);

rect1.copyTo(img.roi(20, 20, 50, 50));
rect2.copyTo(img.roi(120, 120, 40, 40));

// Test 1: Create MatVector and use with findContours
console.log('Test 1: MatVector with findContours');
const contours = new MatVector();
findContours(img, contours, null, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

console.log(`Found ${contours.size()} contours`);
if (contours.size() !== 2) {
    console.error('FAIL: Expected 2 contours');
    process.exit(1);
}

// Test 2: Access individual contours
console.log('\nTest 2: Access individual contours');
for (let i = 0; i < contours.size(); i++) {
    const contour = contours.get(i);
    console.log(`Contour ${i}: ${contour.rows} points, type=${contour.type()}`);
    if (contour.type() !== CV_32SC2) {
        console.error('FAIL: Expected CV_32SC2 type');
        process.exit(1);
    }
}

// Test 3: Iterate with for-of
console.log('\nTest 3: Iterate with for-of');
let count = 0;
for (const contour of contours) {
    console.log(`Contour ${count}: ${contour.rows} points`);
    count++;
}
if (count !== 2) {
    console.error('FAIL: Expected 2 iterations');
    process.exit(1);
}

// Test 4: push_back
console.log('\nTest 4: push_back');
const emptyVec = new MatVector();
const testMat = new Mat(10, 1, CV_32SC2, [0]);
emptyVec.push_back(testMat);
if (emptyVec.size() !== 1) {
    console.error('FAIL: Expected size 1 after push_back');
    process.exit(1);
}
console.log(`After push_back: size=${emptyVec.size()}`);

// Test 5: set
console.log('\nTest 5: set');
const anotherMat = new Mat(5, 1, CV_32SC2, [0]);
emptyVec.set(0, anotherMat);
const retrieved = emptyVec.get(0);
if (retrieved.rows !== 5) {
    console.error('FAIL: Expected 5 rows after set');
    process.exit(1);
}
console.log(`After set: retrieved contour has ${retrieved.rows} points`);

// Test 6: Use contours with drawContours
console.log('\nTest 6: Use with drawContours');
const canvas = new Mat(200, 200, CV_8UC1, [0]);
drawContours(canvas, contours, -1, [255], 2);
console.log('drawContours succeeded');

console.log('\n✓ All MatVector tests passed!');

// Cleanup
contours.delete();
emptyVec.delete();
img.delete();
rect1.delete();
rect2.delete();
canvas.delete();
testMat.delete();
anotherMat.delete();
