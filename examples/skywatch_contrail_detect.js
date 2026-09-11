/**
 * Proof-of-concept contrail detection on plain (non-fisheye) photos, for
 * js/skywatch.md's phase 3/4 (trail detection) before any camera/ADS-B
 * hardware is involved.
 *
 * Approach (tuned empirically against real phone photos in
 * /mnt/data/Bilder/DCIM/Draussen/Chemtrails/, see BUGS for the tuning
 * process and a related minMaxLoc bug found along the way):
 *
 * 1. A fixed HSV threshold cannot separate a contrail from blue sky -
 *    Rayleigh scattering gives the sky itself a strong
 *    saturation/brightness gradient (paler and less saturated toward the
 *    horizon), which is often larger than the contrail-vs-sky difference.
 *    A white top-hat filter (V-S "whiteness" channel, opened with a large
 *    structuring element, subtracted from the original) removes that
 *    slowly-varying background and keeps only *locally* bright streaks -
 *    this is what actually finds the trail.
 * 2. The top-hat responds to any local contrast, including building/tree
 *    silhouette edges against the sky in a handheld photo. Restrict
 *    candidates to a modest dilation of a strict blue-sky mask (rejects
 *    building/tree material outright) - still not enough on its own,
 *    because the dilation halo around a building edge produces a
 *    thin bright ring that also passes an elongation/area shape filter.
 * 3. The distinguishing feature that finally separates real trail from
 *    a building-edge artifact: a real trail floats freely in open sky,
 *    while the artifact hugs directly against solid building/tree
 *    material. Reject any candidate that comes within a few pixels of a
 *    "solid object" mask (dark, or saturated non-blue hue).
 *
 * See js/skywatch/skywatch.md phase 3a for where this does and doesn't
 * work (natural cirrus is the open problem, not solved by this script).
 * The actual mask-building logic lives in js/skywatch/detect.js so
 * js/skywatch/export-training-example.js can reuse it exactly.
 *
 * Usage: qjsm examples/skywatch_contrail_detect.js <input.jpg> <output.jpg>
 */
import * as cv from 'opencv';
import { detectContrails } from '../js/skywatch/detect.js';

function main(inputPath, outputPath) {
  const bgr = cv.imread(inputPath);
  if (bgr.empty()) throw new Error(`could not read ${inputPath}`);

  const { acceptedMask, results } = detectContrails(bgr);

  const output = bgr.clone();
  for (const { contour } of results) {
    const vec = new cv.MatVector();
    vec.push_back(contour);
    cv.drawContours(output, vec, -1, new cv.Scalar(0, 255, 0), 2);
  }
  const skeleton = new cv.Mat();
  cv.ximgproc.thinning(acceptedMask, skeleton);
  for (let y = 0; y < skeleton.rows; y++)
    for (let x = 0; x < skeleton.cols; x++)
      if (skeleton.ucharAt(y, x) > 0) output.set(y, x, [0, 0, 255]);

  cv.imwrite(outputPath, output);
  const summary = results.map(({ contour, ...r }) => r);
  console.log(JSON.stringify({ input: inputPath, trailsFound: summary.length, results: summary }));
}

main(...scriptArgs.slice(1));
