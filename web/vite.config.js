import { defineConfig } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';
import { viteSingleFile } from 'vite-plugin-singlefile';

// Inlines all JS/CSS/assets into a single index.html so the ESP32 only
// needs to serve one file. During development, `npm run dev` proxies the
// API endpoints through to the real plotter on the LAN.

export default defineConfig({
  plugins: [svelte(), viteSingleFile()],
  build: {
    target: 'es2020',
    cssCodeSplit: false,
    assetsInlineLimit: 100_000_000,
    chunkSizeWarningLimit: 100_000_000,
    rollupOptions: { output: { inlineDynamicImports: true } },
  },
  server: {
    proxy: {
      '/status': 'http://drawbot.local',
      '/job':    'http://drawbot.local',
      '/stop':   'http://drawbot.local',
      '/home':   'http://drawbot.local',
    },
  },
});
