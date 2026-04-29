<script>
  // The print panel sits below the canvas. It does three things:
  //   1. Sends the current strokes as G-code to the plotter.
  //   2. Tracks the job that was sent (jobId stored in localStorage)
  //      so a tab refresh / new tab can resume showing progress.
  //   3. Renders a progress bar and live status whenever the firmware's
  //      reported jobId matches the tracked one.

  import { onMount } from 'svelte';
  import Canvas from './draw/Canvas.svelte';
  import { strokes, plotterStatus, plotterOnline } from './draw/store.js';
  import { polylinesToGcode } from './draw/gcode.js';
  import { sendJob, stop as stopJob } from './api.js';

  const JOB_KEY = 'drawbot.job';

  let response = '';
  let sending = false;
  // { jobId, totalCmds } — what we last sent. Survives reload.
  let trackedJob = null;

  onMount(() => {
    try {
      const raw = localStorage.getItem(JOB_KEY);
      if (raw) trackedJob = JSON.parse(raw);
    } catch (_e) {}
  });

  // Reactive: progress for the tracked job, but only when the firmware
  // confirms it's the same one. If jobId doesn't match (firmware
  // rebooted, or another client started a different job), we treat it
  // as no live tracking.
  $: progress = computeProgress(trackedJob, $plotterStatus);

  function computeProgress(tracked, status) {
    if (!tracked || !status) return null;
    if (status.job_id !== tracked.jobId) return null;
    const total = tracked.totalCmds || 0;
    const done = status.cmds_done || 0;
    const pct = total > 0 ? Math.min(100, (done / total) * 100) : 0;
    const isDone = (status.state === 'idle') && total > 0 && done >= total;
    const isAborted = (status.state === 'idle') && total > 0 && done < total;
    return { done, total, pct, isDone, isAborted };
  }

  async function send() {
    if ($strokes.length === 0) return;
    sending = true;
    response = `sending ${$strokes.length} line${$strokes.length === 1 ? '' : 's'}…`;
    const polylines = $strokes.map(s => s.points);
    const gcode = polylinesToGcode(polylines);
    try {
      const { ok, status, body } = await sendJob(gcode);
      if (ok && body && typeof body === 'object' && body.job_id) {
        trackedJob = { jobId: body.job_id, totalCmds: body.total_cmds || 0 };
        localStorage.setItem(JOB_KEY, JSON.stringify(trackedJob));
        response = `[${status}] queued ${trackedJob.totalCmds} cmds (job ${trackedJob.jobId})`;
      } else {
        response = `[${status}] ${typeof body === 'string' ? body : JSON.stringify(body)}`;
      }
    } catch (e) {
      response = `[network] ${e.message}`;
    } finally {
      sending = false;
    }
  }

  async function stop() {
    try {
      await stopJob();
      response = 'stop sent';
    } catch (e) {
      response = `[network] ${e.message}`;
    }
  }

  function summarize(s) {
    if (!s) return 'no status';
    const parts = [];
    if (s.state) parts.push(s.state);
    if (typeof s.x_mm === 'number' && typeof s.y_mm === 'number') {
      parts.push(`(${s.x_mm.toFixed(1)}, ${s.y_mm.toFixed(1)}) mm`);
    }
    if (s.last_error) parts.push(`err: ${s.last_error}`);
    return parts.length ? parts.join(' · ') : JSON.stringify(s);
  }
</script>

<div class="print-view">
  <Canvas tool="drag" />

  <div class="controls">
    <div class="status-row">
      <span class="dot" class:ok={$plotterOnline} class:err={!$plotterOnline} />
      <span class="status-text">
        {#if progress?.isDone}
          done · {progress.total} cmds
        {:else if progress?.isAborted}
          aborted at {progress.done}/{progress.total} ({progress.pct.toFixed(0)}%)
        {:else if progress}
          {$plotterStatus?.state ?? '…'} · {progress.done}/{progress.total} ({progress.pct.toFixed(0)}%)
        {:else}
          {summarize($plotterStatus)}
        {/if}
      </span>
      <span class="count">{$strokes.length} line{$strokes.length === 1 ? '' : 's'}</span>
    </div>

    {#if progress}
      <div class="progress" class:done={progress.isDone} class:aborted={progress.isAborted}>
        <div class="bar" style:width="{progress.pct}%" />
      </div>
    {/if}

    <div class="actions">
      <button class="stop-btn" on:click={stop} disabled={!$plotterOnline}>Stop</button>
      <button class="send-btn primary" on:click={send}
              disabled={$strokes.length === 0 || sending || !$plotterOnline}>
        {sending ? 'Sending…' : `Send to plotter →`}
      </button>
    </div>

    {#if response}
      <div class="response">{response}</div>
    {/if}
  </div>
</div>

<style>
  .print-view {
    flex: 1;
    min-height: 0;
    display: flex;
    flex-direction: column;
  }

  .controls {
    flex: none;
    padding: 0.6rem 0.85rem;
    background: #fff;
    border-top: 1px solid #e5e5e5;
    display: flex;
    flex-direction: column;
    gap: 0.4rem;
  }

  .status-row {
    display: flex;
    align-items: center;
    gap: 0.4rem;
    font-size: 0.7rem;
    color: #666;
    font-family: ui-monospace, "SF Mono", Menlo, monospace;
  }
  .dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: #888;
    flex: none;
  }
  .dot.ok  { background: #16a34a; }
  .dot.err { background: #dc2626; }
  .status-text {
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .count {
    flex: none;
    color: #888;
  }

  .progress {
    height: 4px;
    background: #f0f0f0;
    overflow: hidden;
  }
  .progress .bar {
    height: 100%;
    background: #2563eb;
    transition: width 0.3s ease-out;
  }
  .progress.done .bar    { background: #16a34a; }
  .progress.aborted .bar { background: #dc2626; }

  .actions {
    display: flex;
    gap: 0.4rem;
    align-items: center;
  }
  .send-btn {
    margin: 0 0 0 auto;
    padding: 0.45rem 0.85rem;
    font-size: 0.7rem;
    background: #1a1a1a;
    color: #fff;
    border: 1px solid #1a1a1a;
    text-transform: none;
    letter-spacing: 0;
    font-weight: 500;
  }
  .send-btn:hover { background: #000; }
  .send-btn[disabled],
  .send-btn[disabled]:hover {
    opacity: 0.5;
    cursor: not-allowed;
    background: #1a1a1a;
  }

  .stop-btn {
    margin: 0;
    padding: 0.45rem 0.85rem;
    font-size: 0.7rem;
    background: #fff;
    color: #b91c1c;
    border: 1px solid #d4d4d4;
    text-transform: none;
    letter-spacing: 0;
    font-weight: 500;
  }
  .stop-btn:hover {
    background: #fef2f2;
    border-color: #b91c1c;
  }
  .stop-btn[disabled],
  .stop-btn[disabled]:hover {
    opacity: 0.5;
    cursor: not-allowed;
    background: #fff;
    border-color: #d4d4d4;
  }

  .response {
    font-family: ui-monospace, "SF Mono", Menlo, monospace;
    font-size: 0.65rem;
    color: #444;
    border-top: 1px solid #f0f0f0;
    padding-top: 0.3rem;
    overflow-x: auto;
    white-space: nowrap;
  }
</style>
