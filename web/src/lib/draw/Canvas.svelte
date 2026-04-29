<script>
  // Shared canvas for the drawing flow. SVG, in mm units.
  // Renders paper marks, strokes, handles, pending polyline, and the
  // selection rect. Handles wheel zoom and three tools: draw, select,
  // drag. Dispatches 'drawcommit' when a polyline is committed/canceled
  // so the parent can return the user to Select.

  import { createEventDispatcher } from 'svelte';
  import { strokes, selection, viewport, paper, pointDeleteMode } from './store.js';
  import { PAPER_PRESETS } from './primitives.js';

  export let tool = 'drag';

  const dispatch = createEventDispatcher();

  let svgEl;

  // Pending polyline being built (Draw tool). Array of fixed points; the
  // preview point follows the cursor and is rendered separately.
  let pendingPoly = null;
  let previewPoint = null;
  let drawDownInfo = null;    // { screenX, screenY } during a draw-tool press

  // Drag state (Select / Drag tools).
  let dragMode = null;        // 'translate' | 'rect' | 'pan' | 'point' | 'resize' | null
  let dragStart = null;       // mm at start
  let dragCurrent = null;     // mm at current
  let panOrigin = null;
  let pointEdit = null;
  let resizeAnchor = null;    // [x, y] mm, fixed corner during resize
  let resizeOrigCorner = null;// [x, y] mm, dragged corner at drag start
  let resizeFactor = 1;       // current uniform scale during drag

  // Pre-drag intent: filled on pointerdown for select-tool actions, only
  // promoted to dragMode after the screen-space distance crosses the
  // threshold. Until then, pointerup is a click (no drag side-effects).
  let pendingDrag = null;     // { kind, screenX, screenY, p, ... }
  const DRAG_THRESHOLD = 16;  // squared px (= 4px movement)

  // Ghost insertion point: shown when hovering near a segment of the
  // single selected stroke. Click to insert a vertex there.
  let ghostPoint = null;      // { segIndex, point: [x, y] }

  $: singleSelected = ($selection.size === 1)
    ? $strokes.find(s => s.id === [...$selection][0]) ?? null
    : null;

  // Bounding box around current selection (in mm). Drives the resize
  // handles. Computed from underlying stroke points (not the live
  // preview transforms) so we always have the source-of-truth bounds.
  $: selectionBounds = computeSelectionBounds($strokes, $selection);

  // Show the bounding box + corner handles when:
  //   * 2+ strokes are selected, or
  //   * 1 stroke with too many points for vertex editing to be useful
  //     (image traces, primitives like circle/spiral, squiggle output).
  // Otherwise we fall back to vertex handles for fine-tuning a small line.
  $: showBoundingBox = tool === 'select' && selectionBounds && (
       $selection.size >= 2 ||
       ($selection.size === 1 && singleSelected && singleSelected.points.length > 50)
     );
  $: showVertexHandles = tool === 'select' && singleSelected &&
                         singleSelected.points.length <= 50;

  // Live-scaled bounds during a resize drag, so the bounding box and
  // its corner handles track the cursor.
  $: displayBounds = (dragMode === 'resize' && resizeAnchor && selectionBounds)
    ? scaleBounds(selectionBounds, resizeAnchor, resizeFactor)
    : selectionBounds;

  function computeSelectionBounds(allStrokes, sel) {
    if (sel.size === 0) return null;
    let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
    for (const s of allStrokes) {
      if (!sel.has(s.id)) continue;
      for (const [x, y] of s.points) {
        if (x < minX) minX = x;
        if (y < minY) minY = y;
        if (x > maxX) maxX = x;
        if (y > maxY) maxY = y;
      }
    }
    if (minX === Infinity) return null;
    return { minX, minY, maxX, maxY };
  }

  function scaleBounds(b, anchor, s) {
    return {
      minX: anchor[0] + (b.minX - anchor[0]) * s,
      minY: anchor[1] + (b.minY - anchor[1]) * s,
      maxX: anchor[0] + (b.maxX - anchor[0]) * s,
      maxY: anchor[1] + (b.maxY - anchor[1]) * s,
    };
  }

  $: dragOffset = (dragMode === 'translate' && dragStart && dragCurrent)
    ? [dragCurrent[0] - dragStart[0], dragCurrent[1] - dragStart[1]]
    : [0, 0];

  $: rect = (dragMode === 'rect' && dragStart && dragCurrent)
    ? bounds(dragStart, dragCurrent)
    : null;

  // If the parent flips the tool while we're mid-polyline, auto-commit
  // so we don't lose work or leave a dangling preview.
  $: if (tool !== 'draw' && pendingPoly !== null) commitDraw();

  const MAX_W = Math.max(...PAPER_PRESETS.map(p => p.w));
  const MAX_H = Math.max(...PAPER_PRESETS.map(p => p.h));

  function bounds(a, b) {
    return {
      x0: Math.min(a[0], b[0]), y0: Math.min(a[1], b[1]),
      x1: Math.max(a[0], b[0]), y1: Math.max(a[1], b[1]),
    };
  }

  // Convert client (screen) coords to viewBox mm coords using the SVG's
  // own CTM. This correctly accounts for `preserveAspectRatio="meet"`
  // letterboxing — naive `(clientX - r.left) / r.width * viewport.w`
  // math breaks whenever the container aspect ≠ viewport aspect.
  function pointToMm(e) {
    const ctm = svgEl.getScreenCTM();
    if (!ctm) return [$viewport.x, $viewport.y];
    const pt = svgEl.createSVGPoint();
    pt.x = e.clientX; pt.y = e.clientY;
    const svgPt = pt.matrixTransform(ctm.inverse());
    return [svgPt.x, svgPt.y];
  }

  function strokeBounds(s) {
    let x0 = Infinity, y0 = Infinity, x1 = -Infinity, y1 = -Infinity;
    for (const [x, y] of s.points) {
      if (x < x0) x0 = x; if (x > x1) x1 = x;
      if (y < y0) y0 = y; if (y > y1) y1 = y;
    }
    return { x0, y0, x1, y1 };
  }

  function rectsOverlap(a, b) {
    return !(a.x1 < b.x0 || a.x0 > b.x1 || a.y1 < b.y0 || a.y0 > b.y1);
  }

  function pointsAttr(points) {
    return points.map(p => `${p[0]},${p[1]}`).join(' ');
  }

  function findStrokeId(target) {
    let el = target;
    while (el && el !== svgEl) {
      if (el.dataset && el.dataset.strokeId) return Number(el.dataset.strokeId);
      el = el.parentElement;
    }
    return null;
  }

  function findHandleIndex(target) {
    let el = target;
    while (el && el !== svgEl) {
      if (el.dataset && el.dataset.pointIndex !== undefined) {
        return Number(el.dataset.pointIndex);
      }
      el = el.parentElement;
    }
    return null;
  }

  function findInsertIndex(target) {
    let el = target;
    while (el && el !== svgEl) {
      if (el.dataset && el.dataset.insertIdx !== undefined) {
        return Number(el.dataset.insertIdx);
      }
      el = el.parentElement;
    }
    return null;
  }

  function findCorner(target) {
    let el = target;
    while (el && el !== svgEl) {
      if (el.dataset && el.dataset.corner) return el.dataset.corner;
      el = el.parentElement;
    }
    return null;
  }

  // Maps each corner identifier to its position and the opposite corner
  // (the anchor — which stays fixed while the dragged corner moves).
  function cornerPositions(b, corner) {
    const map = {
      tl: { c: [b.minX, b.minY], a: [b.maxX, b.maxY] },
      tr: { c: [b.maxX, b.minY], a: [b.minX, b.maxY] },
      br: { c: [b.maxX, b.maxY], a: [b.minX, b.minY] },
      bl: { c: [b.minX, b.maxY], a: [b.maxX, b.minY] },
    };
    return map[corner];
  }

  function projectOnSegment(p, a, b) {
    const dx = b[0] - a[0], dy = b[1] - a[1];
    const len2 = dx * dx + dy * dy;
    if (len2 === 0) return [a[0], a[1]];
    let t = ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / len2;
    t = Math.max(0, Math.min(1, t));
    return [a[0] + t * dx, a[1] + t * dy];
  }

  function findClosestSegment(points, p) {
    let best = null;
    for (let i = 0; i < points.length - 1; i++) {
      const proj = projectOnSegment(p, points[i], points[i + 1]);
      const dist = Math.hypot(p[0] - proj[0], p[1] - proj[1]);
      if (!best || dist < best.dist) best = { segIndex: i, proj, dist };
    }
    return best;
  }

  function updateGhost(p) {
    if (tool !== 'select' || !singleSelected || dragMode || pendingDrag || $pointDeleteMode) {
      ghostPoint = null;
      return;
    }
    // Hide near existing vertices so the ghost doesn't compete with handles.
    const vertexAvoid = $viewport.w / 60;
    for (const pt of singleSelected.points) {
      if (Math.hypot(p[0] - pt[0], p[1] - pt[1]) < vertexAvoid) {
        ghostPoint = null;
        return;
      }
    }
    const showRadius = $viewport.w / 90;
    const closest = findClosestSegment(singleSelected.points, p);
    if (closest && closest.dist < showRadius) {
      ghostPoint = { segIndex: closest.segIndex, point: closest.proj };
    } else {
      ghostPoint = null;
    }
  }

  $: if (tool !== 'select' || !singleSelected) ghostPoint = null;

  // Auto-exit delete mode if selection / tool / point count no longer fits.
  $: if (
    tool !== 'select' ||
    !singleSelected ||
    singleSelected.points.length < 3
  ) pointDeleteMode.set(false);

  function commitDraw() {
    if (pendingPoly && pendingPoly.length >= 2) {
      strokes.add(pendingPoly);
    }
    pendingPoly = null;
    previewPoint = null;
    dispatch('drawcommit');
  }

  function cancelDraw() {
    pendingPoly = null;
    previewPoint = null;
    dispatch('drawcommit');
  }

  function onDown(e) {
    e.preventDefault();
    const p = pointToMm(e);

    // Middle-click pans regardless of tool.
    if (e.button === 1 || tool === 'drag') {
      svgEl.setPointerCapture(e.pointerId);
      const ctm = svgEl.getScreenCTM();
      dragMode = 'pan';
      panOrigin = {
        screenX: e.clientX, screenY: e.clientY,
        viewport: { ...$viewport },
        pxPerMmX: ctm ? ctm.a : 1,
        pxPerMmY: ctm ? ctm.d : 1,
      };
      return;
    }

    if (tool === 'draw') {
      svgEl.setPointerCapture(e.pointerId);
      drawDownInfo = { screenX: e.clientX, screenY: e.clientY };
      if (pendingPoly === null) {
        pendingPoly = [p];
      } else {
        const last = pendingPoly[pendingPoly.length - 1];
        if (Math.hypot(p[0] - last[0], p[1] - last[1]) > 0.3) {
          pendingPoly = [...pendingPoly, p];
        }
      }
      previewPoint = p;
      return;
    }

    // Select tool — capture pointer and stash a pending intent. We
    // promote to a real drag mode only after the threshold is crossed.
    svgEl.setPointerCapture(e.pointerId);

    // One-shot delete mode: any click exits the mode, and a click on
    // a handle deletes that vertex.
    if ($pointDeleteMode) {
      pointDeleteMode.set(false);
      const idx = findHandleIndex(e.target);
      if (idx != null && singleSelected && singleSelected.points.length > 2) {
        const newPts = singleSelected.points.filter((_, i) => i !== idx);
        strokes.setPoints(singleSelected.id, newPts);
      }
      return;
    }

    // Corner of the selection bounding box → uniform-scale resize.
    const corner = findCorner(e.target);
    if (corner && selectionBounds) {
      const cp = cornerPositions(selectionBounds, corner);
      pendingDrag = {
        kind: 'resize',
        screenX: e.clientX, screenY: e.clientY,
        p,
        corner: cp.c,
        anchor: cp.a,
      };
      return;
    }

    const handleIdx = findHandleIndex(e.target);
    if (handleIdx != null && singleSelected) {
      // Alt-click deletes the vertex (kept ≥2 so it's still a polyline).
      if (e.altKey) {
        if (singleSelected.points.length > 2) {
          const newPts = singleSelected.points.filter((_, i) => i !== handleIdx);
          strokes.setPoints(singleSelected.id, newPts);
        }
        return;
      }
      pendingDrag = {
        kind: 'point',
        screenX: e.clientX, screenY: e.clientY,
        p, id: singleSelected.id, index: handleIdx,
      };
      return;
    }

    // Click on the ghost insertion point inserts a vertex there and
    // immediately starts a drag for it, so the user can position it in
    // one motion.
    const insertIdx = findInsertIndex(e.target);
    if (insertIdx != null && singleSelected && ghostPoint) {
      const sel = singleSelected;
      const newIndex = insertIdx + 1;
      const newPts = [...sel.points];
      newPts.splice(newIndex, 0, ghostPoint.point);
      strokes.setPoints(sel.id, newPts);
      ghostPoint = null;
      pendingDrag = {
        kind: 'point',
        screenX: e.clientX, screenY: e.clientY,
        p, id: sel.id, index: newIndex,
      };
      return;
    }

    const hitId = findStrokeId(e.target);
    if (hitId != null) {
      // Selection update happens immediately on click (no threshold).
      if (e.shiftKey) {
        const next = new Set($selection);
        if (next.has(hitId)) next.delete(hitId);
        else next.add(hitId);
        selection.set(next);
      } else if (!$selection.has(hitId)) {
        selection.set(new Set([hitId]));
      }
      if ($selection.has(hitId)) {
        pendingDrag = {
          kind: 'translate',
          screenX: e.clientX, screenY: e.clientY,
          p,
        };
      }
    } else {
      // Click on empty: deselect immediately. Drag will start a rect.
      if (!e.shiftKey) selection.set(new Set());
      pendingDrag = {
        kind: 'rect',
        screenX: e.clientX, screenY: e.clientY,
        p, shift: e.shiftKey,
      };
    }
  }

  function onMove(e) {
    if (tool === 'draw') {
      previewPoint = pointToMm(e);
      return;
    }

    if (dragMode === 'pan' && panOrigin) {
      e.preventDefault();
      const dxScreen = e.clientX - panOrigin.screenX;
      const dyScreen = e.clientY - panOrigin.screenY;
      const dxMm = dxScreen / panOrigin.pxPerMmX;
      const dyMm = dyScreen / panOrigin.pxPerMmY;
      viewport.set({
        x: panOrigin.viewport.x - dxMm,
        y: panOrigin.viewport.y - dyMm,
        w: panOrigin.viewport.w,
        h: panOrigin.viewport.h,
      });
      return;
    }

    if (dragMode === 'point' && pointEdit) {
      e.preventDefault();
      pointEdit = { ...pointEdit, current: pointToMm(e) };
      return;
    }

    if (dragMode === 'translate' || dragMode === 'rect') {
      e.preventDefault();
      dragCurrent = pointToMm(e);
      return;
    }

    if (dragMode === 'resize' && resizeAnchor && resizeOrigCorner) {
      e.preventDefault();
      const cur = pointToMm(e);
      const dx0 = resizeOrigCorner[0] - resizeAnchor[0];
      const dy0 = resizeOrigCorner[1] - resizeAnchor[1];
      const dx1 = cur[0] - resizeAnchor[0];
      const dy1 = cur[1] - resizeAnchor[1];
      const d0 = Math.hypot(dx0, dy0);
      const d1 = Math.hypot(dx1, dy1);
      // Sign: scale stays positive only while the cursor is on the same
      // side of the anchor as the original corner. Going past flips it
      // (which would be a flip-mirror — disallow for now to keep it
      // sane).
      const sign = (dx0 * dx1 + dy0 * dy1) > 0 ? 1 : 0;
      resizeFactor = d0 === 0 ? 1 : Math.max(0.05, sign * d1 / d0);
      return;
    }

    // Promote pending intent to a real drag once we exceed the threshold.
    if (pendingDrag) {
      const dx = e.clientX - pendingDrag.screenX;
      const dy = e.clientY - pendingDrag.screenY;
      if (dx * dx + dy * dy > DRAG_THRESHOLD) {
        e.preventDefault();
        const p = pointToMm(e);
        if (pendingDrag.kind === 'point') {
          dragMode = 'point';
          pointEdit = { id: pendingDrag.id, index: pendingDrag.index, current: p };
        } else if (pendingDrag.kind === 'translate') {
          dragMode = 'translate';
          dragStart = pendingDrag.p;
          dragCurrent = p;
        } else if (pendingDrag.kind === 'rect') {
          dragMode = 'rect';
          dragStart = pendingDrag.p;
          dragCurrent = p;
        } else if (pendingDrag.kind === 'resize') {
          dragMode = 'resize';
          resizeAnchor = pendingDrag.anchor;
          resizeOrigCorner = pendingDrag.corner;
          resizeFactor = 1;
        }
        pendingDrag = null;
      }
      return;
    }

    // Hover preview for ghost vertex insertion.
    updateGhost(pointToMm(e));
  }

  function onLeave() {
    ghostPoint = null;
  }

  function onUp(e) {
    if (tool === 'draw') {
      // Drag-release commits a line; click-release waits for more input.
      if (drawDownInfo) {
        const dx = e.clientX - drawDownInfo.screenX;
        const dy = e.clientY - drawDownInfo.screenY;
        drawDownInfo = null;
        if (dx * dx + dy * dy > DRAG_THRESHOLD && pendingPoly) {
          const upP = pointToMm(e);
          const last = pendingPoly[pendingPoly.length - 1];
          if (Math.hypot(upP[0] - last[0], upP[1] - last[1]) > 0.3) {
            pendingPoly = [...pendingPoly, upP];
          }
          commitDraw();
        }
      }
      return;
    }

    if (dragMode === 'pan') {
      dragMode = null; panOrigin = null;
      // Drag tool is one-shot: panning ends → return to select.
      // Middle-click pans don't trigger this since they don't change `tool`.
      if (tool === 'drag') dispatch('dragcommit');
      return;
    }
    if (dragMode === 'point' && pointEdit) {
      const stroke = $strokes.find(s => s.id === pointEdit.id);
      if (stroke) {
        const newPts = stroke.points.map((pt, i) =>
          i === pointEdit.index ? pointEdit.current : pt
        );
        strokes.setPoints(pointEdit.id, newPts);
      }
      pointEdit = null;
      dragMode = null;
      pendingDrag = null;
      return;
    }
    if (dragMode === 'translate') {
      const [dx, dy] = dragOffset;
      strokes.translate([...$selection], dx, dy);
    } else if (dragMode === 'rect' && rect &&
               (rect.x1 - rect.x0 > 0.5 || rect.y1 - rect.y0 > 0.5)) {
      const next = e.shiftKey ? new Set($selection) : new Set();
      for (const s of $strokes) {
        if (rectsOverlap(strokeBounds(s), rect)) next.add(s.id);
      }
      selection.set(next);
    } else if (dragMode === 'resize' && resizeAnchor && resizeFactor !== 1) {
      const ax = resizeAnchor[0];
      const ay = resizeAnchor[1];
      const s = resizeFactor;
      strokes.transform([...$selection], ([x, y]) => [
        ax + (x - ax) * s,
        ay + (y - ay) * s,
      ]);
    }
    dragMode = null;
    dragStart = null;
    dragCurrent = null;
    resizeAnchor = null;
    resizeOrigCorner = null;
    resizeFactor = 1;
    pendingDrag = null;
  }

  function onDblClick(e) {
    if (tool === 'draw' && pendingPoly) {
      e.preventDefault();
      commitDraw();
    }
  }

  function onKeyDown(e) {
    if (tool === 'draw' && pendingPoly) {
      if (e.key === 'Enter') {
        e.preventDefault();
        commitDraw();
      } else if (e.key === 'Escape') {
        e.preventDefault();
        cancelDraw();
      }
    }
  }

  function onWheel(e) {
    e.preventDefault();
    const cursor = pointToMm(e);
    const factor = e.deltaY < 0 ? 1.1 : 1 / 1.1;
    zoomAt(cursor, factor);
  }

  function zoomAt(c, factor) {
    const newW = $viewport.w / factor;
    const newH = $viewport.h / factor;
    if (newW < 10 || newW > 5000) return;
    viewport.set({
      x: c[0] - (c[0] - $viewport.x) / factor,
      y: c[1] - (c[1] - $viewport.y) / factor,
      w: newW, h: newH,
    });
  }

  function zoomCenter(factor) {
    zoomAt([$viewport.x + $viewport.w / 2, $viewport.y + $viewport.h / 2], factor);
  }

  function fitToPaper() {
    const margin = 20;
    viewport.set({
      x: -margin, y: -margin,
      w: $paper.w + 2 * margin,
      h: $paper.h + 2 * margin,
    });
  }

  $: viewBoxAttr = `${$viewport.x} ${$viewport.y} ${$viewport.w} ${$viewport.h}`;
  $: cursorStyle =
       tool === 'draw' ? 'crosshair'
     : tool === 'drag' ? (dragMode === 'pan' ? 'grabbing' : 'grab')
     :                   'default';
