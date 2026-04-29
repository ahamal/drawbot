// Trace mode: squiggle. Single continuous serpentine polyline that
// sweeps the image row by row, with sine-wave amplitude tracking local
// darkness. Dark areas have tall waves, light areas flatten to a near-
// straight line.
//
// Each row reverses direction (left→right, then right→left, …) so the
// end of one row is adjacent to the start of the next — the whole
// shading is one polyline with zero pen-ups, which is the killer
// feature for slow plotters.

export function squiggle(image, { rowSpacingMm, periodMm, ampMm, invert, fit }) {
  const { data, width: w, height: h } = image;
  const rowSpacingPx = Math.max(1, rowSpacingMm / fit.scale);
  const periodPx = Math.max(2, periodMm / fit.scale);
  const ampPx = Math.max(0, ampMm / fit.scale);

  function darknessAt(x, y) {
    const cx = Math.max(0, Math.min(w - 1, x | 0));
    const cy = Math.max(0, Math.min(h - 1, y | 0));
    const i = (cy * w + cx) * 4;
    const lum = 0.299 * data[i] + 0.587 * data[i + 1] + 0.114 * data[i + 2];
    const dark = (255 - lum) / 255;     // 0..1
    return invert ? 1 - dark : dark;
  }

  const points = [];
  let leftToRight = true;
  // Sample step: ~1 px gives smooth waves; smaller would mean more
  // points than the plotter can usefully resolve.
  const stepPx = 1;

  for (let row = 0; row < h; row += rowSpacingPx) {
    if (leftToRight) {
      for (let x = 0; x <= w; x += stepPx) {
        const dark = darknessAt(x, row);
        const y = row + ampPx * dark * Math.sin((x / periodPx) * 2 * Math.PI);
        points.push([x, y]);
      }
    } else {
      for (let x = w; x >= 0; x -= stepPx) {
        const dark = darknessAt(x, row);
        const y = row + ampPx * dark * Math.sin((x / periodPx) * 2 * Math.PI);
        points.push([x, y]);
      }
    }
    leftToRight = !leftToRight;
  }

  const inMm = points.map(([x, y]) => [
    x * fit.scale + fit.x,
    y * fit.scale + fit.y,
  ]);
  return [inMm];
}
