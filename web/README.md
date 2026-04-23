# web/ — TinyRoboCo SPA

Vite + Svelte 4 single-page app. Built once, baked into the ESP32 firmware via `scripts/build_web.sh` and served from `/` on the device.

## Live dev (HMR + API proxy)

```bash
npm install     # first time only
npm run dev     # http://localhost:5173 — HMR; /status, /job, /stop, /home proxy to drawbot.local
```

Edit anything under `src/` and the browser reloads instantly. The dev server proxies API calls to the real plotter on the LAN, so jog/pen/home buttons drive the actual hardware.

## Production build

```bash
npm run build   # writes a single self-contained dist/index.html
```

Then from the repo root:

```bash
./scripts/build_web.sh   # bakes dist/index.html into firmware/plotter/web_assets.h
```

## Layout

```
src/
  main.js              → mounts App.svelte
  app.css              → global styles
  App.svelte           → root: header, polling loop, layout
  lib/
    api.js             → fetch wrappers for the firmware REST API
    StatusPanel.svelte
    JogPad.svelte
    PenControls.svelte
    Actions.svelte
    JobPanel.svelte
vite.config.js         → singleFile plugin + dev-server proxy
package.json
```

## Why single-file output

`vite-plugin-singlefile` inlines all JS, CSS, and assets into the produced `index.html`. The ESP32 firmware then only has to serve one PROGMEM string at `/` — no asset hash routing, no separate JS/CSS endpoints, no LittleFS. Tradeoff: the bundle has to stay small. Current size is ~14 KB.
