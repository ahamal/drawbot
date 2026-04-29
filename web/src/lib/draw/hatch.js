// Trace mode: hatch. Two-step:
//   1. `thresholdVal` defines the "hatchable zone" — pixels lighter
//      than this are paper-white and never get a line.
//   2. Within the zone, tonal modulation applies. Each scan line
//      cycles through N tone levels; lighter in-zone pixels only pass
//      the most permissive levels (sparse), darker pixels pass them
//      all (dense). Smooth gradation from a single algorithm.
//
// Cross-hatching is just running the whole thing again at angle + 90°
// and concatenating.

export function hatch(image, params) {
  const { thresholdVal, spacingMm, levels, angle, crosshatch, invert, fit } = params;
  const { data, width: w, height: h } = image;
  const finestSpacingPx = Math.max(1, spacingMm / fit.scale);
  const N = Math.max(1, levels | 0);

  // Luminance, 0 (black) to 255 (white).
  const gray = new Float32Array(w * h);
  for (let i = 0; i < w * h; i++) {
    const r = data[i * 4];
    const g = data[i * 4 + 1];
    const b = data[i * 4 + 2];
    gray[i] = 0.299 * r + 0.587 * g + 0.114 * b;
  }

  const linesPx = scan(gray, w, h, finestSpacingPx, N, thresholdVal, angle, invert);
  const all = crosshatch
    ? linesPx.concat(scan(gray, w, h, finestSpacingPx, N, thresholdVal, angle + 90, invert))
    : linesPx;

  return all
    .map(poly => poly.map(([x, y]) => [
      x * fit.scale + fit.x,
      y * fit.scale + fit.y,
    ]))
    .filter(p => p.length >= 2);
}

function scan(gray, w, h, finestSpacingPx, levels, thresholdVal, angleDeg, invert) {
  const polylines = [];
  const theta = angleDeg * Math.PI / 180;
  const dx = Math.cos(theta);
  const dy = Math.sin(theta);
  const nx = -dy;
  const ny =  dx;

  const corners = [[0, 0], [w, 0], [0, h], [w, h]];
  let pMin = Infinity, pMax = -Infinity;
  let tMin = Infinity, tMax = -Infinity;
  for (const [cx, cy] of corners) {
    const p = cx * nx + cy * ny;
    const t = cx * dx + cy * dy;
    if (p < pMin) pMin = p;
    if (p > pMax) pMax = p;
    if (t < tMin) tMin = t;
    if (t > tMax) tMax = t;
  }

  // Maps in-zone luminance → 0..255 intensity, so the tonal modulation
  // gets full dynamic range regardless of where the threshold falls.
  const zoneSize = Math.max(1, invert ? 255 - thresholdVal : thresholdVal);

  let rowIdx = 0;
  for (let p = pMin; p <= pMax; p += finestSpacingPx) {
    const level = rowIdx % levels;
    const tone = ((level + 1) / (levels + 1)) * 255;

    let runStart = null;
    let lastInside = null;
    for (let t = tMin; t <= tMax; t += 1) {
      const x = Math.round(p * nx + t * dx);
      const y = Math.round(p * ny + t * dy);
      const inside = x >= 0 && x < w && y >= 0 && y < h;
      let active = false;
      if (inside) {
        const lum = gray[y * w + x];
        const inZone = invert ? lum >= thresholdVal : lum <= thresholdVal;
        if (inZone) {
          const intensity = invert
            ? ((lum - thresholdVal) / zoneSize) * 255
            : ((thresholdVal - lum) / zoneSize) * 255;
          active = intensity > tone;
        }
      }
      if (active) {
        if (runStart === null) runStart = [x, y];
        lastInside = [x, y];
      } else if (runStart !== null) {
        polylines.push([runStart, lastInside]);
        runStart = null;
      }
    }
    if (runStart !== null && lastInside) {
      polylines.push([runStart, lastInside]);
    }
    rowIdx++;
  }

  return polylines;
}
