// Marching-squares contour tracer.
//
// Input: a binary grid (0 or 1) of size `width × height`. Output:
// polylines in pixel-space coords (sub-pixel: contour points sit on
// cell mid-edges, so values like 12.5 are normal). Closed contours
// come back as polylines whose first and last points are equal.
//
// Algorithm: for each 2×2 cell of pixels we compute a 4-bit case (one
// bit per corner = 1 if foreground). Each case dictates which mid-edges
// of the cell carry contour. We emit those segments and then link them
// into polylines by matching shared endpoints.
//
// Saddle cases (5 and 10) — where opposite corners are foreground —
// are resolved arbitrarily; for our pen-plotter use case the visual
// difference is negligible.

export function trace(binary, width, height) {
  const segments = [];

  for (let y = 0; y < height - 1; y++) {
    for (let x = 0; x < width - 1; x++) {
      const tl = binary[y       * width + x    ];
      const tr = binary[y       * width + x + 1];
      const br = binary[(y + 1) * width + x + 1];
      const bl = binary[(y + 1) * width + x    ];
      const c = (tl << 3) | (tr << 2) | (br << 1) | bl;

      const N = [x + 0.5, y      ];
      const E = [x + 1,   y + 0.5];
      const S = [x + 0.5, y + 1  ];
      const W = [x,       y + 0.5];

      switch (c) {
        case  1: segments.push([W, S]); break;
        case  2: segments.push([S, E]); break;
        case  3: segments.push([W, E]); break;
        case  4: segments.push([E, N]); break;
        case  5: segments.push([W, N], [S, E]); break;
        case  6: segments.push([S, N]); break;
        case  7: segments.push([W, N]); break;
        case  8: segments.push([N, W]); break;
        case  9: segments.push([N, S]); break;
        case 10: segments.push([N, E], [W, S]); break;
        case 11: segments.push([N, E]); break;
        case 12: segments.push([E, W]); break;
        case 13: segments.push([E, S]); break;
        case 14: segments.push([S, W]); break;
        // 0 and 15: cell is wholly outside / inside; no contour.
      }
    }
  }

  return linkSegments(segments);
}

function key(p) { return `${p[0]}|${p[1]}`; }

function linkSegments(segments) {
  // For each endpoint, the segments that touch it.
  const endpoints = new Map();
  for (let i = 0; i < segments.length; i++) {
    const [a, b] = segments[i];
    pushTo(endpoints, key(a), i);
    pushTo(endpoints, key(b), i);
  }

  const used = new Uint8Array(segments.length);
  const polylines = [];

  for (let i = 0; i < segments.length; i++) {
    if (used[i]) continue;
    used[i] = 1;
    const poly = [segments[i][0], segments[i][1]];
    extend(poly, endpoints, segments, used, true);
    extend(poly, endpoints, segments, used, false);
    polylines.push(poly);
  }

  return polylines;
}

function pushTo(map, k, v) {
  const arr = map.get(k);
  if (arr) arr.push(v);
  else map.set(k, [v]);
}

function extend(poly, endpoints, segments, used, forward) {
  while (true) {
    const tip = forward ? poly[poly.length - 1] : poly[0];
    const candidates = endpoints.get(key(tip));
    if (!candidates) return;
    let next = -1;
    for (const j of candidates) {
      if (!used[j]) { next = j; break; }
    }
    if (next === -1) return;
    used[next] = 1;
    const seg = segments[next];
    const other = key(seg[0]) === key(tip) ? seg[1] : seg[0];
    if (forward) poly.push(other);
    else         poly.unshift(other);
  }
}
