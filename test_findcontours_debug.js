import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Debug findContours...\n');

// Create a test image
const img = new cv.Mat(200, 200, cv.CV_8UC1, [0]);
console.log('Image created:', img.rows, 'x', img.cols, 'type:', img.type());

// Try drawing a single white pixel first
const data = img.data;
for (let i = 50; i < 100; i++) {
    for (let j = 50; j < 100; j++) {
        data[i * 200 + j] = 255;
    }
}

console.log('Manual rectangle drawn');

// Check if image has non-zero pixels
let nonZeroCount = 0;
for (let i = 0; i < data.length; i++) {
    if (data[i] > 0) {
        nonZeroCount++;
    }
}
console.log('Non-zero pixels:', nonZeroCount);

// Try with array first
const contours = [];
const hierarchy = new cv.Mat();
console.log('\nCalling findContours...');
cv.findContours(img, contours, hierarchy, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
console.log('Found', contours.length, 'contours');

if (contours.length > 0) {
    console.log('First contour:', contours[0].length, 'points');
    const area = cv.contourArea(contours[0]);
    console.log('First contour area:', area);
}
