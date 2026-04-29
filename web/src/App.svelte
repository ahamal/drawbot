<script>
  import { onMount, onDestroy } from 'svelte';
  import StatusPanel  from './lib/StatusPanel.svelte';
  import JogPad       from './lib/JogPad.svelte';
  import PenControls  from './lib/PenControls.svelte';
  import PenConfig    from './lib/PenConfig.svelte';
  import Actions      from './lib/Actions.svelte';
  import SpeedConfig  from './lib/SpeedConfig.svelte';
  import CalibConfig  from './lib/CalibConfig.svelte';
  import JobPanel     from './lib/JobPanel.svelte';
  import DrawView         from './lib/DrawView.svelte';
  import PrintView        from './lib/PrintView.svelte';
  import TraceView        from './lib/TraceView.svelte';
  import FileMenu         from './lib/FileMenu.svelte';
  import TracePhotoButton from './lib/TracePhotoButton.svelte';
  import { getStatus } from './lib/api.js';
  import { route, goto } from './lib/draw/router.js';
  import {
    strokes, plotterStatus, plotterOnline,
    traceImage, tracePolylines, traceStage,
  } from './lib/draw/store.js';

  function cancelTrace() {
    traceImage.set(null);
    tracePolylines.set([]);
    traceStage.set('type');
    goto('/draw');
  }

  let timer;
  // Last successful poll timestamp. We only mark the plotter offline after
  // OFFLINE_AFTER_MS without a successful poll — single transient failures
  // (Wi-Fi blip, temporary 500) shouldn't flicker the online indicator.
  let lastOk = 0;
  const OFFLINE_AFTER_MS = 2000;

  async function poll() {
    try {
      const { body } = await getStatus();
      plotterStatus.set(body);
      plotterOnline.set(true);
      lastOk = Date.now();
    } catch (_e) {
      if (Date.now() - lastOk > OFFLINE_AFTER_MS) {
        plotterOnline.set(false);
      }
    }
  }

  onMount(() => {
    poll();
    timer = setInterval(poll, 500);
  });
  onDestroy(() => clearInterval(timer));
</script>

<header class="topbar">
  <a href="#/" class="brand">
    <h1>drawbot</h1>
    <div class="indicator" class:ok={$plotterOnline} class:err={!$plotterOnline}>
      <span class="dot" />{$plotterOnline ? 'online' : 'offline'}
    </div>
  </a>
  <nav class="actions">
    {#if $route === '/draw'}
      <button class="nav" on:click={() => goto('/')}>← Home</button>
      <FileMenu />
      <TracePhotoButton />
      <button class="nav" on:click={() => goto('/print')} disabled={$strokes.length === 0}>
        Next: Print →
      </button>
    {:else if $route === '/trace'}
      <button class="nav" on:click={cancelTrace}>← Cancel</button>
    {:else if $route === '/print'}
      <button class="nav" on:click={() => goto('/draw')}>← Back to Draw</button>
    {:else}
      <button class="nav" on:click={() => goto('/draw')}>Start Drawing →</button>
    {/if}
  </nav>
</header>

<main>
  {#if $route === '/draw'}
    <DrawView />
  {:else if $route === '/trace'}
    <TraceView />
  {:else if $route === '/print'}
    <PrintView />
  {:else}
    <div class="home">
      <div class="home-inner">
        <div class="home-grid">
          <StatusPanel status={$plotterStatus} />
          <JogPad />
          <PenControls />
          <PenConfig />
          <Actions />
          <SpeedConfig />
          <CalibConfig />
          <JobPanel />
        </div>
      </div>
    </div>
  {/if}
</main>
