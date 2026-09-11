/**
 * Shared contrail-candidate detection core, factored out of
 * examples/skywatch_contrail_detect.js so js/skywatch/export-training-
 * example.js can reuse the exact same mask-building/filtering logic
 * instead of drifting out of sync with a duplicate copy. See that
 * example script's header comment for the method itself (top-hat +
 * object-distance filtering) and why a fixed HSV threshold doesn't work.
 */
import * as cv from 'opencv';

export const MIN_AREA = 200;
export const MIN_ASPECT = 3;
export const MIN_DIST_FROM_OBJECT = 20;

export function buildCandidateMask(hsv) {
  const channels = [];
  cv.split(hsv, channels);
  const [, S, V] = channels;

  const whiteness = new cv.Mat();
  cv.subtract(V, S, whiteness);

  const topHatKernel = cv.getStructuringElement(cv.MORPH_ELLIPSE, new cv.Size(51, 51));
  const tophat = new cv.Mat();
  cv.morphologyEx(whiteness, tophat, cv.MORPH_TOPHAT, topHatKernel);

  const bright = new cv.Mat();
  cv.threshold(tophat, bright, 12, 255, cv.THRESH_BINARY);

  const blueMask = new cv.Mat();
  cv.inRange(hsv, [90, 40, 60], [140, 255, 255], blueMask);
  const nearSky = new cv.Mat();
  cv.dilate(blueMask, nearSky, cv.getStructuringElement(cv.MORPH_ELLIPSE, new cv.Size(15, 15)));

  const candidateMask = new cv.Mat();
  cv.bitwise_and(bright, nearSky, candidateMask);
  cv.morphologyEx(candidateMask, candidateMask, cv.MORPH_CLOSE,
    cv.getStructuringElement(cv.MORPH_ELLIPSE, new cv.Size(9, 9)));

  return { candidateMask, whiteness };
}

export function buildDistanceToSolidObject(hsv) {
  const dark = new cv.Mat();
  cv.inRange(hsv, [0, 0, 0], [180, 255, 99], dark);
  const nonBlueSaturated = new cv.Mat();
  cv.inRange(hsv, [0, 60, 0], [84, 255, 255], nonBlueSaturated);
  const solidObject = new cv.Mat();
  cv.bitwise_or(dark, nonBlueSaturated, solidObject);

  const notObject = new cv.Mat();
  cv.bitwise_not(solidObject, notObject);
  const dist = new cv.Mat();
  cv.distanceTransform(notObject, dist, cv.DIST_L2, 5);
  return dist;
}

export function measureWidths(contourMask) {
  const dist = new cv.Mat();
  cv.distanceTransform(contourMask, dist, cv.DIST_L2, 5);

  const skeleton = new cv.Mat();
  cv.ximgproc.thinning(contourMask, skeleton);

  const widths = [];
  for (let y = 0; y < skeleton.rows; y++) {
    for (let x = 0; x < skeleton.cols; x++) {
      if (skeleton.ucharAt(y, x) > 0) widths.push(2 * dist.floatAt(y, x));
    }
  }
  return { widths, skeleton };
}

/**
 * Runs the full candidate pipeline on a BGR frame and returns:
 * - hsv, whiteness: intermediate Mats a caller may want to reuse
 * - acceptedMask: CV_8UC1, 255 on every pixel of every contour that passed
 *   all filters - this IS the training mask for phase 6's segmentation
 *   model, and what the overlay in examples/skywatch_contrail_detect.js
 *   draws over.
 * - results: per-contour stats (area, lengthPx, width stats)
 */
export function detectContrails(bgr) {
  const hsv = new cv.Mat();
  cv.cvtColor(bgr, hsv, cv.COLOR_BGR2HSV);

  const { candidateMask, whiteness } = buildCandidateMask(hsv);
  const distToObject = buildDistanceToSolidObject(hsv);

  const contours = new cv.MatVector();
  const hierarchy = new cv.Mat();
  cv.findContours(candidateMask, contours, hierarchy, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);

  const acceptedMask = cv.Mat.zeros(candidateMask.rows, candidateMask.cols, cv.CV_8UC1);
  const results = [];

  for (const contour of contours) {
    const area = cv.contourArea(contour);
    if (area < MIN_AREA) continue;

    const rect = cv.minAreaRect(contour);
    const [w, h] = rect.size;
    const long = Math.max(w, h), short = Math.max(1, Math.min(w, h));
    if (long / short < MIN_ASPECT) continue;

    const bbox = cv.boundingRect(contour);
    let minDistToObject = Infinity;
    for (let y = bbox.y; y < bbox.y + bbox.height; y++) {
      for (let x = bbox.x; x < bbox.x + bbox.width; x++) {
        if (candidateMask.ucharAt(y, x) > 0)
          minDistToObject = Math.min(minDistToObject, distToObject.floatAt(y, x));
      }
    }
    if (minDistToObject < MIN_DIST_FROM_OBJECT) continue;

    // cv.drawContours requires a cv.MatVector, not a plain JS array of
    // contour Mats - see BUGS: drawcontours-silently-noops-on-plain-array.
    const singleContourVec = new cv.MatVector();
    singleContourVec.push_back(contour);

    const single = cv.Mat.zeros(candidateMask.rows, candidateMask.cols, cv.CV_8UC1);
    cv.drawContours(single, singleContourVec, -1, new cv.Scalar(255), -1);

    const { widths } = measureWidths(single);
    if (widths.length === 0) continue;

    cv.drawContours(acceptedMask, singleContourVec, -1, new cv.Scalar(255), -1);

    const mean = widths.reduce((a, b) => a + b, 0) / widths.length;
    results.push({
      contour, area, lengthPx: +long.toFixed(1),
      minWidthPx: +Math.min(...widths).toFixed(1),
      meanWidthPx: +mean.toFixed(1),
      maxWidthPx: +Math.max(...widths).toFixed(1),
    });
  }

  return { hsv, whiteness, acceptedMask, results };
}
