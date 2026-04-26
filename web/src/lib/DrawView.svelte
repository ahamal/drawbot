<script>
  import { createEventDispatcher } from 'svelte';
  const dispatch = createEventDispatcher();
</script>

<section class="hd">
  <h2>Draw</h2>
  <button on:click={() => dispatch('back')}>← Back</button>
</section>

<section>
  <p class="todo">Drawing UI goes here. Camera capture, image processing, path tracing, preview, send-to-plotter.</p>
</section>

<style>
  .hd { display: flex; justify-content: space-between; align-items: center; }
  .hd h2 { margin: 0; }
  .todo { color: #888; font-style: italic; margin: 0; font-size: 0.8rem; }
</style>
