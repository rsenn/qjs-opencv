import * as cv from './build/x86_64-linux-debug/opencv.so';

console.log('Testing PointVectorVector...\n');

try {
    // Create a PointVectorVector
    const pvv = new cv.PointVectorVector();
    console.log('✓ Created PointVectorVector');
    
    // Create first PointVector with 3 points
    const pv1 = new cv.PointVector();
    pv1.push_back({x: 1, y: 2});
    pv1.push_back({x: 3, y: 4});
    pv1.push_back({x: 5, y: 6});
    console.log('✓ Created first PointVector with', pv1.size(), 'points');
    
    // Create second PointVector with 2 points
    const pv2 = new cv.PointVector();
    pv2.push_back({x: 10, y: 20});
    pv2.push_back({x: 30, y: 40});
    console.log('✓ Created second PointVector with', pv2.size(), 'points');
    
    // Add both PointVectors to PointVectorVector
    pvv.push_back(pv1);
    pvv.push_back(pv2);
    console.log('✓ Added both PointVectors to PointVectorVector');
    console.log('  Size:', pvv.size());
    
    // Retrieve first PointVector
    const retrieved1 = pvv.get(0);
    console.log('✓ Retrieved first PointVector');
    console.log('  Size:', retrieved1.size());
    
    // Check points in first PointVector
    const p1_0 = retrieved1.get(0);
    const p1_1 = retrieved1.get(1);
    const p1_2 = retrieved1.get(2);
    console.log('  Points:', 
        `(${p1_0.x}, ${p1_0.y})`,
        `(${p1_1.x}, ${p1_1.y})`,
        `(${p1_2.x}, ${p1_2.y})`);
    
    // Verify values
    if (p1_0.x !== 1 || p1_0.y !== 2) {
        throw new Error(`First point mismatch: expected (1, 2), got (${p1_0.x}, ${p1_0.y})`);
    }
    if (p1_1.x !== 3 || p1_1.y !== 4) {
        throw new Error(`Second point mismatch: expected (3, 4), got (${p1_1.x}, ${p1_1.y})`);
    }
    if (p1_2.x !== 5 || p1_2.y !== 6) {
        throw new Error(`Third point mismatch: expected (5, 6), got (${p1_2.x}, ${p1_2.y})`);
    }
    console.log('✓ First PointVector points verified');
    
    // Retrieve second PointVector
    const retrieved2 = pvv.get(1);
    console.log('✓ Retrieved second PointVector');
    console.log('  Size:', retrieved2.size());
    
    // Check points in second PointVector
    const p2_0 = retrieved2.get(0);
    const p2_1 = retrieved2.get(1);
    console.log('  Points:', 
        `(${p2_0.x}, ${p2_0.y})`,
        `(${p2_1.x}, ${p2_1.y})`);
    
    // Verify values
    if (p2_0.x !== 10 || p2_0.y !== 20) {
        throw new Error(`First point mismatch: expected (10, 20), got (${p2_0.x}, ${p2_0.y})`);
    }
    if (p2_1.x !== 30 || p2_1.y !== 40) {
        throw new Error(`Second point mismatch: expected (30, 40), got (${p2_1.x}, ${p2_1.y})`);
    }
    console.log('✓ Second PointVector points verified');
    
    // Test iterator
    console.log('\nTesting iterator...');
    let count = 0;
    for (const pv of pvv) {
        console.log(`  Vector ${count}: size =`, pv.size());
        count++;
    }
    if (count !== 2) {
        throw new Error(`Iterator count mismatch: expected 2, got ${count}`);
    }
    console.log('✓ Iterator works correctly');
    
    // Test set operation
    const pv3 = new cv.PointVector();
    pv3.push_back({x: 100, y: 200});
    pvv.set(0, pv3);
    const retrieved3 = pvv.get(0);
    if (retrieved3.size() !== 1) {
        throw new Error(`Set operation failed: expected size 1, got ${retrieved3.size()}`);
    }
    const p3_0 = retrieved3.get(0);
    if (p3_0.x !== 100 || p3_0.y !== 200) {
        throw new Error(`Set operation failed: expected (100, 200), got (${p3_0.x}, ${p3_0.y})`);
    }
    console.log('✓ Set operation works correctly');
    
    console.log('\n✓ All tests passed!');
    
} catch (e) {
    console.error('\n✗ Test failed:', e.message);
    console.error(e.stack);
    process.exit(1);
}
