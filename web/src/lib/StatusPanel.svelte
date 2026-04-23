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
  dl { display: grid; grid-template-columns: 6rem 1fr; gap: 0.25rem 1rem; margin: 0; }
  dt { color: #666; font-size: 0.85rem; }
  dd { margin: 0; font-family: ui-monospace, "SF Mono", Menlo, monospace; }
</style>
