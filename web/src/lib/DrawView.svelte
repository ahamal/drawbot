<script>
  import Canvas from './draw/Canvas.svelte';
  import { strokes, selection, pointDeleteMode } from './draw/store.js';

  let tool = 'select';

  $: singleSelectedStroke = ($selection.size === 1)
    ? $strokes.find(s => s.id === [...$selection][0]) ?? null
    : null;
  $: canDeletePoint = !!singleSelectedStroke && singleSelectedStroke.points.length >= 3;

  function deleteSelected() {
    strokes.remove([...$selection]);
    selection.set(new Set());
  }

  function onKeyDown(e) {
    const t = e.target;
    if (t && (t.tagName === 'INPUT' || t.tagName === 'TEXTAREA' || t.isContentEditable)) return;

    const cmdKey = e.metaKey || e.ctrlKey;

    if (cmdKey && (e.key === 'z' || e.key === 'Z')) {
      e.preventDefault();
      if (e.shiftKey) strokes.redo();
      else strokes.undo();
      return;
    }
    if (cmdKey && (e.key === 'y' || e.key === 'Y')) {
      e.preventDefault();
      strokes.redo();
      return;
    }

    // Select all. Switches to Select tool so the bounding box / handles
    // appear immediately and the user can act on the selection.
    if (cmdKey && (e.key === 'a' || e.key === 'A')) {
      e.preventDefault();
      if ($strokes.length === 0) return;
      selection.set(new Set($strokes.map(s => s.id)));
      tool = 'select';
      return;
    }

    if (!cmdKey && !e.altKey) {
      if (e.key === 'v' || e.key === 'V') { e.preventDefault(); tool = 'select'; return; }
      if (e.key === 'l' || e.key === 'L') { e.preventDefault(); tool = 'draw';   return; }
      if (e.key === 'h' || e.key === 'H') { e.preventDefault(); tool = 'drag';   return; }
    }

    if (tool !== 'select') return;
    if ((e.key === 'Delete' || e.key === 'Backspace') && $selection.size > 0) {
      e.preventDefault();
      deleteSelected();
    } else if (e.key === 'Escape') {
      // Esc cancels Delete-point mode first; a second Esc deselects.
      if ($pointDeleteMode) pointDeleteMode.set(false);
      else selection.set(new Set());
    }
  }
</script>

<svelte:window on:keydown={onKeyDown} />

<Canvas {tool}
  on:drawcommit={() => tool = 'select'}
  on:dragcommit={() => tool = 'select'}>
  <div class="tool-overlay">
    <button class:active={tool === 'select'} on:click={() => tool = 'select'} title="Select (V)">Select</button>
    <button class:active={tool === 'draw'}   on:click={() => tool = 'draw'}   title="Line (L)">Line</button>
    {#if tool === 'select' && $selection.size > 0}
      <button class="del" on:click={deleteSelected}>Delete ({$selection.size})</button>
    {/if}
    {#if tool === 'select' && canDeletePoint}
      <button class="del-point"
              class:active={$pointDeleteMode}
              on:click={() => pointDeleteMode.set(!$pointDeleteMode)}>
        {$pointDeleteMode ? 'Cancel delete' : 'Delete point'}
      </button>
    {/if}
  </div>
  <button slot="right"
          class="drag-tool"
          class:active={tool === 'drag'}
          on:click={() => tool = 'drag'}
          title="Drag (H) — pan once, then back to Select">
    <svg viewBox="0 0 24 24" width="14" height="14"
         fill="none" stroke="currentColor" stroke-width="2"
         stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
      <polyline points="5 9 2 12 5 15" />
      <polyline points="9 5 12 2 15 5" />
      <polyline points="15 19 12 22 9 19" />
      <polyline points="19 9 22 12 19 15" />
      <line x1="2" y1="12" x2="22" y2="12" />
      <line x1="12" y1="2" x2="12" y2="22" />
    </svg>
  </button>
</Canvas>

<style>
  .tool-overlay {
    position: absolute;
    top: 1rem;
    left: 1rem;
    display: flex;
    gap: 0.125rem;
    flex-wrap: wrap;
    max-width: calc(100% - 5.5rem);
    z-index: 1;
  }
  .tool-overlay button {
    margin: 0;
    padding: 0.3rem 0.55rem;
    font-size: 0.65rem;
    background: #fff;
    color: #1a1a1a;
    border: 1px solid #d4d4d4;
  }
  .tool-overlay button.active {
    background: #1a1a1a;
    color: #fff;
    border-color: #1a1a1a;
  }
  .tool-overlay button.del {
    background: #fff;
    color: #b91c1c;
    border-color: #d4d4d4;
    margin-left: 0.25rem;
  }
  .tool-overlay button.del:hover {
    background: #fef2f2;
    border-color: #b91c1c;
  }
  .tool-overlay button.del-point {
    background: #fff;
    color: #b91c1c;
    border-color: #d4d4d4;
    margin-left: 0.25rem;
  }
  .tool-overlay button.del-point:hover {
    background: #fef2f2;
    border-color: #b91c1c;
  }
  .tool-overlay button.del-point.active {
    background: #b91c1c;
    color: #fff;
    border-color: #b91c1c;
  }

  .drag-tool {
    margin: 0;
    padding: 0.3rem;
    width: 1.85rem;
    height: 1.85rem;
    background: #fff;
    color: #1a1a1a;
    border: 1px solid #d4d4d4;
    text-transform: none;
    letter-spacing: 0;
    line-height: 0;
    display: inline-flex;
    align-items: center;
    justify-content: center;
  }
  .drag-tool.active {
    background: #1a1a1a;
    color: #fff;
    border-color: #1a1a1a;
  }
  .drag-tool:hover {
    background: #f5f5f5;
    border-color: #1a1a1a;
  }
  .drag-tool.active:hover {
    background: #000;
    border-color: #000;
  }
</style>
