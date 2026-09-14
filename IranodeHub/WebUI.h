#ifndef WEB_UI_H
#define WEB_UI_H

// Declared static: this header may end up included by more than one .cpp
// in the future, and `static` gives each translation unit its own private
// copy instead of colliding at link time - the standard way to keep a
// PROGMEM blob safely header-only in Arduino.
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Iranode Hub</title>
<style>
:root{
  color-scheme:dark;
  --bg:#0d1117; --surface:#161b22; --surface-hover:#1c2229; --border:#262c35;
  --text:#e6edf3; --text-muted:#8b949e; --text-faint:#5b6472;
  --accent:#3b82f6;
  --online:#3fb950; --online-soft:rgba(63,185,80,.15);
  --offline:#8b949e; --offline-soft:rgba(139,148,158,.12);
  --radius:14px; --radius-sm:8px;
}
*{box-sizing:border-box;}
body{
  margin:0; background:var(--bg); color:var(--text);
  font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
  -webkit-font-smoothing:antialiased;
}
.page{max-width:1000px; margin:0 auto; padding:28px 20px 60px;}
header.top{display:flex; justify-content:space-between; align-items:baseline; margin-bottom:18px; flex-wrap:wrap; gap:8px;}
header.top h1{font-size:1.4rem; font-weight:600; margin:0; letter-spacing:-0.01em;}
header.top .summary{color:var(--text-muted); font-size:0.9rem;}

#connIssue{display:none; background:#3d1f1f; color:#f2a4a4; border-radius:var(--radius-sm); padding:9px 14px; font-size:0.85rem; margin-bottom:16px;}

.grid{display:grid; grid-template-columns:repeat(auto-fill, minmax(300px, 1fr)); gap:14px;}
p.empty{grid-column:1/-1; color:var(--text-muted);}

.card{background:var(--surface); border:1px solid var(--border); border-radius:var(--radius); padding:16px 18px;}
.card.collapsed{cursor:pointer; transition:background .15s;}
.card.collapsed:hover{background:var(--surface-hover);}

.card-head{display:flex; justify-content:space-between; align-items:flex-start; gap:10px;}
.card-title{display:flex; flex-direction:column; gap:2px; min-width:0; flex:1;}
.card-name{font-weight:600; font-size:1.02rem; white-space:nowrap; overflow:hidden; text-overflow:ellipsis;}
.card-id{font-family:ui-monospace,SFMono-Regular,Menlo,monospace; font-size:0.72rem; color:var(--text-faint);}

.card-actions{display:flex; align-items:center; gap:8px; flex-shrink:0;}
.status-pill{display:flex; align-items:center; gap:5px; font-size:0.75rem; padding:3px 9px 3px 7px; border-radius:20px; white-space:nowrap;}
.status-pill.online{background:var(--online-soft); color:var(--online);}
.status-pill.offline{background:var(--offline-soft); color:var(--offline);}
.status-dot{width:6px; height:6px; border-radius:50%; background:currentColor;}

.icon-btn{background:none; border:none; color:var(--text-muted); cursor:pointer; padding:5px; border-radius:7px; display:flex;}
.icon-btn:hover{background:var(--border); color:var(--text);}
.icon-btn svg{width:16px; height:16px;}

.name-input{
  background:var(--bg); border:1px solid var(--border); color:var(--text);
  border-radius:var(--radius-sm); padding:6px 9px; font-size:0.9rem; width:100%; font-family:inherit;
}
.name-input:focus{outline:none; border-color:var(--accent);}
.edit-row{margin:8px 0 4px;}
.edit-label{font-size:0.7rem; color:var(--text-faint); margin-bottom:4px; text-transform:uppercase; letter-spacing:.04em;}

.channels{margin-top:6px;}
.channel-block{border-top:1px solid var(--border); padding:10px 0;}
.channel-block:first-child{border-top:none; padding-top:12px;}
.channel-row{display:flex; align-items:center; justify-content:space-between; gap:10px;}
.channel-left{display:flex; align-items:center; gap:9px; min-width:0;}
.color-dot{width:11px; height:11px; border-radius:50%; flex-shrink:0; box-shadow:0 0 0 2px rgba(255,255,255,.06);}
.channel-label{font-size:0.9rem; white-space:nowrap; overflow:hidden; text-overflow:ellipsis;}

