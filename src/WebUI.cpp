#include "WebUI.h"
#include "API.h"
#include <ESPAsyncWebServer.h>

static const char HTML_PAGE[] = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Phone Controller</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:#111827;color:#d1d5db;padding:16px;max-width:860px;margin:0 auto}
h1{font-size:1.3rem;color:#60a5fa;margin-bottom:14px}
h2{font-size:.95rem;color:#93c5fd;margin-bottom:10px;font-weight:600;text-transform:uppercase;letter-spacing:.05em}
.card{background:#1f2937;border-radius:8px;padding:16px;margin-bottom:12px;border:1px solid #374151}
.row{display:flex;flex-wrap:wrap;gap:8px;align-items:flex-end;margin-bottom:10px}
.field{display:flex;flex-direction:column;gap:4px}
label{font-size:.78rem;color:#9ca3af}
input,select{background:#111827;border:1px solid #374151;color:#d1d5db;border-radius:4px;padding:6px 9px;font-size:.83rem}
input[type=checkbox]{width:16px;height:16px;margin-top:8px;accent-color:#3b82f6}
input:focus,select:focus{outline:none;border-color:#3b82f6}
button{cursor:pointer;border:none;border-radius:4px;padding:7px 12px;font-size:.8rem;color:#fff;transition:filter .15s}
button:hover{filter:brightness(1.15)}
.bp{background:#1d4ed8}.bd{background:#991b1b}.bs{background:#166534}.bm{background:#374151}.bw{background:#92400e}
table{width:100%;border-collapse:collapse;font-size:.82rem;margin-top:8px}
th{text-align:left;color:#6b7280;padding:5px 8px;border-bottom:1px solid #374151;font-weight:normal}
td{padding:5px 8px;border-bottom:1px solid #273042;vertical-align:middle}
.badge{display:inline-block;padding:1px 8px;border-radius:10px;font-size:.74rem}
.bon{background:#14532d;color:#86efac}.boff{background:#450a0a;color:#fca5a5}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
.don{background:#22c55e}.doff{background:#ef4444}
.muted{color:#4b5563}
.hdr{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px}
.pgrid{display:flex;flex-wrap:wrap;gap:6px;margin-bottom:10px}
#edit-banner{display:none;font-size:.82rem;color:#fbbf24;margin-bottom:8px;padding:5px 8px;background:#451a03;border-radius:4px}
#ws-badge{font-size:.72rem;padding:2px 8px;border-radius:10px;background:#14532d;color:#86efac}
#ws-badge.off{background:#450a0a;color:#fca5a5}
</style>
</head>
<body>
<div class="hdr">
  <h1>Phone Controller</h1>
  <div style="display:flex;gap:8px;align-items:center">
    <span id="ws-badge">Live</span>
    <button class="bm" onclick="refreshAll()">Refresh</button>
  </div>
</div>

<!-- Ringer -->
<div class="card">
  <h2>Ringer</h2>
  <div id="ring-status" style="margin-bottom:10px"><span class="muted">Loading...</span></div>
  <div class="pgrid" id="pat-btns"></div>
  <button class="bd" onclick="stopRing()">Stop Ring</button>
</div>

<!-- Timers -->
<div class="card">
  <h2>Timers</h2>
  <div class="row">
    <div class="field"><label>Duration</label><input id="t-dur" placeholder="5m, 1h30m, 90s" style="width:130px"></div>
    <div class="field"><label>Pattern</label><select id="t-pat"></select></div>
    <button class="bs" onclick="addTimer()">Add Timer</button>
    <button class="bd" onclick="cancelAllTimers()">Cancel All</button>
  </div>
  <table>
    <thead><tr><th>ID</th><th>Remaining</th><th>Total</th><th>Pattern</th><th></th></tr></thead>
    <tbody id="t-body"><tr><td colspan="5" class="muted">Loading...</td></tr></tbody>
  </table>
</div>

<!-- Alarms -->
<div class="card">
  <h2>Alarms</h2>
  <div id="edit-banner">Editing alarm <span id="edit-id"></span> &mdash; fill in fields below and click Update</div>
  <div class="row">
    <div class="field"><label>Time</label><input type="time" id="a-time"></div>
    <div class="field"><label>Pattern</label><select id="a-pat"></select></div>
    <div class="field"><label>Rings (0=&infin;)</label><input type="number" id="a-rings" value="3" min="0" max="99" style="width:70px"></div>
    <div class="field"><label>Repeat daily</label><input type="checkbox" id="a-repeat" checked></div>
    <div class="field"><label>Skip weekends</label><input type="checkbox" id="a-skip"></div>
    <div class="field"><label>Enabled</label><input type="checkbox" id="a-enabled" checked></div>
  </div>
  <div class="row">
    <button class="bs" id="a-submit" onclick="submitAlarm()">Add Alarm</button>
    <button class="bm" id="a-cancel" style="display:none" onclick="cancelEdit()">Cancel Edit</button>
    <button class="bd" onclick="clearAlarms()">Delete All</button>
  </div>
  <table>
    <thead><tr><th>Time</th><th>Pattern</th><th>Rings</th><th>Repeat</th><th>Skip Wknd</th><th>Enabled</th><th></th></tr></thead>
    <tbody id="a-body"><tr><td colspan="7" class="muted">Loading...</td></tr></tbody>
  </table>
</div>

<!-- Clock -->
<div class="card">
  <h2>Hourly Chime</h2>
  <div id="clock-status" style="margin-bottom:10px"><span class="muted">Loading...</span></div>
  <div style="display:flex;gap:8px">
    <button class="bp" onclick="toggleClock()">Toggle On/Off</button>
    <button class="bm" onclick="toggleClockMode()">Toggle Mode</button>
  </div>
</div>

<script>
let editId = null;

async function req(method, path, body) {
  try {
    const o = {method};
    if (body) { o.body = JSON.stringify(body); o.headers = {'Content-Type':'application/json'}; }
    const r = await fetch(path, o);
    return r.ok ? r.json().catch(() => null) : null;
  } catch(e) { return null; }
}

async function loadPatterns() {
  const data = await req('GET', '/ring/patterns');
  if (!data) return;
  const btns = document.getElementById('pat-btns');
  btns.innerHTML = '';
  data.forEach(p => {
    const b = document.createElement('button');
    b.className = 'bp'; b.textContent = p;
    b.onclick = () => startRing(p);
    btns.appendChild(b);
  });
  ['t-pat','a-pat'].forEach(id => {
    const sel = document.getElementById(id);
    const prev = sel.value;
    sel.innerHTML = data.map(p => `<option>${p}</option>`).join('');
    if (prev && data.includes(prev)) sel.value = prev;
  });
  document.getElementById('t-pat').value = 'chirp';
  document.getElementById('a-pat').value = 'us';
}

async function loadRinger() {
  const data = await req('GET', '/ring/status');
  const el = document.getElementById('ring-status');
  if (!data) { el.innerHTML = '<span class="muted">Unknown</span>'; return; }
  const cls = data.ringing ? 'don' : 'doff';
  el.innerHTML = `<span class="dot ${cls}"></span>${data.ringing ? '<strong>Currently ringing</strong>' : 'Not ringing'}`;
}

async function startRing(p) {
  await req('POST', '/ring/' + p);
}

async function stopRing() {
  await req('POST', '/ring/stop');
}

async function loadTimers() {
  const data = await req('GET', '/timer/status');
  const tb = document.getElementById('t-body');
  if (!data || !data.length) {
    tb.innerHTML = '<tr><td colspan="5" class="muted">No active timers</td></tr>';
    return;
  }
  tb.innerHTML = data.map(t =>
    `<tr><td>${t.id}</td><td>${t.remaining}</td><td>${t.total}</td><td>${t.pattern}</td>` +
    `<td><button class="bd" onclick="cancelTimer(${t.id})">Cancel</button></td></tr>`
  ).join('');
}

async function addTimer() {
  const dur = document.getElementById('t-dur').value.trim();
  const pat = document.getElementById('t-pat').value;
  if (!dur) return;
  await req('POST', `/timer/${dur}/${pat}`);
  document.getElementById('t-dur').value = '';
}

async function cancelTimer(id) {
  await req('POST', '/timer/cancel/' + id);
}

async function cancelAllTimers() {
  await req('POST', '/timer/cancel');
}

async function loadAlarms() {
  const data = await req('GET', '/alarm');
  const tb = document.getElementById('a-body');
  if (!data || !data.length) {
    tb.innerHTML = '<tr><td colspan="7" class="muted">No alarms scheduled</td></tr>';
    return;
  }
  tb.innerHTML = data.map(a => {
    const j = JSON.stringify(a).replace(/'/g, "&#39;");
    return `<tr>
      <td><strong>${a.time}</strong></td>
      <td>${a.pattern}</td>
      <td>${a.rings === 0 ? '&infin;' : a.rings}</td>
      <td><span class="badge ${a.repeat ? 'bon':'boff'}">${a.repeat?'yes':'no'}</span></td>
      <td><span class="badge ${a.skipWeekends ? 'bon':'boff'}">${a.skipWeekends?'yes':'no'}</span></td>
      <td><span class="badge ${a.enabled ? 'bon':'boff'}">${a.enabled?'on':'off'}</span></td>
      <td style="white-space:nowrap">
        <button class="bm" onclick='editAlarm(${j})'>Edit</button>
        <button class="bd" onclick="deleteAlarm(${a.id})">Delete</button>
      </td>
    </tr>`;
  }).join('');
}

function editAlarm(a) {
  editId = a.id;
  document.getElementById('a-time').value = a.time;
  document.getElementById('a-pat').value = a.pattern;
  document.getElementById('a-rings').value = a.rings;
  document.getElementById('a-repeat').checked = a.repeat;
  document.getElementById('a-skip').checked = a.skipWeekends;
  document.getElementById('a-enabled').checked = a.enabled;
  document.getElementById('edit-banner').style.display = '';
  document.getElementById('edit-id').textContent = a.id;
  document.getElementById('a-submit').textContent = 'Update Alarm';
  document.getElementById('a-cancel').style.display = '';
  document.getElementById('a-time').focus();
}

function cancelEdit() {
  editId = null;
  document.getElementById('edit-banner').style.display = 'none';
  document.getElementById('a-submit').textContent = 'Add Alarm';
  document.getElementById('a-cancel').style.display = 'none';
  document.getElementById('a-time').value = '';
}

async function submitAlarm() {
  const time = document.getElementById('a-time').value;
  if (!time) return;
  const [hour, minute] = time.split(':').map(Number);
  const body = {
    hour, minute,
    pattern: document.getElementById('a-pat').value,
    rings: parseInt(document.getElementById('a-rings').value) || 3,
    repeat: document.getElementById('a-repeat').checked,
    skipWeekends: document.getElementById('a-skip').checked,
    enabled: document.getElementById('a-enabled').checked
  };
  if (editId !== null) {
    await req('PUT', '/alarm/' + editId, body);
    cancelEdit();
  } else {
    await req('POST', '/alarm', body);
  }
}

async function deleteAlarm(id) {
  await req('DELETE', '/alarm/' + id);
}

async function clearAlarms() {
  if (!confirm('Delete all alarms?')) return;
  await req('DELETE', '/alarm');
}

async function loadClock() {
  const data = await req('GET', '/clock');
  const el = document.getElementById('clock-status');
  if (!data) { el.innerHTML = '<span class="muted">Unknown</span>'; return; }
  const cls = data.enabled ? 'don' : 'doff';
  el.innerHTML = `<span class="dot ${cls}"></span>${data.enabled ? '<strong>Enabled</strong>' : 'Disabled'} &mdash; Mode: <strong>${data.mode}</strong>`;
}

async function toggleClock() {
  await req('POST', '/clock/toggle');
}

async function toggleClockMode() {
  await req('POST', '/clock/mode/toggle');
}

async function refreshAll() {
  await Promise.all([loadRinger(), loadTimers(), loadAlarms(), loadClock()]);
}

// WebSocket — drives all live updates
function connectWS() {
  const ws = new WebSocket(`ws://${location.host}/ws`);
  const badge = document.getElementById('ws-badge');

  ws.onopen = () => {
    badge.textContent = 'Live';
    badge.className = '';
    refreshAll();
  };

  ws.onclose = () => {
    badge.textContent = 'Offline';
    badge.className = 'off';
    setTimeout(connectWS, 3000);
  };

  ws.onmessage = ({data}) => {
    try {
      const {topic} = JSON.parse(data);
      if (topic.startsWith('ring/')) loadRinger();
      else if (topic.startsWith('timer/')) loadTimers();
      else if (topic.startsWith('alarm/')) loadAlarms();
      else if (topic.startsWith('clock/')) loadClock();
    } catch(e) {}
  };
}

loadPatterns().then(refreshAll);
connectWS();
</script>
</body>
</html>
)rawliteral";

void webUIBegin() {
    apiGetServer()->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", HTML_PAGE);
    });
}
