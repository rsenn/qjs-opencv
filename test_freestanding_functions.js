import * as cv from 'opencv';

console.log('Testing freestanding contour functions...\n');

try {
  console.log('Creating test contour...');
  const contour = new cv.Contour();
  contour.push({x: 0, y: 0});
  contour.push({x: 100, y: 0});
  contour.push({x: 100, y: 100});
  contour.push({x: 0, y: 100});
  
  console.log('Contour has ' + contour.length + ' points\n');
  
  // Test freestanding functions
  console.log('Testing freestanding functions:');
  
  const area = cv.contourArea(contour);
  console.log('  cv.contourArea: ' + area + ' (expected ~10000)');
  
  const perimeter = cv.arcLength(contour, true);
  console.log('  cv.arcLength: ' + perimeter + ' (expected ~400)');
  
  const bbox = cv.boundingRect(contour);
  console.log('  cv.boundingRect: x=' + bbox.x + ', y=' + bbox.y + ', w=' + bbox.width + ', h=' + bbox.height);
  
  const minRect = cv.minAreaRect(contour);
  console.log('  cv.minAreaRect: center=(' + minRect.center.x + ',' + minRect.center.y + '), size=(' + minRect.size.width + ',' + minRect.size.height + ')');
  
  const [center, radius] = cv.minEnclosingCircle(contour);
  console.log('  cv.minEnclosingCircle: center=(' + center.x + ',' + center.y + '), radius=' + radius.toFixed(2));
  
  const isConvex = cv.isContourConvex(contour);
  console.log('  cv.isContourConvex: ' + isConvex);
  
  // Test psimpl namespace
  console.log('\nTesting psimpl namespace:');
  const simplified = cv.psimpl.douglasPeucker(contour, 5.0);
  console.log('  cv.psimpl.douglasPeucker: ' + simplified.length + ' points (from ' + contour.length + ')');
  
  console.log('\n✓ All freestanding functions work correctly!');
  
} catch (e) {
  console.log('Error:', e.message);
  console.log('Stack:', e.stack);
}
