<script>
  import { onMount, onDestroy } from 'svelte';
  import StatusPanel  from './lib/StatusPanel.svelte';
  import JogPad       from './lib/JogPad.svelte';
  import PenControls  from './lib/PenControls.svelte';
  import Actions      from './lib/Actions.svelte';
  import SpeedConfig  from './lib/SpeedConfig.svelte';
  import JobPanel     from './lib/JobPanel.svelte';
  import DrawView     from './lib/DrawView.svelte';
  import { getStatus } from './lib/api.js';

  let view = 'home';     // 'home' | 'draw'
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
  <h1>drawbot</h1>
  <div class="indicator" class:ok={online} class:err={!online}>
    <span class="dot" />{online ? 'online' : 'offline'}
  </div>
</header>

{#if view === 'home'}
  <button class="primary" on:click={() => view = 'draw'}>Start Drawing</button>
  <StatusPanel {status} />
  <JogPad />
  <PenControls />
  <Actions />
  <SpeedConfig />
  <JobPanel />
{:else if view === 'draw'}
  <DrawView on:back={() => view = 'home'} />
{/if}

<style>
  .indicator {
    display: inline-flex;
    align-items: center;
    gap: 0.35rem;
    font-size: 0.65rem;
    text-transform: uppercase;
    letter-spacing: 0.1em;
    color: #888;
  }
  .indicator.ok  { color: #16a34a; }
  .indicator.err { color: #dc2626; }
  .dot {
    width: 6px; height: 6px;
    background: currentColor;
    display: inline-block;
  }
</style>
