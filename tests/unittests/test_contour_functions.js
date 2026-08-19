import { tests, eq } from './tinytest.js';
import * as cv from 'opencv';

// Test freestanding contour functions work with Mat CV_32SC2 data
// This verifies Phase 1: Method migration for opencv.js compatibility

tests({

  'contourArea - Mat CV_32SC2 rectangle'() {
    const contour = new cv.Mat(4, 1, cv.CV_32SC2);
    contour.data32S[0] = 0;   contour.data32S[1] = 0;
    contour.data32S[2] = 100; contour.data32S[3] = 0;
    contour.data32S[4] = 100; contour.data32S[5] = 50;
    contour.data32S[6] = 0;   contour.data32S[7] = 50;
    
    const area = cv.contourArea(contour);
    eq(5000, area);
  },

  'arcLength - Mat CV_32SC2 rectangle perimeter'() {
    const contour = new cv.Mat(4, 1, cv.CV_32SC2);
    contour.data32S[0] = 0;   contour.data32S[1] = 0;
    contour.data32S[2] = 100; contour.data32S[3] = 0;
    contour.data32S[4] = 100; contour.data32S[5] = 50;
    contour.data32S[6] = 0;   contour.data32S[7] = 50;
    
    const length = cv.arcLength(contour, true);
    eq(300, length);
  },

  'boundingRect - Mat CV_32SC2'() {
    const contour = new cv.Mat(4, 1, cv.CV_32SC2);
    contour.data32S[0] = 10;  contour.data32S[1] = 20;
    contour.data32S[2] = 110; contour.data32S[3] = 20;
    contour.data32S[4] = 110; contour.data32S[5] = 70;
    contour.data32S[6] = 10;  contour.data32S[7] = 70;
    
    const rect = cv.boundingRect(contour);
    eq(10, rect.x);
    eq(20, rect.y);
    eq(100, rect.width);
    eq(50, rect.height);
  },

  'approxPolyDP - Mat CV_32SC2 input and output'() {
    // Create a complex contour with many points (circle approximation)
    const contour = new cv.Mat(20, 1, cv.CV_32SC2);
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      contour.data32S[i * 2] = Math.round(100 + 50 * Math.cos(angle));
      contour.data32S[i * 2 + 1] = Math.round(100 + 50 * Math.sin(angle));
    }
    
    const approx = new cv.Mat();
    cv.approxPolyDP(contour, approx, 5.0, true);
    
    if (approx.rows >= contour.rows) {
      throw new Error(`Expected fewer points after approximation, got ${approx.rows} >= ${contour.rows}`);
    }
    if (approx.rows < 3) {
      throw new Error(`Expected at least 3 points, got ${approx.rows}`);
    }
    eq(cv.CV_32SC2, approx.type());
  },

  'convexHull - Mat CV_32SC2'() {
    const contour = new cv.Mat(5, 1, cv.CV_32SC2);
    contour.data32S[0] = 0;   contour.data32S[1] = 0;
    contour.data32S[2] = 100; contour.data32S[3] = 0;
    contour.data32S[4] = 50;  contour.data32S[5] = 50; // interior point
    contour.data32S[6] = 100; contour.data32S[7] = 100;
    contour.data32S[8] = 0;   contour.data32S[9] = 100;
    
    const hull = new cv.Mat();
    cv.convexHull(contour, hull);
    
    eq(4, hull.rows);
    eq(cv.CV_32SC2, hull.type());
  },

  'fitEllipse - Mat CV_32SC2'() {
    // Create points roughly on an ellipse
    const contour = new cv.Mat(20, 1, cv.CV_32SC2);
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      contour.data32S[i * 2] = Math.round(100 + 80 * Math.cos(angle));
      contour.data32S[i * 2 + 1] = Math.round(100 + 40 * Math.sin(angle));
    }
    
    const ellipse = cv.fitEllipse(contour);
    if (!ellipse) throw new Error('fitEllipse should return RotatedRect');
    if (typeof ellipse.center.x !== 'number') throw new Error('Should have center.x');
    if (typeof ellipse.center.y !== 'number') throw new Error('Should have center.y');
    if (typeof ellipse.size.width !== 'number') throw new Error('Should have size.width');
    if (typeof ellipse.size.height !== 'number') throw new Error('Should have size.height');
    
    // Verify approximate values (ellipse should be ~160x80)
    const width = Math.max(ellipse.size.width, ellipse.size.height);
    const height = Math.min(ellipse.size.width, ellipse.size.height);
    if (width < 140 || width > 180) {
      throw new Error(`Expected width ~160, got ${width}`);
    }
    if (height < 60 || height > 100) {
      throw new Error(`Expected height ~80, got ${height}`);
    }
  },

  'minEnclosingCircle - Mat CV_32SC2'() {
    const contour = new cv.Mat(4, 1, cv.CV_32SC2);
    contour.data32S[0] = 50;  contour.data32S[1] = 50;
    contour.data32S[2] = 150; contour.data32S[3] = 50;
    contour.data32S[4] = 150; contour.data32S[5] = 150;
    contour.data32S[6] = 50;  contour.data32S[7] = 150;
    
    const {center, radius} = cv.minEnclosingCircle(contour);
    if (!center) throw new Error('Should return center');
    if (Math.abs(center.x - 100) > 2) {
      throw new Error(`Expected center.x ~100, got ${center.x}`);
    }
    if (Math.abs(center.y - 100) > 2) {
      throw new Error(`Expected center.y ~100, got ${center.y}`);
    }
    if (radius < 60 || radius > 80) {
      throw new Error(`Expected radius ~70, got ${radius}`);
    }
  },

  'isContourConvex - Mat CV_32SC2 convex polygon'() {
    const contour = new cv.Mat(4, 1, cv.CV_32SC2);
    contour.data32S[0] = 0;   contour.data32S[1] = 0;
    contour.data32S[2] = 100; contour.data32S[3] = 0;
    contour.data32S[4] = 100; contour.data32S[5] = 100;
    contour.data32S[6] = 0;   contour.data32S[7] = 100;
    
    const isConvex = cv.isContourConvex(contour);
    eq(true, isConvex);
  },

  'pointPolygonTest - Mat CV_32SC2 point inside'() {
    const contour = new cv.Mat(4, 1, cv.CV_32SC2);
    contour.data32S[0] = 0;   contour.data32S[1] = 0;
    contour.data32S[2] = 100; contour.data32S[3] = 0;
    contour.data32S[4] = 100; contour.data32S[5] = 100;
    contour.data32S[6] = 0;   contour.data32S[7] = 100;
    
    const result = cv.pointPolygonTest(contour, { x: 50, y: 50 }, false);
    if (result <= 0) {
      throw new Error(`Point should be inside (positive), got ${result}`);
    }
  },

  'pointPolygonTest - Mat CV_32SC2 point outside'() {
    const contour = new cv.Mat(4, 1, cv.CV_32SC2);
    contour.data32S[0] = 0;   contour.data32S[1] = 0;
    contour.data32S[2] = 100; contour.data32S[3] = 0;
    contour.data32S[4] = 100; contour.data32S[5] = 100;
    contour.data32S[6] = 0;   contour.data32S[7] = 100;
    
    const result = cv.pointPolygonTest(contour, { x: 150, y: 50 }, false);
    if (result >= 0) {
      throw new Error(`Point should be outside (negative), got ${result}`);
    }
  },

  'pointPolygonTest - Mat CV_32SC2 with distance measurement'() {
    const contour = new cv.Mat(4, 1, cv.CV_32SC2);
    contour.data32S[0] = 0;   contour.data32S[1] = 0;
    contour.data32S[2] = 100; contour.data32S[3] = 0;
    contour.data32S[4] = 100; contour.data32S[5] = 100;
    contour.data32S[6] = 0;   contour.data32S[7] = 100;
    
    const result = cv.pointPolygonTest(contour, { x: 50, y: 50 }, true);
    eq(50, result);
  },

  'matchShapes - Mat CV_32SC2'() {
    const contour1 = new cv.Mat(4, 1, cv.CV_32SC2);
    contour1.data32S[0] = 0;   contour1.data32S[1] = 0;
    contour1.data32S[2] = 100; contour1.data32S[3] = 0;
    contour1.data32S[4] = 100; contour1.data32S[5] = 100;
    contour1.data32S[6] = 0;   contour1.data32S[7] = 100;
    
    const contour2 = new cv.Mat(4, 1, cv.CV_32SC2);
    contour2.data32S[0] = 10;   contour2.data32S[1] = 10;
    contour2.data32S[2] = 110;  contour2.data32S[3] = 10;
    contour2.data32S[4] = 110;  contour2.data32S[5] = 110;
    contour2.data32S[6] = 10;   contour2.data32S[7] = 110;
    
    // Same shape, just translated - should have low match value
    const match = cv.matchShapes(contour1, contour2, cv.CONTOURS_MATCH_I1, 0);
    if (match > 0.1) {
      throw new Error(`Expected low match value for similar shapes, got ${match}`);
    }
  },

  'HuMoments - Mat CV_32SC2'() {
    const contour = new cv.Mat(4, 1, cv.CV_32SC2);
    contour.data32S[0] = 0;   contour.data32S[1] = 0;
    contour.data32S[2] = 100; contour.data32S[3] = 0;
    contour.data32S[4] = 100; contour.data32S[5] = 100;
    contour.data32S[6] = 0;   contour.data32S[7] = 100;
    
    const moments = cv.moments(contour);
    const hu = cv.HuMoments(moments);
    
    if (!hu || hu.length !== 7) {
      throw new Error(`Expected Float64Array of length 7, got ${hu}`);
    }
    // Hu moments should be non-zero for a square
    if (hu[0] === 0) throw new Error('hu[0] should be non-zero');
  },

  'fitLine - Mat CV_32SC2'() {
    // Create points roughly on a line
    const contour = new cv.Mat(10, 1, cv.CV_32SC2);
    for (let i = 0; i < 10; i++) {
      contour.data32S[i * 2] = i * 10;
      contour.data32S[i * 2 + 1] = i * 10;
    }
    
    const line = new cv.Mat();
    cv.fitLine(contour, line, cv.DIST_L2, 0, 0.01, 0.01);
    
    if (line.rows !== 1 || line.cols !== 4) {
      throw new Error(`Expected 1x4 Mat, got ${line.rows}x${line.cols}`);
    }
    eq(cv.CV_32FC1, line.type());
    
    // Line direction should be approximately (0.707, 0.707) for 45-degree line
    const vx = line.data32F[0];
    const vy = line.data32F[1];
    if (Math.abs(vx - 0.707) > 0.1 || Math.abs(vy - 0.707) > 0.1) {
      throw new Error(`Expected direction ~(0.707, 0.707), got (${vx}, ${vy})`);
    }
  },

  'convexityDefects - Mat CV_32SC2'() {
    // Create a contour with a concavity
    const contour = new cv.Mat(6, 1, cv.CV_32SC2);
    contour.data32S[0] = 0;   contour.data32S[1] = 0;
    contour.data32S[2] = 50;  contour.data32S[3] = 50;  // concavity
    contour.data32S[4] = 100; contour.data32S[5] = 0;
    contour.data32S[6] = 100; contour.data32S[7] = 100;
    contour.data32S[8] = 50;  contour.data32S[9] = 80;
    contour.data32S[10] = 0;  contour.data32S[11] = 100;
    
    const hull = new cv.Mat();
    cv.convexHull(contour, hull, false, false); // return indices
    
    const defects = new cv.Mat();
    cv.convexityDefects(contour, hull, defects);
    
    // Should have found defects
    if (defects.rows > 0 && defects.cols !== 4) {
      throw new Error(`Expected 4 columns per defect, got ${defects.cols}`);
    }
  },

  'rotatedRectangleIntersection'() {
    const rect1 = new cv.RotatedRect({ x: 50, y: 50 }, { width: 40, height: 40 }, 0);
    const rect2 = new cv.RotatedRect({ x: 70, y: 70 }, { width: 40, height: 40 }, 0);
    
    const intersection = new cv.Mat();
    const result = cv.rotatedRectangleIntersection(rect1, rect2, intersection);
    
    eq(cv.INTERSECT_PARTIAL, result);
    if (intersection.rows === 0) {
      throw new Error('Expected non-empty intersection');
    }
  },

});
