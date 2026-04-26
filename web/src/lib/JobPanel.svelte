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
    font-family: ui-monospace, "SF Mono", Menlo, monospace;
    font-size: 0.75rem;
    padding: 0.4rem;
    background: #fafafa;
    color: #1a1a1a;
    border: 1px solid #d4d4d4;
    resize: vertical;
  }
  textarea::placeholder { color: #aaa; }
  pre {
    background: #fafafa;
    color: #1a1a1a;
    padding: 0.4rem;
    border: 1px solid #e5e5e5;
    font-size: 0.7rem;
    overflow-x: auto;
    margin: 0.4rem 0 0;
    min-height: 1.25rem;
  }
  input[type="file"] {
    font-size: 0.7rem;
    color: #666;
    font-family: inherit;
  }
  input[type="file"]::file-selector-button {
    background: #1a1a1a;
    color: #fff;
    border: 1px solid #1a1a1a;
    padding: 0.25rem 0.5rem;
    font-family: inherit;
    font-size: 0.7rem;
    text-transform: uppercase;
    letter-spacing: 0.08em;
    cursor: pointer;
    margin-right: 0.4rem;
  }
  input[type="file"]::file-selector-button:hover {
    background: #000; border-color: #000;
  }
</style>
