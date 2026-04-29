// Trace mode: outline. Threshold the image, follow region boundaries
// with marching squares, simplify the resulting polylines.
//
// Each mode is a pure `(imageData, params) → polylines[mm]` function so
// TraceView can dispatch by name and stay agnostic of the algorithm.

import { threshold } from './threshold.js';
import { trace } from './trace.js';
import { simplify } from './simplify.js';

export function outline(image, { thresholdVal, detailMm, invert, fit }) {
  const binary = threshold(image, thresholdVal, invert);
  const tracedPx = trace(binary, image.width, image.height);
  return tracedPx
    .map(poly => poly.map(([x, y]) => [
      x * fit.scale + fit.x,
      y * fit.scale + fit.y,
    ]))
    .map(p => simplify(p, detailMm))
    .filter(p => p.length >= 2);
}