</script>

<svelte:window on:keydown={onKeyDown} />

<div class="canvas-wrap">
  <svg
    bind:this={svgEl}
    viewBox={viewBoxAttr}
    preserveAspectRatio="xMidYMid meet"
    class="paper"
    role="application"
    aria-label="Drawing canvas"
    style:cursor={cursorStyle}
    on:pointerdown={onDown}
    on:pointermove={onMove}
    on:pointerup={onUp}
    on:pointercancel={onUp}
    on:pointerleave={onLeave}
    on:dblclick={onDblClick}
    on:wheel={onWheel}
  >
    <rect class="bg" x="-10000" y="-10000" width="20000" height="20000" />

    <g class="paper-marks">
      <line x1="0" y1="0" x2={MAX_W} y2="0" />
      <line x1="0" y1="0" x2="0"     y2={MAX_H} />
      {#each PAPER_PRESETS as p}
        <line x1={p.w} y1="0"   x2={p.w} y2={p.h} />
        <line x1="0"   y1={p.h} x2={p.w} y2={p.h} />
        <text x="1" y={p.h - 1.5}>{p.name}</text>
      {/each}
    </g>

    {#each $strokes as s (s.id)}
      {@const isSel = $selection.has(s.id)}
      {@const tx = (isSel && dragMode === 'translate') ? dragOffset[0] : 0}
      {@const ty = (isSel && dragMode === 'translate') ? dragOffset[1] : 0}
      {@const pts = (dragMode === 'point' && pointEdit?.id === s.id)
        ? s.points.map((p, i) => i === pointEdit.index ? pointEdit.current : p)
        : s.points}
      {@const transform = (isSel && dragMode === 'resize' && resizeAnchor)
        ? `translate(${resizeAnchor[0]} ${resizeAnchor[1]}) scale(${resizeFactor}) translate(${-resizeAnchor[0]} ${-resizeAnchor[1]})`
        : `translate(${tx} ${ty})`}
      <g
        class="stroke"
        class:selected={isSel}
        data-stroke-id={s.id}
        transform={transform}
      >
        <polyline class="hit"     points={pointsAttr(pts)} />
        <polyline class="visible" points={pointsAttr(pts)} />
      </g>
    {/each}

    {#if showBoundingBox && displayBounds}
      {@const b = displayBounds}
      {@const cornerR = $viewport.w / 200}
      <g class="bbox">
        <rect class="bbox-rect"
          x={b.minX} y={b.minY}
          width={b.maxX - b.minX} height={b.maxY - b.minY} />
        {#each [
          { id: 'tl', x: b.minX, y: b.minY, cur: 'nwse-resize' },
          { id: 'tr', x: b.maxX, y: b.minY, cur: 'nesw-resize' },
          { id: 'br', x: b.maxX, y: b.maxY, cur: 'nwse-resize' },
          { id: 'bl', x: b.minX, y: b.maxY, cur: 'nesw-resize' },
        ] as c (c.id)}
          <g data-corner={c.id} style:cursor={c.cur}>
            <rect class="corner-hit"
              x={c.x - cornerR * 2.5} y={c.y - cornerR * 2.5}
              width={cornerR * 5} height={cornerR * 5} />
            <rect class="corner"
              x={c.x - cornerR} y={c.y - cornerR}
              width={cornerR * 2} height={cornerR * 2} />
          </g>
        {/each}
      </g>
    {/if}

    {#if ghostPoint && tool === 'select' && singleSelected}
      {@const ghostR = $viewport.w / 230}
      <g class="ghost" data-insert-idx={ghostPoint.segIndex}>
        <circle class="ghost-hit" cx={ghostPoint.point[0]} cy={ghostPoint.point[1]} r={ghostR * 2.5} />
        <circle class="ghost-dot" cx={ghostPoint.point[0]} cy={ghostPoint.point[1]} r={ghostR} />
      </g>
    {/if}

    {#if showVertexHandles && singleSelected}
      {@const handlePts = (dragMode === 'point' && pointEdit?.id === singleSelected.id)
        ? singleSelected.points.map((p, i) => i === pointEdit.index ? pointEdit.current : p)
        : singleSelected.points}
      {@const handleR = $viewport.w / 230}
      <g class="handles" class:delete-mode={$pointDeleteMode}>
        {#each handlePts as pt, i}
          <g data-point-index={i}>
            <title>{$pointDeleteMode ? 'Click to delete' : 'Drag to move · Alt-click to remove'}</title>
            <circle class="handle-hit" cx={pt[0]} cy={pt[1]} r={handleR * 2.5} />
            <circle class="handle"     cx={pt[0]} cy={pt[1]} r={handleR} />
          </g>
        {/each}
      </g>
    {/if}

    {#if pendingPoly}
      {@const dotR = $viewport.w / 220}
      {#if pendingPoly.length >= 2}
        <polyline class="pending fixed" points={pointsAttr(pendingPoly)} />
      {/if}
      {#if previewPoint && pendingPoly.length > 0}
        {@const last = pendingPoly[pendingPoly.length - 1]}
        <line class="pending preview"
          x1={last[0]} y1={last[1]}
          x2={previewPoint[0]} y2={previewPoint[1]} />
      {/if}
      {#each pendingPoly as pt}
        <circle class="pending-dot" cx={pt[0]} cy={pt[1]} r={dotR} />
      {/each}
    {/if}

    {#if rect}
      <rect class="select-rect"
        x={rect.x0} y={rect.y0}
        width={rect.x1 - rect.x0}
        height={rect.y1 - rect.y0} />
    {/if}
  </svg>

  <div class="right-overlay">
    <div class="zoom-overlay">
      <button on:click={() => zoomCenter(1.25)}    title="Zoom in">+</button>
      <button on:click={() => zoomCenter(1 / 1.25)} title="Zoom out">−</button>
      <button on:click={fitToPaper}                title="Fit paper">⊡</button>
    </div>
    <slot name="right" />
  </div>

  {#if tool === 'select' && singleSelected}
    <div class="edit-hint" class:delete={$pointDeleteMode}>
      {#if $pointDeleteMode}
        Click a point to delete · Esc to cancel
      {:else}
        Click line to add point · Alt-click handle to remove
      {/if}
    </div>
  {/if}

  <slot />
</div>

<style>
  /* Fills the parent flex container. DrawView/PrintView render this
   * directly inside <main>, so it takes all remaining vertical space
   * below the topbar (which already has its own border-bottom). */
  .canvas-wrap {
    position: relative;
    width: 100%;
    flex: 1 1 auto;
    min-height: 0;
    background: #fff;
    overflow: hidden;
  }

  svg.paper {
    display: block;
    width: 100%;
    height: 100%;
    touch-action: none;
    user-select: none;
  }

  .bg { fill: #fff; pointer-events: all; }

  .paper-marks line {
    stroke: rgba(234, 88, 12, 0.18);
    stroke-width: 1;
    vector-effect: non-scaling-stroke;
    pointer-events: none;
  }
  .paper-marks text {
    font-size: 3.5px;
    fill: rgba(234, 88, 12, 0.45);
    font-family: ui-monospace, "SF Mono", Menlo, monospace;
    pointer-events: none;
  }

  .stroke .visible {
    fill: none;
    stroke: #1a1a1a;
    stroke-width: 1.5;
    stroke-linejoin: round;
    stroke-linecap: round;
    vector-effect: non-scaling-stroke;
    pointer-events: none;
  }
  .stroke .hit {
    fill: none;
    stroke: transparent;
    stroke-width: 12;
    vector-effect: non-scaling-stroke;
    pointer-events: stroke;
  }
  .stroke.selected .visible {
    stroke: #2563eb;
    stroke-width: 2;
  }

  .pending.fixed {
    fill: none;
    stroke: #1a1a1a;
    stroke-width: 1.5;
    stroke-linejoin: round;
    stroke-linecap: round;
    vector-effect: non-scaling-stroke;
    pointer-events: none;
  }
  .pending.preview {
    stroke: rgba(0, 0, 0, 0.4);
    stroke-width: 1.5;
    stroke-dasharray: 4 4;
    stroke-linecap: round;
    vector-effect: non-scaling-stroke;
    pointer-events: none;
  }
  .pending-dot {
    fill: #1a1a1a;
    pointer-events: none;
  }

  rect.select-rect {
    fill: rgba(37, 99, 235, 0.08);
    stroke: #2563eb;
    stroke-width: 1;
    stroke-dasharray: 4 4;
    vector-effect: non-scaling-stroke;
    pointer-events: none;
  }

  .handles g { cursor: grab; }
  .handles .handle {
    fill: #fff;
    stroke: #2563eb;
    stroke-width: 1.5;
    vector-effect: non-scaling-stroke;
    pointer-events: none;
  }
  .handles .handle-hit {
    fill: transparent;
    pointer-events: all;
  }
  .handles g:hover .handle { fill: #2563eb; }

  .handles.delete-mode g { cursor: pointer; }
  .handles.delete-mode .handle { stroke: #dc2626; }
  .handles.delete-mode g:hover .handle { fill: #dc2626; }

  .bbox-rect {
    fill: none;
    stroke: #2563eb;
    stroke-width: 1;
    stroke-dasharray: 4 4;
    vector-effect: non-scaling-stroke;
    pointer-events: none;
  }
  .bbox .corner {
    fill: #fff;
    stroke: #2563eb;
    stroke-width: 1.5;
    vector-effect: non-scaling-stroke;
    pointer-events: none;
  }
  .bbox .corner-hit {
    fill: transparent;
    pointer-events: all;
  }
  .bbox g:hover .corner { fill: #2563eb; }

  .ghost { cursor: copy; }
  .ghost .ghost-dot {
    fill: rgba(37, 99, 235, 0.15);
    stroke: #2563eb;
    stroke-width: 1.25;
    stroke-dasharray: 2 2;
    vector-effect: non-scaling-stroke;
    pointer-events: none;
  }
  .ghost .ghost-hit {
    fill: transparent;
    pointer-events: all;
  }
  .ghost:hover .ghost-dot {
    fill: #2563eb;
    stroke-dasharray: none;
  }

  .right-overlay {
    position: absolute;
    top: 1rem;
    right: 1rem;
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
    z-index: 1;
  }
  .zoom-overlay {
    display: flex;
    flex-direction: column;
    gap: 0.125rem;
  }

  .edit-hint {
    position: absolute;
    bottom: 1rem;
    left: 50%;
    transform: translateX(-50%);
    padding: 0.3rem 0.65rem;
    background: rgba(255, 255, 255, 0.92);
    border: 1px solid #e5e5e5;
    font-size: 0.65rem;
    color: #666;
    font-family: ui-monospace, "SF Mono", Menlo, monospace;
    white-space: nowrap;
    pointer-events: none;
    z-index: 1;
  }
  .edit-hint.delete {
    color: #b91c1c;
    border-color: #fecaca;
    background: rgba(254, 242, 242, 0.95);
  }
  .zoom-overlay button {
    margin: 0;
    padding: 0.3rem;
    width: 1.85rem;
    background: #fff;
    color: #1a1a1a;
    border: 1px solid #d4d4d4;
    text-transform: none;
    letter-spacing: 0;
    line-height: 1;
    font-size: 0.85rem;
  }
  .zoom-overlay button:hover {
    background: #f5f5f5;
    border-color: #1a1a1a;
  }
</style>
