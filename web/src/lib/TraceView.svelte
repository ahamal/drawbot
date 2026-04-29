<script>
  // Photo → polylines wizard. Three stages, navigated via buttons inside
  // the panel:
  //
  //   type       — pick Squiggle or Hatch (cards, no params)
  //   threshold  — pick threshold + levels (hatch only; squiggle skips
  //                this stage). Preview is posterized to N+1 gray bands
  //                so the user can see what `levels` will discretize
  //                their tones into.
  //   lines      — pick mode-specific line params. Preview shows the
  //                actual polylines (white background for hatch; faded
  //                photo + lines for squiggle).
  //
  // App.svelte's topbar shows only the Cancel button while we're in
  // `/trace`; everything else (Next, Back, Add to canvas) lives in the
  // panel below.

  import { onMount } from 'svelte';
  import {
    paper, traceImage, tracePolylines, traceStage,
  } from './draw/store.js';
  import { goto } from './draw/router.js';
  import { hatch }    from './draw/hatch.js';
  import { squiggle } from './draw/squiggle.js';
  import { outline }  from './draw/outline.js';

  let mode = 'hatch';

  // Hatch params
  let hatchThreshold = 128;
  let hatchLevels = 1;
  let hatchSpacing = 2;
  let hatchAngle = 0;
  let hatchCross = false;

  // Outline params
  let outlineThreshold = 128;
  let outlineDetail = 0.5;

  // Squiggle params
  let squiggleSpacing = 3;
  let squigglePeriod = 4;
  let squiggleAmp = 1;

  // Modes that need a threshold/zone stage before line params.
  function needsThreshold(m) { return m === 'hatch' || m === 'outline'; }

  let invert = false;
  let imageDataUrl = null;
  let thresholdPreviewUrl = null; // posterized for hatch, B/W for outline
  let imgW = 0, imgH = 0;
  let busy = false;

  $: fit = (imgW > 0 && imgH > 0)
    ? computeFit($paper.w, $paper.h, imgW, imgH)
    : null;

  function computeFit(paperW, paperH, w, h) {
    const scale = Math.min(paperW / w, paperH / h);
    const fw = w * scale;
    const fh = h * scale;
    return { scale, x: (paperW - fw) / 2, y: (paperH - fh) / 2, w: fw, h: fh };
  }

  function rotatePaper() {
    paper.update(p => ({ ...p, w: p.h, h: p.w }));
  }

  // Wizard navigation. For Squiggle, the threshold stage is skipped.
  function next() {
    if ($traceStage === 'type') {
      traceStage.set(needsThreshold(mode) ? 'threshold' : 'lines');
    } else if ($traceStage === 'threshold') {
      traceStage.set('lines');
    }
  }
  function back() {
    if ($traceStage === 'lines') {
      traceStage.set(needsThreshold(mode) ? 'threshold' : 'type');
    } else if ($traceStage === 'threshold') {
      traceStage.set('type');
    }
  }

  // The Add action lives in App.svelte too (commitTrace via topbar in the
  // old design). Now it's an in-panel button; we trigger the same logic
  // by re-using the store contracts: write polylines + signal commit.
  // The actual commit still happens via App.svelte's commitTrace, which
  // App listens for via the route. To keep architecture simple we just
  // route to a function App passes via context — but Svelte doesn't have
  // shared event buses, so simplest is: do the commit here too.
  import { strokes, selection } from './draw/store.js';
  function commit() {
    if ($tracePolylines.length === 0) return;
    const ids = strokes.addPolylines($tracePolylines);
    selection.set(new Set(ids));
    traceImage.set(null);
    tracePolylines.set([]);
    traceStage.set('type');
    goto('/draw');
  }

  function paramsFor(m) {
    const common = { invert, fit };
    switch (m) {
      case 'hatch':
        return { ...common, thresholdVal: hatchThreshold, spacingMm: hatchSpacing,
                 levels: hatchLevels, angle: hatchAngle, crosshatch: hatchCross };
      case 'outline':
        return { ...common, thresholdVal: outlineThreshold, detailMm: outlineDetail };
      case 'squiggle':
        return { ...common, rowSpacingMm: squiggleSpacing, periodMm: squigglePeriod,
                 ampMm: squiggleAmp };
    }
  }

  // Posterize the photo to N+1 gray bands so the user can see what the
  // threshold + levels combo will discretize their image into. Above the
  // threshold = pure white. Inside the zone = quantized to N+1 evenly
  // spaced grays from light to black.
  function buildPosterPreview(img, threshold, levels, inv) {
    const c = document.createElement('canvas');
    c.width = img.width;
    c.height = img.height;
    const ctx = c.getContext('2d');
    const out = ctx.createImageData(img.width, img.height);
    const N = Math.max(1, levels | 0);
    const zoneSize = Math.max(1, inv ? 255 - threshold : threshold);
    for (let i = 0; i < img.width * img.height; i++) {
      const r = img.data[i * 4];
      const g = img.data[i * 4 + 1];
      const b = img.data[i * 4 + 2];
      const lum = 0.299 * r + 0.587 * g + 0.114 * b;
      const inZone = inv ? lum >= threshold : lum <= threshold;
      let v;
      if (!inZone) {
        v = 255;
      } else {
        const intensity = inv
          ? (lum - threshold) / zoneSize
          : (threshold - lum) / zoneSize;
        // Mirror hatch.js exactly: a row at level k passes if intensity
        // > (k+1)/(N+1). count = how many of the N levels pass at this
        // pixel, in [0, N]. count=0 → no line (white); count=N → all
        // levels pass (max density = black).
        let count = Math.floor(intensity * (N + 1));
        if (count > N) count = N;
        v = 255 - Math.round((count / N) * 255);
      }
      out.data[i * 4]     = v;
      out.data[i * 4 + 1] = v;
      out.data[i * 4 + 2] = v;
      out.data[i * 4 + 3] = 255;
    }
    ctx.putImageData(out, 0, 0);
    return c.toDataURL('image/png');
  }

  // Pure binary B/W preview for outline mode — every pixel is either
  // pure white (out of zone) or pure black (in zone). The contour
  // tracer follows the boundary between the two.
  function buildBwPreview(img, threshold, inv) {
    const c = document.createElement('canvas');
    c.width = img.width;
    c.height = img.height;
    const ctx = c.getContext('2d');
    const out = ctx.createImageData(img.width, img.height);
    for (let i = 0; i < img.width * img.height; i++) {
      const r = img.data[i * 4];
      const g = img.data[i * 4 + 1];
      const b = img.data[i * 4 + 2];
      const lum = 0.299 * r + 0.587 * g + 0.114 * b;
      const inZone = inv ? lum > threshold : lum < threshold;
      const v = inZone ? 0 : 255;
      out.data[i * 4]     = v;
      out.data[i * 4 + 1] = v;
      out.data[i * 4 + 2] = v;
      out.data[i * 4 + 3] = 255;
    }
    ctx.putImageData(out, 0, 0);
    return c.toDataURL('image/png');
  }

  function recompute() {
    const img = $traceImage;
    if (!img || !fit) return;
    busy = true;

    if ($traceStage === 'type') {
      thresholdPreviewUrl = null;
      tracePolylines.set([]);
    } else if ($traceStage === 'threshold') {
      if (mode === 'hatch') {
        thresholdPreviewUrl = buildPosterPreview(img, hatchThreshold, hatchLevels, invert);
      } else if (mode === 'outline') {
        thresholdPreviewUrl = buildBwPreview(img, outlineThreshold, invert);
      }
      tracePolylines.set([]);
    } else if ($traceStage === 'lines') {
      thresholdPreviewUrl = null;
      let polys = [];
      if (mode === 'hatch')        polys = hatch(img, paramsFor('hatch'));
      else if (mode === 'outline') polys = outline(img, paramsFor('outline'));
      else if (mode === 'squiggle') polys = squiggle(img, paramsFor('squiggle'));
      // Drop polylines below MIN_LENGTH_MM — sub-millimeter strokes
      // are pure pen-up/down overhead on a slow plotter and hide
      // beneath the rig's mechanical error anyway.
      tracePolylines.set(
        polys.filter(p => p.length >= 2 && polylineLengthMm(p) >= MIN_LENGTH_MM)
      );
    }

    busy = false;
  }

  let timer = null;
  function scheduleRecompute() {
    if (timer) clearTimeout(timer);
    timer = setTimeout(recompute, 150);
  }
  $: {
    mode; invert; fit; $traceStage;
    hatchThreshold; hatchSpacing; hatchLevels; hatchAngle; hatchCross;
    outlineThreshold; outlineDetail;
    squiggleSpacing; squigglePeriod; squiggleAmp;
    scheduleRecompute();
  }

  function pointsAttr(pts) {
    return pts.map(p => `${p[0].toFixed(2)},${p[1].toFixed(2)}`).join(' ');
  }

  const MIN_LENGTH_MM = 1;
  function polylineLengthMm(pts) {
    let len = 0;
    for (let i = 1; i < pts.length; i++) {
      const dx = pts[i][0] - pts[i - 1][0];
      const dy = pts[i][1] - pts[i - 1][1];
      len += Math.hypot(dx, dy);
    }
    return len;
  }

  onMount(() => {
    const img = $traceImage;
    if (!img) {
      goto('/draw');
      return;
    }
    imgW = img.width;
    imgH = img.height;

    const photoLandscape = imgW > imgH;
    const paperLandscape = $paper.w > $paper.h;
    if (photoLandscape !== paperLandscape) rotatePaper();

    const c = document.createElement('canvas');
    c.width = imgW;
    c.height = imgH;
    c.getContext('2d').putImageData(img, 0, 0);
    imageDataUrl = c.toDataURL('image/png');

    // Always start the wizard from the type stage when the view mounts.
    traceStage.set('type');

    recompute();
  });
