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
<title>IRANODE HUB</title>
<style>
  :root { color-scheme: dark; }
  body { font-family: -apple-system, Arial, sans-serif; background:#111; color:#eee; margin:0; padding:20px; }
  .container { max-width: 900px; margin:auto; }
  h1 { margin-bottom: 18px; font-size: 1.3em; }
  .card { background:#1d1d1d; border-radius:10px; padding:14px 16px; margin-bottom:10px; }
  .card.collapsed { cursor:pointer; opacity:0.75; }
  .row { display:flex; justify-content:space-between; align-items:center; gap:10px; }
  .badge { font-size:0.75em; padding:2px 9px; border-radius:12px; white-space:nowrap; }
  .badge.online { background:#16342a; color:#4ade80; }
  .badge.offline { background:#3a1f1f; color:#f87171; }
  .id { font-family: monospace; color:#999; font-size:0.85em; }
  .channel { display:flex; justify-content:space-between; align-items:center; padding:7px 0; border-top:1px solid #2a2a2a; }
  .channel:first-of-type { border-top:none; margin-top:6px; }
  .swatch { display:inline-block; width:13px; height:13px; border-radius:3px; vertical-align:middle; margin-right:7px; }
  button.toggle { padding:4px 14px; border-radius:6px; border:none; cursor:pointer; font-size:0.9em; }
  button.toggle.on { background:#16342a; color:#4ade80; }
  button.toggle.off { background:#333; color:#aaa; }
  details { margin-top:2px; }
  summary { cursor:pointer; color:#888; font-size:0.8em; padding:4px 0; }
  .colorrow { margin-top:6px; }
  .colorrow .label { font-size:0.75em; color:#777; margin-bottom:4px; }
  .colorpick { display:flex; gap:6px; flex-wrap:wrap; }
  .colorpick button { width:22px; height:22px; border-radius:5px; border:2px solid transparent; cursor:pointer; padding:0; }
  .colorpick button.selected { border-color:#fff; }
  .small { color:#888; font-size:0.8em; margin-top:8px; }
  p.empty { color:#888; }
</style>
</head>
<body>
<div class="container">
  <h1>IRANODE HUB</h1>
  <div id="devices">Loading…</div>
</div>
<script>
const COLOR_NAMES = ['off','red','green','blue','yellow','cyan','magenta','white'];
const COLOR_HEX   = ['#333','#f44', '#4f8',  '#48f','#fd4',   '#4de', '#f4e',   '#fff'];

// Detail the user has explicitly asked to see for a currently-offline
// device (via clicking its collapsed card) - kept client-side so the next
// poll doesn't collapse it back before the user is done looking.
const expandedOffline = new Map();

function el(html) {
  const t = document.createElement('template');
  t.innerHTML = html.trim();
  return t.content.firstElementChild;
}

function switchCardHtml(device, interactive) {
  const badge = device.online
    ? '<span class="badge online">ONLINE</span>'
    : '<span class="badge offline">OFFLINE</span>';

  let body = `<div class="row"><span class="id">${device.id}</span>${badge}</div>`;

  device.switch.channels.forEach((ch, i) => {
    const swatchColor = COLOR_HEX[ch.relay ? ch.colorOn : ch.colorOff];
    body += `<div class="channel">
      <span><span class="swatch" style="background:${swatchColor}"></span>CH${i + 1}</span>`;

    body += interactive
      ? `<button class="toggle ${ch.relay ? 'on' : 'off'}" data-action="relay" data-ch="${i + 1}" data-value="${ch.relay ? 0 : 1}">${ch.relay ? 'ON' : 'OFF'}</button>`
      : `<span>${ch.relay ? 'ON' : 'OFF'}</span>`;
    body += `</div>`;

    body += `<details><summary>Colors ▾</summary>`;
    // slotValue matches SET_COLOR's `value` field convention exactly:
    // 1 = on-color slot, 0 = off-color slot - not the loop's array index.
    [[1, 'On', ch.colorOn], [0, 'Off', ch.colorOff]].forEach(([slotValue, label, current]) => {
      body += `<div class="colorrow"><div class="label">${label}-color</div><div class="colorpick" data-ch="${i + 1}" data-slot="${slotValue}">`;
      COLOR_NAMES.forEach((name, c) => {
        const sel = current === c ? ' selected' : '';
        body += interactive
          ? `<button title="${name}" style="background:${COLOR_HEX[c]}" class="${sel.trim()}" data-action="color" data-color="${c}"></button>`
          : `<span title="${name}" style="display:inline-block;width:22px;height:22px;border-radius:5px;background:${COLOR_HEX[c]};margin-right:6px;${sel ? 'outline:2px solid #fff' : ''}"></span>`;
      });
      body += `</div></div>`;
    });
    body += `</details>`;
  });

  return body;
}

function bindSwitchCardEvents(cardEl, deviceId) {
  cardEl.querySelectorAll('button[data-action="relay"]').forEach(btn => {
    btn.addEventListener('click', () => {
      const params = new URLSearchParams({ id: deviceId, channel: btn.dataset.ch, value: btn.dataset.value });
      fetch('/api/device/relay', { method: 'POST', headers: {'Content-Type':'application/x-www-form-urlencoded'}, body: params });
    });
  });
  cardEl.querySelectorAll('.colorpick').forEach(pick => {
    pick.querySelectorAll('button[data-action="color"]').forEach(btn => {
      btn.addEventListener('click', () => {
        const params = new URLSearchParams({ id: deviceId, channel: pick.dataset.ch, slot: pick.dataset.slot, color: btn.dataset.color });
        fetch('/api/device/color', { method: 'POST', headers: {'Content-Type':'application/x-www-form-urlencoded'}, body: params });
      });
    });
  });
}

function renderGenericCard(device) {
  const badge = device.online ? '<span class="badge online">ONLINE</span>' : '<span class="badge offline">OFFLINE</span>';
  return el(`<div class="card"><div class="row"><span class="id">${device.id}</span>${badge}</div></div>`);
}

function renderCollapsed(device) {
  const card = el(`<div class="card collapsed"><div class="row"><span class="id">${device.id}</span><span class="badge offline">OFFLINE</span></div></div>`);
  card.addEventListener('click', async () => {
    try {
      const res = await fetch('/api/device/detail?id=' + device.id);
      const detail = await res.json();
      if (!detail.found) return;
      detail.online = false;
      expandedOffline.set(device.id, detail);
      const expanded = detail.switch ? el(`<div class="card">${switchCardHtml(detail, false)}</div>`) : renderGenericCard(detail);
      expanded.dataset.id = device.id;
      card.replaceWith(expanded);
    } catch (e) { console.log(e); }
  });
  return card;
}

function renderCard(device) {
  if (device.online) {
    if (!device.switch) return renderGenericCard(device);
    const card = el(`<div class="card">${switchCardHtml(device, true)}</div>`);
    bindSwitchCardEvents(card, device.id);
    return card;
  }
  if (expandedOffline.has(device.id)) {
    const cached = expandedOffline.get(device.id);
    return cached.switch ? el(`<div class="card">${switchCardHtml(cached, false)}</div>`) : renderGenericCard(cached);
  }
  return renderCollapsed(device);
}

async function loadDevices() {
  try {
    const res = await fetch('/api/devices');
    const devices = await res.json();
    const container = document.getElementById('devices');

    if (devices.length === 0) {
      container.innerHTML = '<p class="empty">No devices yet.</p>';
      expandedOffline.clear();
      return;
    }
    if (container.querySelector('.empty')) container.innerHTML = '';

    devices.forEach((device, i) => {
      if (device.online) expandedOffline.delete(device.id); // it's live now, drop the stale cache

      const existing = container.children[i];
      // Leave a card alone entirely while the user has its color picker
      // open in it - a poll rebuilding the DOM under them would snap it
      // shut mid-choice.
      if (existing && existing.dataset.id === device.id && existing.querySelector('details[open]')) {
        return;
      }

      const card = renderCard(device);
      card.dataset.id = device.id;

      if (existing && existing.dataset.id === device.id) existing.replaceWith(card);
      else if (existing) container.insertBefore(card, existing);
      else container.appendChild(card);
    });

    while (container.children.length > devices.length) {
      container.removeChild(container.lastChild);
    }
  } catch (e) { console.log(e); }
}

setInterval(loadDevices, 1500);
loadDevices();
</script>
</body>
</html>
)rawliteral";

#endif // WEB_UI_H
