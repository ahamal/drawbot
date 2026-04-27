// Polyline generators for test shapes. All sizes in mm, top-left origin,
// Y increases downward — matches the firmware's coordinate convention.
//
// Each function returns an array of polylines. A polyline is an array of
// [x, y] points. Multiple polylines means the pen lifts between them.

export const A5_W = 148;
export const A5_H = 210;
const CX = A5_W / 2;
const CY = A5_H / 2;

export function line(x1 = 20, y1 = 30, x2 = A5_W - 20, y2 = A5_H - 30) {
  return [[ [x1, y1], [x2, y2] ]];
}

export function square(side = 50, cx = CX, cy = CY) {
  const h = side / 2;
  return [[
    [cx - h, cy - h],
    [cx + h, cy - h],
    [cx + h, cy + h],
    [cx - h, cy + h],
    [cx - h, cy - h],
  ]];
}

export function circle(r = 30, cx = CX, cy = CY, segments = 32) {
  const pts = [];
  for (let i = 0; i <= segments; i++) {
    const t = (i / segments) * 2 * Math.PI;
    pts.push([cx + r * Math.cos(t), cy + r * Math.sin(t)]);
  }
  return [pts];
}

export function spiral(maxR = 40, turns = 5, cx = CX, cy = CY, segPerTurn = 32) {
  const total = turns * segPerTurn;
  const pts = [];
  for (let i = 0; i <= total; i++) {
    const t = (i / total) * turns * 2 * Math.PI;
    const r = (i / total) * maxR;
    pts.push([cx + r * Math.cos(t), cy + r * Math.sin(t)]);
  }
  return [pts];
}
