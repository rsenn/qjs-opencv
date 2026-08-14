import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Debug PointVectorVector...\n');

// Create a PointVector with points
const pv1 = new cv.PointVector();
pv1.push_back({x: 1, y: 2});
pv1.push_back({x: 3, y: 4});
console.log('pv1 size:', pv1.size());
console.log('pv1 point 0:', pv1.get(0));
console.log('pv1 point 1:', pv1.get(1));

// Create PointVectorVector and add pv1
const pvv = new cv.PointVectorVector();
pvv.push_back(pv1);
console.log('\npvv size:', pvv.size());

// Retrieve the PointVector
const retrieved = pvv.get(0);
console.log('retrieved size:', retrieved.size());
console.log('retrieved point 0:', retrieved.get(0));
console.log('retrieved point 1:', retrieved.get(1));
