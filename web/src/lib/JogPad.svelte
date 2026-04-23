<script>
  import { sendJob } from './api.js';
  let step = 10;
  function jog(dx, dy) {
    sendJob(`G91\nG1 X${dx * step} Y${dy * step}\nG90`);
  }
</script>

<section>
  <h2>Jog</h2>
  <div class="step">
    <label>
      Step
      <select bind:value={step}>
        <option value={1}>1 mm</option>
        <option value={5}>5 mm</option>
        <option value={10}>10 mm</option>
        <option value={25}>25 mm</option>
      </select>
    </label>
  </div>
  <div class="pad">
    <button on:click={() => jog(0, 1)}>Y+</button>
    <button on:click={() => jog(-1, 0)}>X−</button>
    <button on:click={() => jog(1, 0)}>X+</button>
    <button on:click={() => jog(0, -1)}>Y−</button>
  </div>
</section>

<style>
  .step { margin-bottom: 0.5rem; }
  .pad {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 4rem));
    grid-template-rows: repeat(3, 2.5rem);
    gap: 0.25rem;
    justify-content: center;
  }
  .pad button { margin: 0; width: 100%; padding: 0; }
  .pad button:nth-child(1) { grid-column: 2; grid-row: 1; }
  .pad button:nth-child(2) { grid-column: 1; grid-row: 2; }
  .pad button:nth-child(3) { grid-column: 3; grid-row: 2; }
  .pad button:nth-child(4) { grid-column: 2; grid-row: 3; }
</style>
