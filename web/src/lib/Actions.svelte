<script>
  import { sendJob, stop } from './api.js';
</script>

<section>
  <h2>Actions</h2>
  <button on:click={() => sendJob('G92 X0 Y0')}>Set Origin</button>
  <button on:click={() => sendJob('G28')}>Home</button>
  <button class="danger" on:click={stop}>STOP</button>
</section>
