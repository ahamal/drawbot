// Ramer-Douglas-Peucker polyline simplification.
//
// For each segment of the polyline (between currently-kept points), find
// the intermediate point furthest from that segment. If it's farther
// than `epsilon`, keep it and recurse on the two halves. Otherwise drop
// all intermediate points. Endpoints are always preserved, which means
// closed polylines stay closed.

export function simplify(points, epsilon) {
  if (points.length < 3 || epsilon <= 0) return points;

  const eps2 = epsilon * epsilon;
  const keep = new Uint8Array(points.length);
  keep[0] = 1;
  keep[points.length - 1] = 1;

  rdp(points, 0, points.length - 1, eps2, keep);

  const out = [];
  for (let i = 0; i < points.length; i++) {
    if (keep[i]) out.push(points[i]);
  }
  return out;
}

function rdp(pts, start, end, eps2, keep) {
  let maxD2 = 0;
  let idx = -1;
  for (let i = start + 1; i < end; i++) {
    const d2 = distSqPointToSeg(pts[i], pts[start], pts[end]);
    if (d2 > maxD2) { maxD2 = d2; idx = i; }
  }
  if (maxD2 > eps2 && idx >= 0) {
    keep[idx] = 1;
    rdp(pts, start, idx, eps2, keep);
    rdp(pts, idx, end, eps2, keep);
  }
}

function distSqPointToSeg(p, a, b) {
  const dx = b[0] - a[0];
  const dy = b[1] - a[1];
  const len2 = dx * dx + dy * dy;
  if (len2 === 0) {
    const ex = p[0] - a[0], ey = p[1] - a[1];
    return ex * ex + ey * ey;
  }
  let t = ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / len2;
  t = Math.max(0, Math.min(1, t));
  const cx = a[0] + t * dx;
  const cy = a[1] + t * dy;
  const ex = p[0] - cx, ey = p[1] - cy;
  return ex * ex + ey * ey;
}
