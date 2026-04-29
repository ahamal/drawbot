<script>
  import { onMount } from 'svelte';
  import { sendJob } from './api.js';

  // Default = theoretical (4096 / (π × 10mm)) ≈ 130.4. Real value is empirical
  // and depends on thread thickness, drum-mount accuracy, etc.
  let stepsPerMm = 130.4;
  let saving = false;
  let dirty = false;

  // Calibration wizard state.
  let calibStep = 'idle';      // 'idle' | 'measuring'
  let actualMm = '';

  async function load() {
    try {
      const r = await fetch('/config');
      if (!r.ok) return;
      const j = await r.json();
      stepsPerMm = Number(j.steps_per_mm);
      dirty = false;
    } catch (_e) {}
  }

  async function postValue(value) {
    saving = true;
    try {
      const url = `/config?steps_per_mm=${value}`;
      const r = await fetch(url, { method: 'POST' });
      if (r.ok) {
        const j = await r.json();
        stepsPerMm = Number(j.steps_per_mm);
        dirty = false;
      }
    } finally {
      saving = false;
    }
  }

  async function apply() {
    await postValue(Number(stepsPerMm));
  }

  async function runCalibrationMove() {
    // Pen up, then 100mm relative move in X. User has marked the start point.
    await sendJob('M5\nG91\nG1 X100\nG90');
    calibStep = 'measuring';
    actualMm = '';
  }

  async function applyCalibration() {
    const measured = Number(actualMm);
    if (!(measured > 0)) return;
    const corrected = stepsPerMm * (100 / measured);
    await postValue(corrected);
    calibStep = 'idle';
    actualMm = '';
  }

  function cancelCalibration() {
    calibStep = 'idle';
    actualMm = '';
  }

  onMount(load);
</script>

<section>
  <h2>Calibration</h2>

  <label>
    <span class="row">
      <span>Steps / mm</span>
      <span class="val">{Number(stepsPerMm).toFixed(4)}</span>
    </span>
    <input type="number" step="0.0001" min="0.1"
           bind:value={stepsPerMm} on:input={() => dirty = true} />
  </label>

  <button on:click={apply} disabled={saving || !dirty}>
    {saving ? 'Applying…' : dirty ? 'Apply' : 'Saved'}
  </button>

  <h3>Auto-calibrate X axis</h3>

  {#if calibStep === 'idle'}
    <p class="hint">
      Mark the pen position. Click Move 100mm. Then measure how far the
      carriage actually traveled and enter it.
    </p>
    <button on:click={runCalibrationMove}>Move 100mm in X</button>
  {:else if calibStep === 'measuring'}
    <p class="hint">Measure the actual distance with a ruler:</p>
    <label>
      <span>Actual distance (mm)</span>
      <input type="number" step="0.1" min="0.1" bind:value={actualMm} />
    </label>
    <div class="row-actions">
      <button on:click={applyCalibration} disabled={!(Number(actualMm) > 0)}>
        Apply correction
      </button>
      <button on:click={cancelCalibration}>Cancel</button>
    </div>
  {/if}
</section>

<style>
  label {
    display: block;
    margin-bottom: 0.5rem;
    color: #666;
    font-size: 0.7rem;
  }
  .row {
    display: flex;
    justify-content: space-between;
    margin-bottom: 0.15rem;
  }
  .val {
    font-family: ui-monospace, "SF Mono", Menlo, monospace;
    color: #1a1a1a;
  }
  input[type="number"] {
    width: 100%;
    background: #fafafa;
    color: #1a1a1a;
    border: 1px solid #d4d4d4;
    padding: 0.3rem 0.4rem;
    font-family: ui-monospace, "SF Mono", Menlo, monospace;
    font-size: 0.75rem;
  }
  h3 {
    font-size: 0.65rem;
    margin: 0.75rem 0 0.4rem;
    text-transform: uppercase;
    letter-spacing: 0.15em;
    color: #888;
    font-weight: 500;
  }
  .hint { font-size: 0.7rem; color: #666; margin: 0 0 0.4rem; }
  .row-actions { display: flex; gap: 0.25rem; flex-wrap: wrap; }
  button[disabled] { opacity: 0.5; cursor: not-allowed; }
</style>
