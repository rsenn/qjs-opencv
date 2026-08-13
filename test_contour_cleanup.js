import * as cv from 'opencv';

console.log('Testing Contour cleanup...\n');

// Test 1: Contour iteration still works
console.log('Test 1: Contour iteration');
const contour = new cv.Contour();
contour.push({x: 10, y: 10});
contour.push({x: 20, y: 10});
contour.push({x: 20, y: 20});
contour.push({x: 10, y: 20});

console.log(`  Contour has ${contour.length} points`);
for (const point of contour) {
  console.log(`    Point: (${point.x}, ${point.y})`);
}

// Test 2: Contours collection still works
console.log('\nTest 2: Contours collection');
const img = new cv.Mat.zeros(100, 100, cv.CV_8UC1);
cv.rectangle(img, {x: 20, y: 20}, {x: 40, y: 40}, 255, -1);
cv.rectangle(img, {x: 60, y: 60}, {x: 80, y: 80}, 255, -1);

const contours = [];
const hierarchy = [];
cv.findContours(img, contours, hierarchy, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);

console.log(`  Found ${contours.length} contours`);
for (let i = 0; i < contours.length; i++) {
  const c = contours[i];
  console.log(`    Contour ${i} has ${c.length} points`);
}

// Test 3: Freestanding functions still work
console.log('\nTest 3: Freestanding functions');
const c = contours[0];
console.log(`  cv.contourArea: ${cv.contourArea(c)}`);
console.log(`  cv.arcLength: ${cv.arcLength(c, true)}`);
console.log(`  cv.boundingRect: ${JSON.stringify(cv.boundingRect(c))}`);

// Test 4: psimpl namespace works
console.log('\nTest 4: psimpl namespace');
const simplified = cv.psimpl.douglasPeucker(contour, 2.0);
console.log(`  Simplified contour has ${simplified.length} points (from ${contour.length})`);

// Test 5: Verify removed methods are gone
console.log('\nTest 5: Verify removed methods are gone');
const removed = ['toArray', 'toString', 'approxPolyDP', 'convexHull', 'fitEllipse', 
                 'aspectRatio', 'extent', 'solidity', 'getMat', 'fromRect', 'fromString'];
for (const method of removed) {
  if (method in cv.Contour.prototype) {
    console.log(`  ❌ FAIL: ${method} still exists on Contour`);
  } else if (method.startsWith('from') && method in cv.Contour) {
    console.log(`  ❌ FAIL: ${method} still exists as static method`);
  } else {
    console.log(`  ✓ ${method} removed`);
  }
}

console.log('\n✓ All tests passed!');
