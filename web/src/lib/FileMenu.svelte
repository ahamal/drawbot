<script>
  // Dropdown menu in the topbar with Save (download .svg), Open (upload
  // .svg), and Clear actions. Auto-save to localStorage happens
  // separately in the strokes store; this is for explicit, durable,
  // shareable saves.

  import { strokes, selection, paper } from './draw/store.js';
  import { strokesToSvg, svgToStrokes } from './draw/svg.js';
  import { PAPER_PRESETS } from './draw/primitives.js';

  let open = false;
  let menuEl;
  let fileInput;
  let lastError = '';

  function toggle() { open = !open; }
  function close()  { open = false; }

  function onWindowClick(e) {
    if (open && menuEl && !menuEl.contains(e.target)) close();
  }

  function timestamp() {
    const d = new Date();
    const pad = (n) => String(n).padStart(2, '0');
    return `${d.getFullYear()}${pad(d.getMonth() + 1)}${pad(d.getDate())}-${pad(d.getHours())}${pad(d.getMinutes())}`;
  }

  function downloadBlob(filename, content, type) {
    const blob = new Blob([content], { type });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    a.click();
    URL.revokeObjectURL(url);
  }

  function save() {
    if ($strokes.length === 0) return;
    const svg = strokesToSvg($strokes, $paper);
    downloadBlob(`drawbot-${timestamp()}.svg`, svg, 'image/svg+xml');
    close();
  }

  function pickFile() {
    fileInput.click();
  }

  function onFileChange(e) {
    const file = e.target.files[0];
    e.target.value = '';
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () => {
      try {
        const { polylines, paperName } = svgToStrokes(reader.result);
        if (polylines.length === 0) {
          lastError = 'No polylines found in that SVG';
          return;
        }
        strokes.clear();
        strokes.addPolylines(polylines);
        selection.set(new Set());
        if (paperName) {
          const preset = PAPER_PRESETS.find(p => p.name === paperName);
          if (preset) paper.set(preset);
        }
        lastError = '';
      } catch (err) {
        lastError = err.message || 'Could not load file';
      }
    };
    reader.onerror = () => { lastError = 'Could not read file'; };
    reader.readAsText(file);
    close();
  }

  function clearAll() {
    strokes.clear();
    selection.set(new Set());
    close();
  }
</script>

<svelte:window on:click={onWindowClick} />

<div class="file-menu" bind:this={menuEl}>
  <button class="nav" on:click|stopPropagation={toggle} class:open>File ▾</button>
  {#if open}
    <div class="menu"
         role="menu"
         tabindex="-1"
         on:click|stopPropagation
         on:keydown|stopPropagation>
      <button class="item" role="menuitem" on:click={save} disabled={$strokes.length === 0}>Save…</button>
      <button class="item" role="menuitem" on:click={pickFile}>Open…</button>
      <hr />
      <button class="item destructive" role="menuitem" on:click={clearAll} disabled={$strokes.length === 0}>Clear</button>
    </div>
  {/if}
  <input type="file" accept=".svg,image/svg+xml"
         bind:this={fileInput} on:change={onFileChange} hidden />
</div>

{#if lastError}
  <button class="error" type="button" on:click={() => lastError = ''}>{lastError}</button>
{/if}

<style>
  .file-menu {
    position: relative;
    display: inline-block;
  }
  .menu {
    position: absolute;
    top: calc(100% + 0.25rem);
    left: 0;
    min-width: 9rem;
    background: #fff;
    border: 1px solid #d4d4d4;
    box-shadow: 0 2px 6px rgba(0, 0, 0, 0.06);
    z-index: 10;
    padding: 0.2rem;
  }
  .menu .item {
    display: block;
    width: 100%;
    text-align: left;
    background: transparent;
    color: #1a1a1a;
    border: none;
    padding: 0.4rem 0.6rem;
    margin: 0;
    font-size: 0.7rem;
    text-transform: none;
    letter-spacing: 0;
    font-weight: 500;
    cursor: pointer;
  }
  .menu .item:hover { background: #f5f5f5; }
  .menu .item:disabled,
  .menu .item:disabled:hover {
    opacity: 0.5;
    cursor: not-allowed;
    background: transparent;
  }
  .menu .item.destructive { color: #b91c1c; }
  .menu .item.destructive:hover { background: #fef2f2; }
  .menu hr {
    border: none;
    border-top: 1px solid #f0f0f0;
    margin: 0.2rem 0;
  }

  .error {
    position: fixed;
    top: 4rem;
    left: 50%;
    transform: translateX(-50%);
    background: #fef2f2;
    color: #b91c1c;
    border: 1px solid #fecaca;
    padding: 0.4rem 0.75rem;
    font-size: 0.7rem;
    cursor: pointer;
    z-index: 100;
    text-transform: none;
    letter-spacing: 0;
    margin: 0;
    font-family: inherit;
  }
</style>
