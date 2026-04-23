<script>
  import { sendJob } from './api.js';
</script>

<section>
  <h2>Pen</h2>
  <button on:click={() => sendJob('M3')}>Pen Down</button>
  <button on:click={() => sendJob('M5')}>Pen Up</button>
</section>