</script>

<div class="trace-view">
  <div class="canvas-area">
    <svg viewBox="0 0 {$paper.w} {$paper.h}"
         preserveAspectRatio="xMidYMid meet"
         class="preview">
      <rect class="paper-bg" x="0" y="0" width={$paper.w} height={$paper.h} />

      {#if $traceStage === 'threshold' && thresholdPreviewUrl && fit}
        <image href={thresholdPreviewUrl}
               x={fit.x} y={fit.y}
               width={fit.w} height={fit.h}
               preserveAspectRatio="none" />
      {:else if $traceStage === 'type' && imageDataUrl && fit}
        <image href={imageDataUrl}
               x={fit.x} y={fit.y}
               width={fit.w} height={fit.h}
               preserveAspectRatio="none"
               opacity="0.55" />
      {:else if $traceStage === 'lines' && (mode === 'squiggle' || mode === 'outline') && imageDataUrl && fit}
        <image href={imageDataUrl}
               x={fit.x} y={fit.y}
               width={fit.w} height={fit.h}
               preserveAspectRatio="none"
               opacity="0.45" />
      {/if}
      <!-- Hatch lines stage: white only, no image -->

      {#each $tracePolylines as poly}
        <polyline class="trace-line" points={pointsAttr(poly)} />
      {/each}
    </svg>
  </div>

  <div class="controls">
    {#if $traceStage === 'type'}
      <h3 class="stage-title">Style</h3>
      <div class="type-cards">
        <button class="type-card" class:active={mode === 'outline'}
                on:click={() => mode = 'outline'}>
          <strong>Outline</strong>
          <small>Trace boundaries between dark &amp; light regions.</small>
        </button>
        <button class="type-card" class:active={mode === 'hatch'}
                on:click={() => mode = 'hatch'}>
          <strong>Hatch</strong>
          <small>Parallel lines. Tone via density.</small>
        </button>
        <button class="type-card" class:active={mode === 'squiggle'}
                on:click={() => mode = 'squiggle'}>
          <strong>Squiggle</strong>
          <small>Single wavy line. Tone via amplitude. Plot-friendly.</small>
        </button>
      </div>
    {:else if $traceStage === 'threshold' && mode === 'hatch'}
      <h3 class="stage-title">Threshold &amp; levels</h3>
      <label>
        <span class="lbl">Threshold</span>
        <input type="range" min="0" max="255" bind:value={hatchThreshold} />
        <span class="val">{hatchThreshold}</span>
      </label>
      <label>
        <span class="lbl">Levels</span>
        <input type="range" min="1" max="6" step="1" bind:value={hatchLevels} />
        <span class="val">{hatchLevels}</span>
      </label>
      <label class="check">
        <input type="checkbox" bind:checked={invert} />
        <span>Invert (light → lines)</span>
      </label>
    {:else if $traceStage === 'threshold' && mode === 'outline'}
      <h3 class="stage-title">Threshold</h3>
      <label>
        <span class="lbl">Threshold</span>
        <input type="range" min="0" max="255" bind:value={outlineThreshold} />
        <span class="val">{outlineThreshold}</span>
      </label>
      <label class="check">
        <input type="checkbox" bind:checked={invert} />
        <span>Invert (light → lines)</span>
      </label>
    {:else if $traceStage === 'lines' && mode === 'hatch'}
      <h3 class="stage-title">Lines</h3>
      <label>
        <span class="lbl">Spacing</span>
        <input type="range" min="1" max="6" step="0.1" bind:value={hatchSpacing} />
        <span class="val">{hatchSpacing.toFixed(1)} mm</span>
      </label>
      <label>
        <span class="lbl">Angle</span>
        <input type="range" min="0" max="180" step="15" bind:value={hatchAngle} />
        <span class="val">{hatchAngle}°</span>
      </label>
      <label class="check">
        <input type="checkbox" bind:checked={hatchCross} />
        <span>Cross-hatch</span>
      </label>
    {:else if $traceStage === 'lines' && mode === 'outline'}
      <h3 class="stage-title">Lines</h3>
      <label>
        <span class="lbl">Detail</span>
        <input type="range" min="0" max="3" step="0.1" bind:value={outlineDetail} />
        <span class="val">{outlineDetail.toFixed(1)} mm</span>
      </label>
    {:else if $traceStage === 'lines' && mode === 'squiggle'}
      <h3 class="stage-title">Lines</h3>
      <label>
        <span class="lbl">Spacing</span>
        <input type="range" min="1" max="8" step="0.1" bind:value={squiggleSpacing} />
        <span class="val">{squiggleSpacing.toFixed(1)} mm</span>
      </label>
      <label>
        <span class="lbl">Period</span>
        <input type="range" min="1.5" max="12" step="0.1" bind:value={squigglePeriod} />
        <span class="val">{squigglePeriod.toFixed(1)} mm</span>
      </label>
      <label>
        <span class="lbl">Amplitude</span>
        <input type="range" min="0" max="5" step="0.1" bind:value={squiggleAmp} />
        <span class="val">{squiggleAmp.toFixed(1)} mm</span>
      </label>
      <label class="check">
        <input type="checkbox" bind:checked={invert} />
        <span>Invert (light → tall waves)</span>
      </label>
    {/if}

    <div class="nav-row">
      {#if $traceStage !== 'type'}
        <button class="nav-btn" on:click={back}>← Back</button>
      {:else}
        <button class="nav-btn" on:click={rotatePaper} title="Rotate paper">↻ Rotate</button>
      {/if}

      <span class="status">
        {#if busy}
          computing…
        {:else if $traceStage === 'lines'}
          {$tracePolylines.length} line{$tracePolylines.length === 1 ? '' : 's'}
        {/if}
      </span>

      {#if $traceStage === 'lines'}
        <button class="nav-btn primary" on:click={commit}
                disabled={$tracePolylines.length === 0}>
          Add {$tracePolylines.length} →
        </button>
      {:else}
        <button class="nav-btn primary" on:click={next}>Next →</button>
      {/if}
    </div>
  </div>
</div>

<style>
  .trace-view {
    flex: 1;
    min-height: 0;
    display: flex;
    flex-direction: column;
  }

  .canvas-area {
    flex: 1;
    min-height: 0;
    position: relative;
    width: 100%;
    background: #f5f5f5;
    overflow: hidden;
  }
  svg.preview {
    display: block;
    width: 100%;
    height: 100%;
  }
  .paper-bg { fill: #fff; }
  .trace-line {
    fill: none;
    stroke: #dc2626;
    stroke-width: 1.25;
    stroke-linejoin: round;
    stroke-linecap: round;
    vector-effect: non-scaling-stroke;
  }

  .controls {
    flex: none;
    padding: 0.6rem 0.85rem;
    background: #fff;
    border-top: 1px solid #e5e5e5;
    display: flex;
    flex-direction: column;
    gap: 0.4rem;
  }

  .stage-title {
    margin: 0;
    font-size: 0.65rem;
    text-transform: uppercase;
    letter-spacing: 0.15em;
    color: #888;
    font-weight: 500;
  }

  .type-cards {
    display: flex;
    gap: 0.4rem;
  }
  .type-card {
    flex: 1;
    margin: 0;
    padding: 0.6rem 0.7rem;
    background: #fff;
    color: #1a1a1a;
    border: 1px solid #d4d4d4;
    text-transform: none;
    letter-spacing: 0;
    font-weight: 400;
    text-align: left;
    cursor: pointer;
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    gap: 0.2rem;
    line-height: 1.3;
  }
  .type-card:hover { border-color: #1a1a1a; }
  .type-card strong {
    font-size: 0.8rem;
    font-weight: 600;
  }
  .type-card small {
    font-size: 0.65rem;
    color: #666;
  }
  .type-card.active {
    background: #1a1a1a;
    color: #fff;
    border-color: #1a1a1a;
  }
  .type-card.active small { color: #ddd; }

  .controls label {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    font-size: 0.7rem;
    color: #444;
  }
  .lbl { width: 4.5rem; flex: none; }
  .controls input[type="range"] {
    flex: 1;
    min-width: 0;
  }
  .val {
    font-family: ui-monospace, "SF Mono", Menlo, monospace;
    font-size: 0.65rem;
    color: #666;
    width: 3.5rem;
    text-align: right;
    flex: none;
  }
  .check { gap: 0.4rem; }
  .check span { width: auto; }

  .nav-row {
    display: flex;
    gap: 0.5rem;
    align-items: center;
    margin-top: 0.2rem;
  }
  .status {
    flex: 1;
    text-align: center;
    font-size: 0.65rem;
    color: #666;
    font-family: ui-monospace, "SF Mono", Menlo, monospace;
  }

  .nav-btn {
    margin: 0;
    padding: 0.4rem 0.75rem;
    font-size: 0.7rem;
    background: #fff;
    color: #1a1a1a;
    border: 1px solid #d4d4d4;
    text-transform: none;
    letter-spacing: 0;
    font-weight: 500;
  }
  .nav-btn:hover {
    background: #f5f5f5;
    border-color: #1a1a1a;
  }
  .nav-btn.primary {
    background: #1a1a1a;
    color: #fff;
    border-color: #1a1a1a;
  }
  .nav-btn.primary:hover {
    background: #000;
    border-color: #000;
  }
  .nav-btn[disabled],
  .nav-btn[disabled]:hover {
    opacity: 0.5;
    cursor: not-allowed;
    background: transparent;
    border-color: #d4d4d4;
  }
</style>
