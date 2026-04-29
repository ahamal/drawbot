// Serialization to/from SVG. Strokes are polylines in mm, so SVG is the
// natural format — viewable anywhere, half-compatible with other tools.
//
// On save: write `<polyline>` elements inside an SVG with viewBox set
// to the union of paper bounds and stroke bounds. Drawbot-specific
// metadata (paper preset name, format version) lives in namespaced
// attributes that other SVG viewers ignore.
//
// On load: extract `<polyline>` and `<line>` elements only. We don't
// flatten `<path>` curves, apply `<g>` transforms, or otherwise honor
// the full SVG spec — that's enough for round-tripping our own files
// and for simple SVGs from elsewhere. Anything else is silently ignored.

const NS = 'https://github.com/ahamal/drawbot';
const VERSION = '1';

const fmt = (n) => Number(n.toFixed(3)).toString();

export function strokesToSvg(strokes, paper) {
  let minX = 0, minY = 0, maxX = paper.w, maxY = paper.h;
  for (const s of strokes) {
    for (const [x, y] of s.points) {
      if (x < minX) minX = x;
      if (y < minY) minY = y;
      if (x > maxX) maxX = x;
      if (y > maxY) maxY = y;
    }
  }
  const w = maxX - minX;
  const h = maxY - minY;

  const polys = strokes
    .map(s => `  <polyline points="${s.points.map(p => `${fmt(p[0])},${fmt(p[1])}`).join(' ')}" />`)
    .join('\n');

  return `<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg"
     xmlns:drawbot="${NS}"
     viewBox="${fmt(minX)} ${fmt(minY)} ${fmt(w)} ${fmt(h)}"
     width="${fmt(paper.w)}mm" height="${fmt(paper.h)}mm"
     drawbot:paper="${paper.name}" drawbot:version="${VERSION}"
     fill="none" stroke="black" stroke-width="0.5"
     stroke-linecap="round" stroke-linejoin="round">
${polys}
</svg>
`;
}

// Returns { polylines: [[ [x,y], ... ], ...], paperName: string|null }
// Throws on parse error.
export function svgToStrokes(svgText) {
  const parser = new DOMParser();
  const doc = parser.parseFromString(svgText, 'image/svg+xml');
  if (doc.querySelector('parsererror')) {
    throw new Error('Not a valid SVG file');
  }

  const polylines = [];

  for (const pl of doc.querySelectorAll('polyline')) {
    const pts = parsePoints(pl.getAttribute('points') || '');
    if (pts.length >= 2) polylines.push(pts);
  }

  for (const ln of doc.querySelectorAll('line')) {
    const x1 = parseFloat(ln.getAttribute('x1'));
    const y1 = parseFloat(ln.getAttribute('y1'));
    const x2 = parseFloat(ln.getAttribute('x2'));
    const y2 = parseFloat(ln.getAttribute('y2'));
    if ([x1, y1, x2, y2].every(Number.isFinite)) {
      polylines.push([[x1, y1], [x2, y2]]);
    }
  }

  const svg = doc.querySelector('svg');
  const paperName = svg
    ? svg.getAttributeNS(NS, 'paper') || svg.getAttribute('drawbot:paper') || null
    : null;

  return { polylines, paperName };
}

function parsePoints(str) {
  const nums = str.split(/[\s,]+/).filter(s => s.length > 0).map(parseFloat);
  const points = [];
  for (let i = 0; i + 1 < nums.length; i += 2) {
    if (Number.isFinite(nums[i]) && Number.isFinite(nums[i + 1])) {
      points.push([nums[i], nums[i + 1]]);
    }
  }
  return points;
}
