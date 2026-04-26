<script>
  export let status;
</script>

<section>
  <h2>Status</h2>
  {#if status}
    <dl>
      <dt>State</dt>    <dd>{status.state}</dd>
      <dt>Position</dt> <dd>{status.x_mm.toFixed(2)}, {status.y_mm.toFixed(2)} mm</dd>
      <dt>Pen</dt>      <dd>{status.pen}</dd>
      <dt>Queue</dt>    <dd>{status.queue_remaining}</dd>
      <dt>Error</dt>    <dd>{status.last_error ?? '—'}</dd>
    </dl>
  {:else}
    <p>—</p>
  {/if}
</section>

<style>
  dl { display: grid; grid-template-columns: 5rem 1fr; gap: 0.15rem 0.75rem; margin: 0; }
  dt { color: #666; font-size: 0.65rem; text-transform: uppercase; letter-spacing: 0.08em; align-self: center; }
  dd { margin: 0; color: #1a1a1a; font-family: ui-monospace, "SF Mono", Menlo, monospace; }
  p  { margin: 0; color: #888; }
</style>
