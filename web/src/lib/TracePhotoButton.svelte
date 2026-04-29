<script>
  // Topbar button that triggers the OS file picker, downsamples the
  // chosen image to a processing-friendly size, stashes it in the
  // traceImage store, and navigates to /trace. `capture="environment"`
  // hints to mobile browsers that the rear camera is a valid source.

  import { traceImage } from './draw/store.js';
  import { goto } from './draw/router.js';

  // Cap the long edge during processing so trace + threshold stay
  // responsive even on 12MP phone photos. 700px gives ~0.2mm/pixel on
  // an A5 sheet — well below pen-tip precision.
  const MAX_DIM = 700;

  let fileInput;
  let lastError = '';

  async function loadImage(file) {
    const url = URL.createObjectURL(file);
    try {
      const img = new Image();
      img.src = url;
      await new Promise((resolve, reject) => {
        img.onload = resolve;
        img.onerror = () => reject(new Error('Image failed to load'));
      });
      const scale = Math.min(1, MAX_DIM / Math.max(img.naturalWidth, img.naturalHeight));
      const w = Math.max(1, Math.round(img.naturalWidth  * scale));
      const h = Math.max(1, Math.round(img.naturalHeight * scale));
      const canvas = document.createElement('canvas');
      canvas.width = w;
      canvas.height = h;
      const ctx = canvas.getContext('2d');
      ctx.drawImage(img, 0, 0, w, h);
      return ctx.getImageData(0, 0, w, h);
    } finally {
      URL.revokeObjectURL(url);
    }
  }

  async function onChange(e) {
    const file = e.target.files[0];
    e.target.value = '';
    if (!file) return;
    lastError = '';
    try {
      const imageData = await loadImage(file);
      traceImage.set(imageData);
      goto('/trace');
    } catch (err) {
      lastError = err.message || 'Could not load image';
    }
  }
</script>

<button class="nav" on:click={() => fileInput.click()}>Trace Photo</button>
<input type="file" accept="image/*" capture="environment"
       bind:this={fileInput} on:change={onChange} hidden />

{#if lastError}
  <button class="error" type="button" on:click={() => lastError = ''}>{lastError}</button>
{/if}

<style>
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
