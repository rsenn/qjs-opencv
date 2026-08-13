import { tests, eq } from './tinytest.js';
import * as cv from 'opencv';

// Test psimpl (polyline simplification) functions with Mat CV_32SC2 data
// This verifies Phase 1: psimpl functions migrated to cv.psimpl.* namespace

tests({

  'psimpl.douglasPeucker - Mat CV_32SC2'() {
    // Create a complex contour with many points
    const contour = new cv.Mat(20, 1, cv.CV_32SC2);
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      contour.data32S[i * 2] = Math.round(100 + 50 * Math.cos(angle));
      contour.data32S[i * 2 + 1] = Math.round(100 + 50 * Math.sin(angle));
    }
    
    const simplified = cv.psimpl.douglasPeucker(contour, 5.0);
    
    if (simplified.rows >= contour.rows) {
      throw new Error(`Expected fewer points, got ${simplified.rows} >= ${contour.rows}`);
    }
    if (simplified.rows < 3) {
      throw new Error(`Expected at least 3 points, got ${simplified.rows}`);
    }
    eq(cv.CV_32SC2, simplified.type());
    eq(1, simplified.cols);
  },

  'psimpl.reumannWitkam - Mat CV_32SC2'() {
    const contour = new cv.Mat(20, 1, cv.CV_32SC2);
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      contour.data32S[i * 2] = Math.round(100 + 50 * Math.cos(angle));
      contour.data32S[i * 2 + 1] = Math.round(100 + 50 * Math.sin(angle));
    }
    
    const simplified = cv.psimpl.reumannWitkam(contour, 3.0);
    
    if (simplified.rows >= contour.rows) {
      throw new Error(`Expected fewer points, got ${simplified.rows} >= ${contour.rows}`);
    }
    eq(cv.CV_32SC2, simplified.type());
  },

  'psimpl.radialDistance - Mat CV_32SC2'() {
    const contour = new cv.Mat(20, 1, cv.CV_32SC2);
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      contour.data32S[i * 2] = Math.round(100 + 50 * Math.cos(angle));
      contour.data32S[i * 2 + 1] = Math.round(100 + 50 * Math.sin(angle));
    }
    
    const simplified = cv.psimpl.radialDistance(contour, 5.0);
    
    if (simplified.rows >= contour.rows) {
      throw new Error(`Expected fewer points, got ${simplified.rows} >= ${contour.rows}`);
    }
    eq(cv.CV_32SC2, simplified.type());
  },

  'psimpl.nthPoint - Mat CV_32SC2'() {
    const contour = new cv.Mat(20, 1, cv.CV_32SC2);
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      contour.data32S[i * 2] = Math.round(100 + 50 * Math.cos(angle));
      contour.data32S[i * 2 + 1] = Math.round(100 + 50 * Math.sin(angle));
    }
    
    // Keep every 2nd point
    const simplified = cv.psimpl.nthPoint(contour, 2);
    
    if (simplified.rows !== 10) {
      throw new Error(`Expected 10 points (every 2nd), got ${simplified.rows}`);
    }
    eq(cv.CV_32SC2, simplified.type());
  },

  'psimpl.opheim - Mat CV_32SC2'() {
    const contour = new cv.Mat(20, 1, cv.CV_32SC2);
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      contour.data32S[i * 2] = Math.round(100 + 50 * Math.cos(angle));
      contour.data32S[i * 2 + 1] = Math.round(100 + 50 * Math.sin(angle));
    }
    
    const simplified = cv.psimpl.opheim(contour, 2.0, 10.0);
    
    if (simplified.rows >= contour.rows) {
      throw new Error(`Expected fewer points, got ${simplified.rows} >= ${contour.rows}`);
    }
    eq(cv.CV_32SC2, simplified.type());
  },

  'psimpl.lang - Mat CV_32SC2'() {
    const contour = new cv.Mat(20, 1, cv.CV_32SC2);
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      contour.data32S[i * 2] = Math.round(100 + 50 * Math.cos(angle));
      contour.data32S[i * 2 + 1] = Math.round(100 + 50 * Math.sin(angle));
    }
    
    const simplified = cv.psimpl.lang(contour, 2.0, 10.0);
    
    if (simplified.rows >= contour.rows) {
      throw new Error(`Expected fewer points, got ${simplified.rows} >= ${contour.rows}`);
    }
    eq(cv.CV_32SC2, simplified.type());
  },

  'psimpl.perpendicularDistance - Mat CV_32SC2'() {
    const contour = new cv.Mat(20, 1, cv.CV_32SC2);
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      contour.data32S[i * 2] = Math.round(100 + 50 * Math.cos(angle));
      contour.data32S[i * 2 + 1] = Math.round(100 + 50 * Math.sin(angle));
    }
    
    const simplified = cv.psimpl.perpendicularDistance(contour, 2.0, 10.0);
    
    if (simplified.rows >= contour.rows) {
      throw new Error(`Expected fewer points, got ${simplified.rows} >= ${contour.rows}`);
    }
    eq(cv.CV_32SC2, simplified.type());
  },

  'psimpl - backward compatibility with Contour'() {
    // Create a Contour object (old API)
    const contour = new cv.Contour();
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      contour.push(new cv.Point(
        Math.round(100 + 50 * Math.cos(angle)),
        Math.round(100 + 50 * Math.sin(angle))
      ));
    }
    
    // Should still work with Contour objects
    const simplified = cv.psimpl.douglasPeucker(contour, 5.0);
    
    if (simplified.rows >= contour.length) {
      throw new Error(`Expected fewer points, got ${simplified.rows} >= ${contour.length}`);
    }
    eq(cv.CV_32SC2, simplified.type());
  },

  'psimpl - JS array input'() {
    // Test with plain JS array
    const points = [];
    for (let i = 0; i < 20; i++) {
      const angle = (i / 20) * Math.PI * 2;
      points.push({
        x: Math.round(100 + 50 * Math.cos(angle)),
        y: Math.round(100 + 50 * Math.sin(angle))
      });
    }
    
    const simplified = cv.psimpl.douglasPeucker(points, 5.0);
    
    if (simplified.rows >= points.length) {
      throw new Error(`Expected fewer points, got ${simplified.rows} >= ${points.length}`);
    }
    eq(cv.CV_32SC2, simplified.type());
  },

});