.switch{position:relative; display:inline-block; width:40px; height:24px; flex-shrink:0;}
.switch input{opacity:0; width:0; height:0;}
.slider{position:absolute; inset:0; background:#30363d; border-radius:24px; cursor:pointer; transition:.15s;}
.slider::before{content:""; position:absolute; width:18px; height:18px; left:3px; top:3px; background:#fff; border-radius:50%; transition:.15s;}
.switch input:checked + .slider{background:var(--online);}
.switch input:checked + .slider::before{transform:translateX(16px);}
.switch input:disabled + .slider{opacity:.35; cursor:default;}

.color-block{margin-top:8px;}
.color-block.locked{opacity:.4; pointer-events:none;}
.swatch-row{display:flex; gap:8px; flex-wrap:wrap;}
.swatch-btn{width:26px; height:26px; border-radius:50%; border:2px solid transparent; cursor:pointer; padding:0; box-shadow:inset 0 0 0 1px rgba(255,255,255,.08);}
.swatch-btn.selected{border-color:var(--text); transform:scale(1.12);}
</style>
</head>
<body>
<div class="page">
  <header class="top">
    <h1>Iranode Hub</h1>
    <span class="summary" id="summary"></span>
  </header>
  <div id="connIssue">Can't reach the hub right now - retrying…</div>
  <div id="devices" class="grid">Loading…</div>
</div>
<script>
const COLOR_NAMES = ['Off','Red','Green','Blue','Yellow','Cyan','Magenta','White'];
const COLOR_HEX   = ['#30363d','#f85149','#3fb950','#58a6ff','#e3b341','#39c5cf','#db61a2','#f0f6fc'];

const expandedOffline = new Map(); // deviceId -> cached detail, only while an offline card is expanded
const editingIds = new Set();      // deviceId currently in edit mode

const ICON_EDIT = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 20h9"/><path d="M16.5 3.5a2.12 2.12 0 0 1 3 3L7 19l-4 1 1-4Z"/></svg>';
const ICON_DONE = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20 6 9 17l-5-5"/></svg>';
const ICON_CHEVRON = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="m9 18 6-6-6-6"/></svg>';

function el(html) {
  const t = document.createElement('template');
  t.innerHTML = html.trim();
  return t.content.firstElementChild;
}
function escapeHtml(s) {
  return (s || '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}
function postForm(url, params) {
  return fetch(url, { method: 'POST', headers: {'Content-Type':'application/x-www-form-urlencoded'}, body: new URLSearchParams(params) });
}
function submitOnEnter(input) {
  input.addEventListener('keydown', e => { if (e.key === 'Enter') input.blur(); });
}
function formatAge(sec) {
  if (sec < 60) return sec + 's ago';
  if (sec < 3600) return Math.floor(sec / 60) + 'm ago';
  if (sec < 86400) return Math.floor(sec / 3600) + 'h ago';
  return Math.floor(sec / 86400) + 'd ago';
}
function displayName(device) {
  return device.name && device.name.length ? escapeHtml(device.name) : device.id;
}
function statusPill(device) {
  return device.online
    ? '<span class="status-pill online"><span class="status-dot"></span>Online</span>'
    : '<span class="status-pill offline"><span class="status-dot"></span>Offline</span>';
}

function renderDeviceHead(device, editing) {
  const title = editing
    ? `<input class="name-input" data-role="device-name" value="${escapeHtml(device.name || '')}" placeholder="${device.id}">`
    : `<span class="card-name">${displayName(device)}</span><span class="card-id">${device.id}</span>`;
  return `<div class="card-head">
    <div class="card-title">${title}</div>
    <div class="card-actions">
      ${statusPill(device)}
      <button class="icon-btn" data-role="edit-toggle">${editing ? ICON_DONE : ICON_EDIT}</button>
    </div>
  </div>`;
}

function colorSwatches(selected) {
  return COLOR_NAMES.map((name, c) =>
    `<button class="swatch-btn ${c === selected ? 'selected' : ''}" title="${name}" style="background:${COLOR_HEX[c]}" data-color="${c}"></button>`
  ).join('');
}

function renderSwitchBody(device, editing) {
  const canControl = device.online;
  let html = '<div class="channels">';

  device.switch.channels.forEach((ch, i) => {
    const swatchColor = COLOR_HEX[ch.relay ? ch.colorOn : ch.colorOff];
    const label = ch.name && ch.name.length ? escapeHtml(ch.name) : ('Channel ' + (i + 1));

    html += '<div class="channel-block">';
    if (editing) {
      html += `<div class="edit-row">
        <div class="edit-label">Channel ${i + 1} name</div>
        <input class="name-input" data-role="channel-name" data-ch="${i + 1}" value="${escapeHtml(ch.name || '')}" placeholder="Channel ${i + 1}">
      </div>`;
    }
    html += `<div class="channel-row">
      <div class="channel-left"><span class="color-dot" style="background:${swatchColor}"></span><span class="channel-label">${label}</span></div>
      <label class="switch"><input type="checkbox" data-action="relay" data-ch="${i + 1}" ${ch.relay ? 'checked' : ''} ${canControl ? '' : 'disabled'}><span class="slider"></span></label>
    </div>`;
    if (editing) {
      html += `<div class="color-block ${canControl ? '' : 'locked'}">
        <div class="edit-label">On-color</div>
        <div class="swatch-row" data-ch="${i + 1}" data-slot="1">${colorSwatches(ch.colorOn)}</div>
      </div>
      <div class="color-block ${canControl ? '' : 'locked'}">
        <div class="edit-label">Off-color</div>
        <div class="swatch-row" data-ch="${i + 1}" data-slot="0">${colorSwatches(ch.colorOff)}</div>
      </div>`;
    }
    html += '</div>';
  });

  html += '</div>';
  return html;
}

function bindCardEvents(card, device) {
  card.querySelector('[data-role="edit-toggle"]').addEventListener('click', () => {
    if (editingIds.has(device.id)) editingIds.delete(device.id);
    else editingIds.add(device.id);
    const fresh = renderCard(device);
    fresh.dataset.id = device.id;
    card.replaceWith(fresh);
  });

  card.querySelectorAll('input[data-action="relay"]').forEach(input => {
    input.addEventListener('change', () => {
      postForm('/api/device/relay', { id: device.id, channel: input.dataset.ch, value: input.checked ? 1 : 0 });
    });
  });

  const nameInput = card.querySelector('[data-role="device-name"]');
  if (nameInput) {
    submitOnEnter(nameInput);
    nameInput.addEventListener('blur', () => {
      postForm('/api/device/name', { id: device.id, name: nameInput.value });
      device.name = nameInput.value;
    });
  }

  card.querySelectorAll('[data-role="channel-name"]').forEach(input => {
    submitOnEnter(input);
    input.addEventListener('blur', () => {
      const idx = input.dataset.ch - 1;
      postForm('/api/device/channel-name', { id: device.id, channel: input.dataset.ch, name: input.value });
      device.switch.channels[idx].name = input.value;
    });
  });

  card.querySelectorAll('.swatch-row').forEach(row => {
    row.querySelectorAll('.swatch-btn').forEach(btn => {
      btn.addEventListener('click', () => {
        postForm('/api/device/color', { id: device.id, channel: row.dataset.ch, slot: row.dataset.slot, color: btn.dataset.color });
        row.querySelectorAll('.swatch-btn').forEach(b => b.classList.remove('selected'));
        btn.classList.add('selected');
      });
    });
  });
}

function renderCollapsed(device) {
  const age = device.lastSeenSec != null ? `Last seen ${formatAge(device.lastSeenSec)}` : '';
  const card = el(`<div class="card collapsed">
    <div class="card-head">
      <div class="card-title"><span class="card-name">${device.id}</span><span class="card-id">${age}</span></div>
      <div class="card-actions">
        <span class="status-pill offline"><span class="status-dot"></span>Offline</span>
        <span class="icon-btn">${ICON_CHEVRON}</span>
      </div>
    </div>
  </div>`);
  card.addEventListener('click', async () => {
    try {
      const res = await fetch('/api/device/detail?id=' + device.id);
      const detail = await res.json();
      if (!detail.found) return;
      detail.online = false;
      expandedOffline.set(device.id, detail);
      const fresh = renderCard(detail);
      fresh.dataset.id = device.id;
      card.replaceWith(fresh);
    } catch (e) { console.log(e); }
  });
  return card;
}

// Single dispatch point: builds a header always, and a type-specific body
// only when we actually know the type (a device that's only ever sent a
// bare heartbeat so far - a rare boot-order edge case - just shows a
// header with no channels rather than guessing).
function renderCard(device) {
  if (!device.online && !expandedOffline.has(device.id)) return renderCollapsed(device);

  const source = device.online ? device : expandedOffline.get(device.id);
  const editing = editingIds.has(device.id);
  const head = renderDeviceHead(source, editing);
  const body = source.switch ? renderSwitchBody(source, editing) : '';
  // Future device types add their own "else if (source.someType) body = renderSomeTypeBody(...)" here.

  const card = el(`<div class="card">${head}${body}</div>`);
  bindCardEvents(card, source);
  return card;
}

function updateSummary(devices) {
  const summary = document.getElementById('summary');
  if (!devices.length) { summary.textContent = ''; return; }
  summary.textContent = devices.filter(d => d.online).length + ' of ' + devices.length + ' online';
}

function renderList(devices) {
  const container = document.getElementById('devices');
  updateSummary(devices);

  if (devices.length === 0) {
    container.innerHTML = '<p class="empty">No devices yet - waiting for the first one to check in.</p>';
    editingIds.clear();
    expandedOffline.clear();
    return;
  }
  if (container.querySelector('.empty')) container.innerHTML = '';

  devices.forEach((device, i) => {
    if (device.online) expandedOffline.delete(device.id); // it's live now, drop the stale cache

    const existing = container.children[i];
    // Don't rebuild a card the user is actively editing out from under them.
    if (existing && existing.dataset.id === device.id && editingIds.has(device.id)) return;

    const card = renderCard(device);
    card.dataset.id = device.id;

    if (existing && existing.dataset.id === device.id) existing.replaceWith(card);
    else if (existing) container.insertBefore(card, existing);
    else container.appendChild(card);
  });

  while (container.children.length > devices.length) {
    container.removeChild(container.lastChild);
  }
}

async function loadDevices() {
  try {
    const res = await fetch('/api/devices');
    const devices = await res.json();
    document.getElementById('connIssue').style.display = 'none';
    renderList(devices);
  } catch (e) {
    document.getElementById('connIssue').style.display = 'block';
  }
}

setInterval(loadDevices, 1500);
loadDevices();
</script>
</body>
</html>
)rawliteral";

#endif // WEB_UI_H
