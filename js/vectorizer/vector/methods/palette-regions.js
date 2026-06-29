// vector/methods/palette-regions.js
//
// Posterized "paint-by-numbers" vector art. The image is colour-quantized with
// the binding's palette functions, then each colour band is traced into filled
// polygons whose fill is the region's mean colour. Visually the most striking
// of the methods.
//
// NOTE: paletteGenerate/paletteMatch are rsenn-binding extras (not stock
// OpenCV). Assumed signatures:
//   paletteGenerate(src, k)      -> palette Mat (k colours)
//   paletteMatch(src, palette)   -> CV_8U index map (values 0..k-1)
// If your build differs, only this file needs adjusting.

import {
  Mat, GaussianBlur, Size, compare, findContours, pyrMeanShiftFiltering,
  paletteGenerate, paletteMatch,
  CMP_EQ, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE,
} from 'opencv.so';

import { VectorMethod } from '../base.js';
import { create } from '../../core/vectordata.js';
import { contoursToShapes, meanColorMasked, release } from '../../cv/convert.js';

const CMP_EQ_V = typeof CMP_EQ === 'number' ? CMP_EQ : 0;

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
    let src = mat;
    let smoothed = null;
    if (p.meanShift) {
      smoothed = new Mat();
      pyrMeanShiftFiltering(mat, smoothed, p.spatial, p.color);
      src = smoothed;
    }
    const palette = paletteGenerate(src, p.colors);
    const idx = paletteMatch(src, palette);   // CV_8U single-channel index map

    const shapes = [];
    for (let k = 0; k < p.colors; k++) {
      const mask = new Mat();
      compare(idx, k, mask, CMP_EQ_V);
      const contours = findContours(mask, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE)[0];
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
    }
    release(idx, palette, smoothed);
    // Largest regions first so small detail paints on top.
    return create(meta.width, meta.height, { shapes });
  }
}
