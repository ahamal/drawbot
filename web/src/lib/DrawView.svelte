<script>
  import { createEventDispatcher } from 'svelte';
  import * as primitives from './draw/primitives.js';
  import { polylinesToGcode } from './draw/gcode.js';
  import { sendJob } from './api.js';

  const dispatch = createEventDispatcher();
  let response = '';

  async function send(polylines, label) {
    const gcode = polylinesToGcode(polylines);
    response = `sending ${label}…`;
    try {
      const { status, body } = await sendJob(gcode);
      response = `[${status}] ${typeof body === 'string' ? body : JSON.stringify(body)}`;
    } catch (e) {
      response = `[network] ${e.message}`;
    }
  }
</script>

<section class="hd">
  <h2>Draw</h2>
  <button on:click={() => dispatch('back')}>← Back</button>
</section>

<section>
  <h2>Primitives</h2>
  <p class="hint">
    Test shapes centered on the paper. Position the pen at the top-left
    corner first, then tap <strong>Set Origin</strong> on the home view so
    these draw within the page.
  </p>
  <div class="grid">
    <button on:click={() => send(primitives.line(),   'line')}>Line</button>
    <button on:click={() => send(primitives.square(), 'square')}>Square 50 mm</button>
    <button on:click={() => send(primitives.circle(), 'circle')}>Circle Ø 60 mm</button>
    <button on:click={() => send(primitives.spiral(), 'spiral')}>Spiral</button>
  </div>
  <pre>{response}</pre>
</section>

<style>
  .hd { display: flex; justify-content: space-between; align-items: center; }
  .hd h2 { margin: 0; }
  .hint { font-size: 0.7rem; color: #666; margin: 0 0 0.5rem; }
  .grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 0.25rem;
  }
  .grid button { width: 100%; margin: 0; padding: 0.5rem; }
  pre {
    background: #fafafa; color: #1a1a1a;
    padding: 0.4rem; border: 1px solid #e5e5e5;
    font-size: 0.7rem; overflow-x: auto;
    margin: 0.5rem 0 0; min-height: 1.25rem;
  }
</style>
