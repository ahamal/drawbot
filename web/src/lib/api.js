// Thin fetch wrapper around the plotter's REST API. All routes are
// same-origin from the deployed firmware; in `npm run dev` they are
// proxied to drawbot.local by vite.config.js.

export async function api(path, opts = {}) {
  const r = await fetch(path, opts);
  const ct = r.headers.get('content-type') || '';
  const body = ct.includes('application/json') ? await r.json() : await r.text();
  return { ok: r.ok, status: r.status, body };
}

export const sendJob = (gcode) =>
  api('/job', {
    method: 'POST',
    headers: { 'Content-Type': 'text/plain' },
    body: gcode,
  });

export const stop      = () => api('/stop',   { method: 'POST' });
export const home      = () => api('/home',   { method: 'POST' });
export const getStatus = () => api('/status');
