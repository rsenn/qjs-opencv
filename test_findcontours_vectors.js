import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing findContours with MatVector and PointVectorVector...\n');

// Create a test image
const img = new cv.Mat(300, 300, cv.CV_8UC1, [0]);

// Manually draw 2 rectangles
const data = img.data;

// Rectangle 1: 50x50 at (30, 30)
for (let i = 30; i < 80; i++) {
    for (let j = 30; j < 80; j++) {
        data[i * 300 + j] = 255;
    }
}

// Rectangle 2: 60x60 at (180, 180)
for (let i = 180; i < 240; i++) {
    for (let j = 180; j < 240; j++) {
        data[i * 300 + j] = 255;
    }
}

console.log('Test image created with 2 rectangles');

// Test 1: findContours with MatVector
console.log('\n=== Test 1: MatVector ===');
const matContour = new cv.MatVector();
const hierarchy1 = new cv.Mat();
cv.findContours(img, matContour, hierarchy1, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
console.log('MatVector size:', matContour.size());

for (let i = 0; i < matContour.size(); i++) {
    const contour = matContour.get(i);
    console.log(`  Contour ${i}: ${contour.rows} points, type=${contour.type()}`);
    
    if (contour.rows === 0) {
        throw new Error(`Contour ${i} has 0 rows`);
    }
    
    const area = cv.contourArea(contour);
    console.log(`    Area: ${area}`);
}
console.log('✓ MatVector test passed');

// Test 2: findContours with PointVectorVector
console.log('\n=== Test 2: PointVectorVector ===');
const pvvContour = new cv.PointVectorVector();
const hierarchy2 = new cv.Mat();
cv.findContours(img, pvvContour, hierarchy2, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
console.log('PointVectorVector size:', pvvContour.size());

for (let i = 0; i < pvvContour.size(); i++) {
    const pointVec = pvvContour.get(i);
    console.log(`  Contour ${i}: ${pointVec.size()} points`);
    
    if (pointVec.size() === 0) {
        throw new Error(`Contour ${i} has 0 points`);
    }
    
    const area = cv.contourArea(pointVec);
    console.log(`    Area: ${area}`);
}
console.log('✓ PointVectorVector test passed');

// Test 3: findContours with traditional array (backward compatibility)
console.log('\n=== Test 3: Traditional Array (backward compatibility) ===');
const arrayContour = [];
const hierarchy3 = new cv.Mat();
cv.findContours(img, arrayContour, hierarchy3, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
console.log('Array length:', arrayContour.length);

for (let i = 0; i < arrayContour.length; i++) {
    const contour = arrayContour[i];
    console.log(`  Contour ${i}: ${contour.length} points`);
    
    if (contour.length === 0) {
        throw new Error(`Contour ${i} has 0 points`);
    }
    
    const area = cv.contourArea(contour);
    console.log(`    Area: ${area}`);
}
console.log('✓ Traditional array test passed');

// Test 4: Compare results
console.log('\n=== Test 4: Compare Results ===');
if (matContour.size() !== pvvContour.size()) {
    throw new Error(`MatVector and PointVectorVector have different sizes: ${matContour.size()} vs ${pvvContour.size()}`);
}
if (matContour.size() !== arrayContour.length) {
    throw new Error(`MatVector and array have different sizes: ${matContour.size()} vs ${arrayContour.length}`);
}

console.log('All methods found', matContour.size(), 'contours');

// Compare areas
for (let i = 0; i < matContour.size(); i++) {
    const matArea = cv.contourArea(matContour.get(i));
    const pvvArea = cv.contourArea(pvvContour.get(i));
    const arrArea = cv.contourArea(arrayContour[i]);
    
    console.log(`  Contour ${i} areas: MatVector=${matArea}, PVV=${pvvArea}, Array=${arrArea}`);
    
    if (Math.abs(matArea - pvvArea) > 0.01) {
        throw new Error(`Contour ${i} area mismatch: MatVector=${matArea}, PointVectorVector=${pvvArea}`);
    }
    if (Math.abs(matArea - arrArea) > 0.01) {
        throw new Error(`Contour ${i} area mismatch: MatVector=${matArea}, Array=${arrArea}`);
    }
}
console.log('✓ All areas match');

// Test 5: Test with hierarchy
console.log('\n=== Test 5: Hierarchy Test ===');
const matContour2 = new cv.MatVector();
const hierarchy4 = new cv.Mat();
cv.findContours(img, matContour2, hierarchy4, cv.RETR_TREE, cv.CHAIN_APPROX_SIMPLE);
console.log('MatVector size:', matContour2.size());
console.log('Hierarchy size:', hierarchy4.rows, 'x', hierarchy4.cols);
console.log('✓ Hierarchy test passed');

console.log('\n✓✓✓ All tests passed! ✓✓✓');
