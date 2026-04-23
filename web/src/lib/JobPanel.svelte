<script>
  import { sendJob } from './api.js';

  let gcode = '';
  let response = '';

  async function send() {
    if (!gcode.trim()) return;
    response = 'sending…';
    try {
      const { status, body } = await sendJob(gcode);
      response = `[${status}] ${typeof body === 'string' ? body : JSON.stringify(body)}`;
    } catch (e) {
      response = `[network] ${e.message}`;
    }
  }

  async function onFile(e) {
    const f = e.target.files[0];
    if (f) gcode = await f.text();
  }
</script>

<section>
  <h2>Job</h2>
  <textarea bind:value={gcode} rows="8" placeholder={"G90\nG1 X10 Y10\n..."} />
  <div class="row">
    <input type="file" accept=".gcode,.nc,.txt" on:change={onFile} />
    <button on:click={send}>Send</button>
  </div>
  <pre>{response}</pre>
</section>

<style>
  textarea {
    width: 100%;
    font-family: ui-monospace, monospace;
    font-size: 0.85rem;
    padding: 0.5rem;
    border: 1px solid #ccc;
    border-radius: 6px;
  }
  pre {
    background: #f1f3f5;
    padding: 0.5rem;
    border-radius: 6px;
    font-size: 0.8rem;
    overflow-x: auto;
    margin: 0.5rem 0 0;
    min-height: 1.5rem;
  }
  input[type="file"] { font-size: 0.85rem; }
</style>
