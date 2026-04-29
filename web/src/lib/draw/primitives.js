// Polyline generators for test shapes. All sizes in mm. Pure functions —
// the caller decides where on the paper they go and how big they are.
//
// Each function returns an array of polylines. A polyline is an array of
// [x, y] points. Multiple polylines means the pen lifts between them.

export function line(x1, y1, x2, y2) {
  return [[ [x1, y1], [x2, y2] ]];
}

export function square(cx, cy, side) {
  const h = side / 2;
  return [[
    [cx - h, cy - h],
    [cx + h, cy - h],
    [cx + h, cy + h],
    [cx - h, cy + h],
    [cx - h, cy - h],
  ]];
}

export function circle(cx, cy, r, segments = 32) {
  const pts = [];
  for (let i = 0; i <= segments; i++) {
    const t = (i / segments) * 2 * Math.PI;
    pts.push([cx + r * Math.cos(t), cy + r * Math.sin(t)]);
  }
  return [pts];
}

export function spiral(cx, cy, maxR, turns = 5, segPerTurn = 32) {
  const total = turns * segPerTurn;
  const pts = [];
  for (let i = 0; i <= total; i++) {
    const t = (i / total) * turns * 2 * Math.PI;
    const r = (i / total) * maxR;
    pts.push([cx + r * Math.cos(t), cy + r * Math.sin(t)]);
  }
  return [pts];
}

// Paper size presets. Dimensions in mm, portrait orientation.
export const PAPER_PRESETS = [
  { name: 'A5',     w: 148, h: 210 },
  { name: 'A4',     w: 210, h: 297 },
  { name: 'A3',     w: 297, h: 420 },
  { name: 'Letter', w: 216, h: 279 },
];
