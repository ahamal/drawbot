// Trace mode: edges. Sobel gradient magnitude → threshold → marching
// squares contour around the edge bands. Picks up boundaries from
// chromatic transitions too, not just luminance — works on colorful
// photos where plain threshold collapses everything to similar gray.
//
// `thresholdVal` is the gradient-magnitude cutoff (~0–360 dynamic
// range); a higher value keeps only stronger edges.

import { trace } from './trace.js';
import { simplify } from './simplify.js';

export function edges(image, { thresholdVal, detailMm, invert, fit }) {
  const { data, width: w, height: h } = image;

  // Luminance pass — Sobel only cares about a single intensity channel.
  const gray = new Float32Array(w * h);
  for (let i = 0; i < w * h; i++) {
    const r = data[i * 4];
    const g = data[i * 4 + 1];
    const b = data[i * 4 + 2];
    gray[i] = 0.299 * r + 0.587 * g + 0.114 * b;
  }

  // Sobel: gradient magnitude per pixel, then threshold.
  const binary = new Uint8Array(w * h);
  for (let y = 1; y < h - 1; y++) {
    for (let x = 1; x < w - 1; x++) {
      const tl = gray[(y - 1) * w + (x - 1)];
      const tc = gray[(y - 1) * w +  x     ];
      const tr = gray[(y - 1) * w + (x + 1)];
      const ml = gray[ y      * w + (x - 1)];
      const mr = gray[ y      * w + (x + 1)];
      const bl = gray[(y + 1) * w + (x - 1)];
      const bc = gray[(y + 1) * w +  x     ];
      const br = gray[(y + 1) * w + (x + 1)];
      const gx = -tl + tr - 2 * ml + 2 * mr - bl + br;
      const gy = -tl - 2 * tc - tr + bl + 2 * bc + br;
      const mag = Math.sqrt(gx * gx + gy * gy);
      const isEdge = mag > thresholdVal;
      binary[y * w + x] = (invert ? !isEdge : isEdge) ? 1 : 0;
    }
  }

  const tracedPx = trace(binary, w, h);
  return tracedPx
    .map(poly => poly.map(([x, y]) => [
      x * fit.scale + fit.x,
      y * fit.scale + fit.y,
    ]))
    .map(p => simplify(p, detailMm))
    .filter(p => p.length >= 2);
}
