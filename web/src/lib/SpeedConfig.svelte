<script>
  import { onMount } from 'svelte';

  let maxSpeed = 1200;
  let acceleration = 600;
  let saving = false;
  let dirty = false;

  async function load() {
    try {
      const r = await fetch('/config');
      if (!r.ok) return;
      const j = await r.json();
      maxSpeed = Math.round(j.max_speed);
      acceleration = Math.round(j.acceleration);
      dirty = false;
    } catch (_e) {}
  }

  async function apply() {
    saving = true;
    try {
      const url = `/config?max_speed=${maxSpeed}&acceleration=${acceleration}`;
      const r = await fetch(url, { method: 'POST' });
      if (r.ok) {
        const j = await r.json();
        maxSpeed = Math.round(j.max_speed);
        acceleration = Math.round(j.acceleration);
        dirty = false;
      }
    } finally {
      saving = false;
    }
  }

  onMount(load);
</script>

<section>
  <h2>Speed</h2>

  <label>
    <span class="row">
      <span>Max speed</span>
      <span class="val">{maxSpeed} sps</span>
    </span>
    <input type="range" min="100" max="2400" step="50"
           bind:value={maxSpeed} on:input={() => dirty = true} />
  </label>

  <label>
    <span class="row">
      <span>Acceleration</span>
      <span class="val">{acceleration} sps²</span>
    </span>
    <input type="range" min="100" max="3000" step="50"
           bind:value={acceleration} on:input={() => dirty = true} />
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
  button[disabled] {
    opacity: 0.5;
    cursor: not-allowed;
  }
</style>
