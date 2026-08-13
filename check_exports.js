import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Available exports:', Object.keys(cv).sort().join(', '));
console.log('Has MatVector?', 'MatVector' in cv);
console.log('Has Mat?', 'Mat' in cv);
