// vector/methods/palette-regions.js
//
// Posterized "paint-by-numbers" vector art. The image is colour-quantized with
// the binding's palette functions, then each colour band is traced into filled
// polygons whose fill is the region's mean colour. Visually the most striking
// of the methods.
//
// NOTE: paletteGenerate/paletteMatch are rsenn-binding extras (not stock
// OpenCV). Real signatures (js_algorithms.cpp):
//   paletteGenerate(src, mode, count) -> array of [b,g,r] colours (mode 0 =
//     BGR colour space + cube distance, see dominant_colors_grabber.hpp)
//   paletteMatch(src, dstOut, palette) -> writes a CV_8U index map (values
//     0..palette.length-1) into dstOut (must be a pre-allocated Mat/output
//     array - a plain [] resolves to cv::noArray() and is never filled).

import {
  Mat, MatVector, GaussianBlur, Size, inRange, findContours, pyrMeanShiftFiltering,
  paletteGenerate, paletteMatch,
  RETR_EXTERNAL, CHAIN_APPROX_SIMPLE,
} from 'opencv';

import { VectorMethod } from '../base.js';
import { create } from '../../core/vectordata.js';
import { contoursToShapes, meanColorMasked, release } from '../../cv/convert.js';

export class PaletteRegions extends VectorMethod {
  static id = 'palette';
  static label = 'Palette Regions';
  static description = 'Quantize to K colours, trace each band into filled coloured polygons.';

  paramsSpec() {
    return [
      { key: 'colors',   label: 'Colours (K)', type: 'int',   min: 2, max: 24, step: 1, default: 8 },
      { key: 'meanShift', label: 'Mean-shift', type: 'bool',  default: 1 },
      { key: 'spatial',  label: 'MS spatial',  type: 'int',   min: 2, max: 30, step: 1, default: 10 },
      { key: 'color',    label: 'MS colour',   type: 'int',   min: 2, max: 60, step: 1, default: 24 },
      { key: 'epsilon',  label: 'Simplify',    type: 'float', min: 0, max: 0.02, step: 0.0005, default: 0.001 },
      { key: 'minArea',  label: 'Min area',    type: 'int',   min: 0, max: 4000, step: 20, default: 80 },
    ];
  }

  apply(mat, p, meta) {
    const tick = meta.tick || (() => {});
    let src = mat;
    let smoothed = null;
    if (p.meanShift) {
      smoothed = new Mat();
      pyrMeanShiftFiltering(mat, smoothed, p.spatial, p.color);
      src = smoothed;
    }
    tick(0.15);
    const palette = paletteGenerate(src, 0, p.colors);   // array of [b,g,r]
    const idx = new Mat();
    paletteMatch(src, idx, palette);   // CV_8U single-channel index map
    tick(0.25);

    const shapes = [];
    for (let k = 0; k < palette.length; k++) {
      const mask = new Mat();
      inRange(idx, [k, k, k, k], [k, k, k, k], mask);
      const contours = new MatVector(), hierarchy = [];
      findContours(mask, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
      const fill = meanColorMasked(mat, mask);
      const band = contoursToShapes(contours, {
        mode: 'fill',
        epsilon: p.epsilon,
        minArea: p.minArea,
        minPoints: 3,
        style: { stroke: null, fill },
      });
      shapes.push(...band);
      release(mask);
      tick(0.25 + 0.7 * ((k + 1) / palette.length));
    }
    release(idx, palette, smoothed);
    // Largest regions first so small detail paints on top.
    return create(meta.width, meta.height, { shapes });
  }
}
