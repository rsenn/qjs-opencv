// core/composer.js
//
// Adapts the Model to the pure SVG composer. Reads each vectorized frame's
// result + placement and produces the final document. Pure JS (no OpenCV).

import { compose } from './svg.js';
import { identity } from './geometry.js';

export class Composer {
  compose(model) {
    const placements = [];
    for (const f of model.vectorizedFrames()) {
      const res = model.results.get(f.id);
      const pl = model.placements.get(f.id) || { H: identity(), opacity: 1 };
      placements.push({
        id: f.id,
        vectorData: res.vectorData,
        H: pl.H || identity(),
        opacity: pl.opacity ?? 1,
      });
    }
    return compose(model.canvas.width, model.canvas.height, placements, {
      background: model.canvas.background,
    });
  }
}
