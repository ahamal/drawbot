// Shared state for the drawing flow.
//
// strokes   — the polylines on the paper. Single source of truth, with
//             undo/redo history (full snapshots; cheap for a pen plotter
//             where stroke counts stay in the hundreds).
// selection — set of stroke ids currently selected (Select tool).
// viewport  — SVG viewBox in mm: { x, y, w, h }. Pan/zoom of the canvas.
// paper     — active paper size: { name, w, h }. Persisted to localStorage.

import { writable } from 'svelte/store';

const STORAGE_KEY = 'drawbot.strokes';
let nextId = 1;
const newId = () => nextId++;

// Try to restore strokes from localStorage on first load. Format is
// `[{ id, points: [[x, y], ...] }, ...]` — the same shape we store
// in-memory, so it round-trips cleanly.
function loadStored() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return [];
    const parsed = JSON.parse(raw);
    if (!Array.isArray(parsed)) return [];
    const valid = [];
    let maxId = 0;
    for (const s of parsed) {
      if (
        s && typeof s.id === 'number' && Array.isArray(s.points) &&
        s.points.length >= 2 && s.points.every(p => Array.isArray(p) && p.length === 2 && Number.isFinite(p[0]) && Number.isFinite(p[1]))
      ) {
        valid.push(s);
        if (s.id > maxId) maxId = s.id;
      }
    }
    nextId = maxId + 1;
    return valid;
  } catch (_e) {
    return [];
  }
}

function createStrokeStore() {
  const initial = loadStored();
  const { subscribe, update, set } = writable(initial);

  const past = [];
  const future = [];
  const MAX_HISTORY = 100;
  let current = initial;

  // Auto-save (debounced) on every change. Survives refresh.
  let saveTimer = null;
  subscribe(s => {
    current = s;
    if (saveTimer) clearTimeout(saveTimer);
    saveTimer = setTimeout(() => {
      try { localStorage.setItem(STORAGE_KEY, JSON.stringify(s)); }
      catch (_e) {}
    }, 200);
  });

  function snapshot() {
    past.push(current);
    if (past.length > MAX_HISTORY) past.shift();
    future.length = 0;
  }

  return {
    subscribe,

    add(points) {
      if (!points || points.length < 2) return null;
      snapshot();
      const id = newId();
      update(s => [...s, { id, points }]);
      return id;
    },

    addPolylines(polylines) {
      const valid = (polylines || []).filter(p => p && p.length >= 2);
      if (valid.length === 0) return [];
      snapshot();
      const ids = [];
      update(s => {
        const next = [...s];
        for (const pts of valid) {
          const id = newId();
          ids.push(id);
          next.push({ id, points: pts });
        }
        return next;
      });
      return ids;
    },

    remove(ids) {
      if (!ids || ids.length === 0) return;
      snapshot();
      const drop = new Set(ids);
      update(s => s.filter(p => !drop.has(p.id)));
    },

    translate(ids, dx, dy) {
      if (dx === 0 && dy === 0) return;
      snapshot();
      const move = new Set(ids);
      update(s => s.map(p =>
        move.has(p.id)
          ? { ...p, points: p.points.map(([x, y]) => [x + dx, y + dy]) }
          : p
      ));
    },

    setPoints(id, points) {
      snapshot();
      update(s => s.map(p => p.id === id ? { ...p, points } : p));
    },

    transform(ids, fn) {
      if (typeof fn !== 'function' || !ids || ids.length === 0) return;
      snapshot();
      const set = new Set(ids);
      update(s => s.map(p =>
        set.has(p.id) ? { ...p, points: p.points.map(fn) } : p
      ));
    },

    clear() {
      if (current.length === 0) return;
      snapshot();
      set([]);
    },

    undo() {
      if (past.length === 0) return;
      future.push(current);
      const prev = past.pop();
      set(prev);
    },

    redo() {
      if (future.length === 0) return;
      past.push(current);
      const next = future.pop();
      set(next);
    },
  };
}

export const strokes = createStrokeStore();

export const selection = writable(new Set());

// One-shot edit mode: when true, the next click on a vertex handle
// removes that point (instead of starting a drag). Auto-clears after
// the click. Toggled by the "Delete point" button in DrawView.
export const pointDeleteMode = writable(false);

export const viewport = writable({ x: -20, y: -20, w: 250, h: 337 });

const DEFAULT_PAPER = { name: 'A5', w: 148, h: 210 };
const storedPaper = (() => {
  try {
    const s = JSON.parse(localStorage.getItem('drawbot.paper') || 'null');
    if (s && s.w > 0 && s.h > 0 && s.name) return s;
  } catch (_e) {}
  return null;
})();

export const paper = writable(storedPaper || DEFAULT_PAPER);
paper.subscribe(p => {
  if (p) localStorage.setItem('drawbot.paper', JSON.stringify(p));
});

// Plotter telemetry — written by App's poll loop, read by views that
// want to show live status (e.g. the Print floating panel).
export const plotterStatus = writable(null);
export const plotterOnline = writable(false);

// Trace Photo flow. `traceImage` holds the user-picked photo as
// ImageData (downsampled to processing size). `tracePolylines` is the
// live preview produced by TraceView; the topbar's "Add to canvas"
// reads it. `traceStage` lets the topbar render context-aware nav
// buttons for multi-step modes (currently hatch: threshold → hatch).
// All clear when the flow ends.
export const traceImage = writable(null);
export const tracePolylines = writable([]);
// Stage of the in-panel wizard. TraceView owns transitions; App.svelte
// uses it only to decide whether the topbar shows Cancel (always) or
// also any other button (it doesn't, currently).
export const traceStage = writable('type'); // 'type' | 'threshold' | 'lines'
