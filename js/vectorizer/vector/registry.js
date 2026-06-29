// vector/registry.js
//
// Central registry of vectorization plugins. Adding a method = import its class
// and list it here. Nothing else in the app references concrete method classes.

import { CannyContours } from './methods/canny-contours.js';
import { ThresholdContours } from './methods/threshold-contours.js';
import { HoughLines } from './methods/hough-lines.js';
import { LSDLines } from './methods/lsd.js';
import { FastLines } from './methods/fast-line.js';
import { PaletteRegions } from './methods/palette-regions.js';
import { LowPolyDelaunay } from './methods/lowpoly-delaunay.js';
import { Skeleton } from './methods/skeleton.js';
import { Stipple } from './methods/stipple.js';
import { ShapeFit } from './methods/shape-fit.js';

export class Registry {
  constructor() {
    this._byId = new Map();
    this._order = [];
  }

  register(MethodClass) {
    const inst = new MethodClass();
    this._byId.set(inst.id, inst);
    this._order.push(inst.id);
    return this;
  }

  get(id) { return this._byId.get(id); }
  has(id) { return this._byId.has(id); }
  list() { return this._order.map((id) => this._byId.get(id)); }
  first() { return this._byId.get(this._order[0]); }
}

export function defaultRegistry() {
  const r = new Registry();
  // Order = order shown in the stage-2 method picker.
  r.register(CannyContours)      // the required starter: Canny -> findContours
   .register(ThresholdContours)  // filled regions from (adaptive) threshold
   .register(PaletteRegions)     // posterized color-region "paint by numbers"
   .register(LowPolyDelaunay)    // goodFeaturesToTrack + Subdiv2D triangulation
   .register(HoughLines)         // probabilistic Hough line segments
   .register(LSDLines)           // LineSegmentDetector
   .register(FastLines)          // FastLineDetector
   .register(Skeleton)           // morphological skeleton centerlines
   .register(ShapeFit)           // fit ellipses / min-area rects to contours
   .register(Stipple);           // intensity-driven dot stipple
  return r;
}
