import * as cv from 'opencv';

try {
  console.log('Creating image...');
  const img = cv.Mat.zeros(100, 100, cv.CV_8UC1);
  console.log('Drawing rectangles...');
  cv.rectangle(img, {x: 20, y: 20}, {x: 40, y: 40}, 255, -1);
  cv.rectangle(img, {x: 60, y: 60}, {x: 80, y: 80}, 255, -1);
  
  console.log('Finding contours...');
  const contours = [];
  const hierarchy = [];
  cv.findContours(img, contours, hierarchy, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
  
  console.log('Found ' + contours.length + ' contours');
  for (let i = 0; i < contours.length; i++) {
    console.log('Contour ' + i + ': ' + contours[i].length + ' points');
  }
} catch (e) {
  console.log('Error:', e.message);
  console.log('Stack:', e.stack);
}
