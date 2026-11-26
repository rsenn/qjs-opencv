import * as std from 'std';
import * as cv from 'opencv';

/* findContours/drawContours with plain JS arrays: contours get one CV_32SC2
 * Mat per contour, hierarchy one [next, previous, child, parent] per contour */
let failed = 0;
function ok(name, fn) {
  try {
    fn();
    console.log('OK  ', name);
  } catch(e) {
    failed++;
    console.log('FAIL', name, '-', (e && e.message) || e);
  }
}

function scene() {
  const bin = new cv.Mat(100, 100, cv.CV_8UC1);
  bin.setTo(0);
  cv.rectangle(bin, [10, 10], [40, 40], [255], -1);
  cv.rectangle(bin, [60, 60], [90, 90], [255], -1);
  cv.rectangle(bin, [65, 65], [85, 85], [0], -1);
  return bin;
}

ok('plain array contours + hierarchy, RETR_TREE', () => {
  const contours = [], hierarchy = [];
  cv.findContours(scene(), contours, hierarchy, cv.RETR_TREE, cv.CHAIN_APPROX_SIMPLE);
  if(contours.length != 3) throw new Error('contours ' + contours.length);
  if(hierarchy.length != 3) throw new Error('hierarchy ' + hierarchy.length);
  if(hierarchy.some(h => h.length != 4)) throw new Error('hierarchy rows ' + JSON.stringify(hierarchy));
  if(!hierarchy.some(h => h[3] >= 0)) throw new Error('no child contour has a parent');
});

ok('hierarchy as Mat still works', () => {
  const contours = [], hier = new cv.Mat();
  cv.findContours(scene(), contours, hier, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
  if(contours.length != 2) throw new Error('contours ' + contours.length);
  if(hier.empty()) throw new Error('hierarchy Mat empty');
});

ok('MatVector contours still work', () => {
  const contours = new cv.MatVector(), hier = new cv.Mat();
  cv.findContours(scene(), contours, hier, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
  if(contours.size() != 2) throw new Error('size ' + contours.size());
});

ok('drawContours draws what findContours returned in a plain array', () => {
  const contours = [], hierarchy = [], dst = new cv.Mat(100, 100, cv.CV_8UC1);
  dst.setTo(0);
  cv.findContours(scene(), contours, hierarchy, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
  cv.drawContours(dst, contours, -1, [255], 1);
  if(cv.countNonZero(dst) < 100) throw new Error('drew ' + cv.countNonZero(dst) + ' pixels');
});

if(failed) {
  console.log(`\n${failed} test(s) FAILED`);
  std.exit(1);
}
console.log('\nAll tests passed');
