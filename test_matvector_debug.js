import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Import successful');
console.log('cv type:', typeof cv);
console.log('cv is null?', cv === null);
console.log('cv is undefined?', cv === undefined);

if (!cv) {
    console.error('ERROR: cv is not available!');
    process.exit(1);
}

console.log('Total exports:', Object.keys(cv).length);
console.log('Has MatVector?', 'MatVector' in cv);
console.log('Has Mat?', 'Mat' in cv);

const { findContours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Mat, MatVector } = cv;

console.log('Destructuring successful');
console.log('MatVector type:', typeof MatVector);
console.log('cv.MatVector type:', typeof cv.MatVector);
console.log('cv.Mat type:', typeof cv.Mat);

if (typeof cv.MatVector === 'undefined') {
    console.error('ERROR: MatVector is not exported from the module!');
    console.log('Available exports (first 20):', Object.keys(cv).slice(0, 20).join(', '));
    process.exit(1);
}

// Create a simple test image with some contours
const img = new Mat(100, 100, cv.CV_8UC1, 0);

// Draw some rectangles to create contours
cv.rectangle(img, {x: 10, y: 10, width: 30, height: 30}, 255, -1);
cv.rectangle(img, {x: 50, y: 50, width: 20, height: 20}, 255, -1);
cv.rectangle(img, {x: 70, y: 10, width: 25, height: 40}, 255, -1);

// Find contours using MatVector
const contours = new cv.MatVector();
const hierarchy = new Mat();

console.log('Calling findContours...');
findContours(img, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

console.log('Number of contours found:', contours.size());
console.log('Type of contours:', contours.constructor.name);

// Test iteration
console.log('\nIterating through contours:');
let count = 0;
for (const contour of contours) {
    console.log(`  Contour ${count}: rows=${contour.rows}, cols=${contour.cols}, type=${contour.type()}`);
    
    // Test get() method
    const c = contours.get(count);
    console.log(`    get(${count}): rows=${c.rows}, cols=${c.cols}`);
    
    count++;
}

// Test push_back
console.log('\nTesting push_back...');
const newContour = new Mat(5, 1, cv.CV_32SC2);
const originalSize = contours.size();
contours.push_back(newContour);
console.log(`  Original size: ${originalSize}, After push_back: ${contours.size()}`);

// Test set
console.log('\nTesting set...');
const replacementContour = new Mat(3, 1, cv.CV_32SC2);
contours.set(0, replacementContour);
const retrieved = contours.get(0);
console.log(`  Set contour 0, retrieved: rows=${retrieved.rows}, cols=${retrieved.cols}`);

// Test delete
console.log('\nTesting delete...');
contours.delete();
console.log('  Deleted contours MatVector');

console.log('\n✓ MatVector test completed successfully!');
