// Convert polylines (arrays of [x, y] points in mm) into G-code text.
//
// Convention per polyline: pen up, rapid to start, pen down, linear segments.
// Final M5 lifts the pen at end of job.

const fmt = (n) => Number(n).toFixed(2);

export function polylinesToGcode(polylines) {
  const out = ['G90'];   // absolute coordinates

  for (const poly of polylines) {
    if (poly.length < 2) continue;
    const [x0, y0] = poly[0];
    out.push('M5');
    out.push(`G0 X${fmt(x0)} Y${fmt(y0)}`);
    out.push('M3');
    for (let i = 1; i < poly.length; i++) {
      const [x, y] = poly[i];
      out.push(`G1 X${fmt(x)} Y${fmt(y)}`);
    }
  }
  out.push('M5');
  return out.join('\n') + '\n';
}
