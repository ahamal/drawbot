<script>
  import { onMount } from 'svelte';

  let penUp = 90;
  let penDown = 70;
  let saving = false;
  let dirty = false;

  async function load() {
    try {
      const r = await fetch('/config');
      if (!r.ok) return;
      const j = await r.json();
      penUp = j.pen_up_deg;
      penDown = j.pen_down_deg;
      dirty = false;
    } catch (_e) {}
  }

  async function apply() {
    saving = true;
    try {
      const url = `/config?pen_up_deg=${penUp}&pen_down_deg=${penDown}`;
      const r = await fetch(url, { method: 'POST' });
      if (r.ok) {
        const j = await r.json();
        penUp = j.pen_up_deg;
        penDown = j.pen_down_deg;
        dirty = false;
      }
    } finally {
      saving = false;
    }
  }

  onMount(load);
</script>

<section>
  <h2>Pen calibration</h2>

  <label>
    <span class="row">
      <span>Pen up</span>
      <span class="val">{penUp}°</span>
    </span>
    <input type="range" min="0" max="180" step="1"
           bind:value={penUp} on:input={() => dirty = true} />
  </label>

  <label>
    <span class="row">
      <span>Pen down</span>
      <span class="val">{penDown}°</span>
    </span>
    <input type="range" min="0" max="180" step="1"
           bind:value={penDown} on:input={() => dirty = true} />
  </label>

  <button on:click={apply} disabled={saving || !dirty}>
    {saving ? 'Applying…' : dirty ? 'Apply' : 'Saved'}
  </button>
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
  input[type="range"] {
    width: 100%;
    accent-color: #1a1a1a;
  }
  button[disabled] { opacity: 0.5; cursor: not-allowed; }
</style>
