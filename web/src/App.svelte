<script>
  import { onMount, onDestroy } from 'svelte';
  import StatusPanel  from './lib/StatusPanel.svelte';
  import JogPad       from './lib/JogPad.svelte';
  import PenControls  from './lib/PenControls.svelte';
  import Actions      from './lib/Actions.svelte';
  import JobPanel     from './lib/JobPanel.svelte';
  import { getStatus } from './lib/api.js';

  let status = null;
  let online = false;
  let timer;

  async function poll() {
    try {
      const { body } = await getStatus();
      status = body;
      online = true;
    } catch (_e) {
      online = false;
    }
  }

  onMount(() => {
    poll();
    timer = setInterval(poll, 500);
  });
  onDestroy(() => clearInterval(timer));
</script>

<header>
  <h1>TinyRoboCo</h1>
  <div class="pill" class:ok={online} class:err={!online}>
    {online ? 'online' : 'offline'}
  </div>
</header>

<StatusPanel {status} />
<JogPad />
<PenControls />
<Actions />
<JobPanel />

<style>
  .pill {
    padding: 0.25rem 0.75rem;
    border-radius: 999px;
    font-size: 0.85rem;
    font-weight: 500;
    background: #ddd;
    color: #444;
  }
  .pill.ok  { background: #d4edda; color: #155724; }
  .pill.err { background: #f8d7da; color: #721c24; }
</style>
