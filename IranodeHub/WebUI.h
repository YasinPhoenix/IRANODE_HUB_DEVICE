#ifndef WEB_UI_H
#define WEB_UI_H

// Declared static: this header may end up included by more than one .cpp
// in the future, and `static` gives each translation unit its own private
// copy instead of colliding at link time - the standard way to keep a
// PROGMEM blob safely header-only in Arduino.
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fa" dir="rtl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>Iranode Hub</title>
<style>
:root{
  --bg:#101113;--surface:#17181b;--surface-2:#1e2024;--border:#2a2c30;
  --text:#e8e9ea;--muted:#86898f;
  --tab-bg:#1c1e22;--tab-active-bg:#34383e;--tab-active-text:#f4f4f5;
  --offline-dot:#8a5a3d;--online-dot:#4a8a63;
  --radius-lg:18px;--radius-md:13px;--radius-sm:8px;
}
*{box-sizing:border-box}
body{
  margin:0;background:var(--bg);color:var(--text);
  font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Tahoma,sans-serif;
  padding-top:env(safe-area-inset-top,0);padding-bottom:env(safe-area-inset-bottom,0);
}
.logo-area{
  height:22vh;
  min-height:80px;
  max-height:80px;
  display:flex;
  align-items:center;
  justify-content:space-between;
  gap:10px;
  padding:0 14px;
  background:var(--surface);
  border-bottom:1px solid var(--border);
  /* Physical left/right order regardless of the page's RTL direction -
     same reasoning as .channel-grid: the button belongs on the visual
     left and the logo on the visual right no matter the text direction. */
  direction:ltr;
}
.iranode-logo{
  display:block;
  width:min(40vw,180px);
  height:auto;
  flex:0 0 auto;
}
.ap-config-btn{
  display:flex;
  align-items:center;
  gap:6px;
  flex:0 1 auto;
  min-width:0;
  padding:8px 12px;
  border-radius:var(--radius-sm);
  background:var(--surface-2);
  border:1px solid var(--border);
  color:var(--text);
  text-decoration:none;
  font-size:.78rem;
  white-space:nowrap;
  overflow:hidden;
  text-overflow:ellipsis;
}
.ap-config-btn:active{background:var(--tab-bg)}
.ap-config-btn svg{width:16px;height:16px;flex:0 0 auto}
.ap-config-btn span{overflow:hidden;text-overflow:ellipsis;direction:rtl}
@media (max-width:360px){
  .ap-config-btn span{display:none} /* icon-only on very narrow screens */
  .iranode-logo{width:min(34vw,140px)}
}

.conn-issue{
  display:none;background:#3d1f1f;color:#f2a4a4;
  border-radius:var(--radius-sm);padding:9px 14px;font-size:.82rem;
  margin:12px 16px 0;
}

.tabs-row{display:flex;align-items:center;gap:8px;padding-inline:16px;border-bottom:1px solid var(--border)}
.tabs{
  flex:1;min-width:0;display:flex;gap:8px;overflow-x:auto;padding-block:14px;
  scrollbar-width:none;-ms-overflow-style:none;
}
.tabs::-webkit-scrollbar{display:none}
.tab{
  flex:0 0 auto;padding:9px 16px;border-radius:999px;border:none;font-family:inherit;
  background:var(--tab-bg);color:var(--muted);font-size:.85rem;font-weight:600;
  white-space:nowrap;cursor:pointer;display:flex;align-items:center;gap:7px;
  transition:background .15s ease,color .15s ease;
}
.tab.active{background:var(--tab-active-bg);color:var(--tab-active-text)}
.tab .dot{width:6px;height:6px;border-radius:50%;flex:0 0 auto}
.tab .dot.online{background:var(--online-dot)}
.tab .dot.offline{background:var(--offline-dot)}

.panel{padding:18px 16px 40px;max-width:480px;margin-inline:auto}
.panel-header{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:14px}
.panel-header h2{font-size:1.05rem;margin:0;font-weight:800}
.header-left{display:flex;align-items:center;gap:8px}
.icon-btn{
  width:34px;height:34px;border-radius:10px;border:1px solid var(--border);
  background:linear-gradient(160deg,#24262b,#191b1f);
  display:flex;align-items:center;justify-content:center;
  cursor:pointer;color:var(--muted);flex:0 0 auto;
  box-shadow:0 3px 7px rgba(0,0,0,.6),inset 0 1px 0 rgba(255,255,255,.05);
  transition:background .15s ease,box-shadow .15s ease,color .15s ease;
}
.icon-btn:active{box-shadow:inset 0 2px 5px rgba(0,0,0,.6)}
.icon-btn.active{
  background:linear-gradient(160deg,#3d424a,#2b2f35);color:var(--text);
  border-color:var(--tab-active-bg);
  box-shadow:0 3px 8px rgba(0,0,0,.55),inset 0 1px 0 rgba(255,255,255,.08);
}
.icon-btn svg{width:18px;height:18px;display:block}
.status-badge{
  display:inline-flex;align-items:center;gap:6px;
  background:var(--surface-2);border:1px solid var(--border);
  border-radius:999px;padding:6px 12px;font-size:.76rem;color:var(--muted);font-weight:600;
}
.status-badge .dot{width:6px;height:6px;border-radius:50%}
.status-badge.online .dot{background:var(--online-dot)}
.status-badge.offline .dot{background:var(--offline-dot)}
.name-input{
  font-size:1.05rem;font-weight:800;color:var(--text);background:var(--surface-2);
  border:1px solid var(--border);border-radius:var(--radius-sm);padding:6px 10px;
  width:100%;max-width:220px;font-family:inherit;
}
.name-input:focus{outline:none;border-color:var(--muted)}

.info-note{
  background:var(--surface);border:1px solid var(--border);border-radius:var(--radius-md);
  padding:12px 14px;color:var(--muted);font-size:.82rem;margin-bottom:16px;line-height:1.6;
}

.channel-grid{
  --ch-gap:12px;
  display:grid;grid-template-columns:repeat(auto-fit,100px);gap:var(--ch-gap);
  /* space-evenly (not center): whatever width is left over after laying
     out the fixed-size buttons gets split into equal gaps between AND
     around them, so the edge-to-button gap always matches the
     button-to-button gap - no per-variant spacing needed. auto-fit (vs
     auto-fill) collapses any unused track to 0 width first, so this math
     isn't thrown off by a phantom empty column. Top/bottom padding is
     tied to the same --ch-gap so the space above the first row and below
     the last row matches the row-gap too (relevant once a variant has
     more than one row, e.g. the 4-switch 2x2 grid). */
  justify-content:space-evenly;padding:var(--ch-gap) 0;
  direction:ltr;
}
/* 4-switch devices: force a fixed 2x2 grid (ch1|ch2 / ch3|ch4) so the
   layout always mirrors the physical unit instead of auto-fit deciding
   how many columns fit (which produced an uneven 3-then-1 split). Spacing
   still comes from justify-content:space-evenly above. */
.channel-grid.ch-count-4{
  grid-template-columns:repeat(2,100px);
}
.channel-btn{
  position:relative;width:100px;height:100px;border:none;border-radius:var(--radius-lg);
  cursor:pointer;padding:0;transition:opacity .12s ease;
}
.channel-btn:active{opacity:.82}
.channel-btn[disabled]{cursor:default;opacity:.45}
.channel-btn .btn-inner{
  position:absolute;inset:12px;border-radius:12px;background:rgba(0,0,0,.24);
  box-shadow:0 2px 8px rgba(0,0,0,.55),inset 0 1px 0 rgba(255,255,255,.06);
  display:flex;flex-direction:column;align-items:center;justify-content:center;gap:5px;
  color:#f5f5f6;
}
.channel-btn .cname{font-size:.8rem;font-weight:700;line-height:1.25;word-break:break-word;padding:0 4px;direction:rtl}
.channel-btn .cstate{font-size:.58rem;opacity:.78;letter-spacing:.02em;direction:rtl}

.ch-0{background:#2b2b2b}
.ch-1{background:#e74c3c}
.ch-2{background:#2ecc71}
.ch-3{background:#3498db}
.ch-4{background:#f1c40f}
.ch-5{background:#1abc9c}
.ch-6{background:#9b59b6}
.ch-7{background:#ffffff}

.channel-block{border-top:1px solid var(--border);padding:14px 0}
.channel-block:first-child{border-top:none;padding-top:0}
.chan-name-input{
  width:100%;font-size:.88rem;font-weight:700;color:var(--text);background:var(--surface-2);
  border:1px solid var(--border);border-radius:var(--radius-sm);padding:7px 10px;
  margin-bottom:12px;font-family:inherit;
}
.chan-name-input:focus{outline:none;border-color:var(--muted)}
.colorblock{margin-top:10px}
.colorblock .label{font-size:.7rem;color:var(--muted);margin-bottom:7px;text-transform:uppercase;letter-spacing:.04em}
.swatches{display:flex;flex-wrap:wrap;gap:8px}
.swatch{
  width:26px;height:26px;border-radius:50%;border:2px solid transparent;cursor:pointer;
  box-shadow:inset 0 0 0 1px rgba(255,255,255,.08),0 2px 6px rgba(0,0,0,.5);
  transition:transform .1s ease,border-color .1s ease,box-shadow .1s ease;
}
.swatch.selected{
  border-color:var(--text);transform:scale(1.12);
  box-shadow:inset 0 0 0 1px rgba(255,255,255,.12),0 3px 7px rgba(0,0,0,.55);
}
</style>
</head>
<body>

<div class="logo-area">
  <a class="ap-config-btn" href="/wifi" aria-label="تنظیمات Wi-Fi">
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
      <path d="M12 20a1.4 1.4 0 1 0 0-2.8 1.4 1.4 0 0 0 0 2.8Z" fill="currentColor"/>
      <path d="M4 9.5c4.5-4 11.5-4 16 0M6.8 13c3.2-2.6 7.2-2.6 10.4 0M9.6 16.4c1.6-1.2 3.2-1.2 4.8 0" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
    </svg>
    <span>تنظیمات Wi-Fi</span>
  </a>
  <svg class="iranode-logo"
       viewBox="0 0 893 256"
       xmlns="http://www.w3.org/2000/svg"
       role="img"
       aria-label="Iranode">
    <path d="M 682 195 L 685 197 L 730 197 L 735 195 L 740 190 L 749 164 L 749 160 L 703 159 L 697 161 L 691 167 Z M 786 71 L 672 71 L 657 75 L 646 82 L 639 89 L 635 96 L 600 195 L 577 254 L 599 255 L 608 253 L 620 248 L 635 236 L 645 221 L 647 213 L 650 208 L 651 201 L 661 177 L 679 125 L 686 115 L 692 112 L 762 112 L 768 110 L 776 102 Z M 541 76 L 538 74 L 505 74 L 500 75 L 491 80 L 486 89 L 465 147 L 459 153 L 450 157 L 425 157 L 423 155 L 448 88 L 449 81 L 444 73 L 438 71 L 343 71 L 333 73 L 322 78 L 309 89 L 299 105 L 273 177 L 273 188 L 275 192 L 282 196 L 355 196 L 357 200 L 351 210 L 343 215 L 299 215 L 278 217 L 267 226 L 256 254 L 362 254 L 378 246 L 389 236 L 397 224 L 407 197 L 463 197 L 474 195 L 493 184 L 501 176 L 508 165 Z M 388 113 L 374 154 L 371 157 L 330 156 L 330 151 L 342 121 L 347 116 L 355 112 Z M 282 86 L 274 77 L 266 73 L 260 72 L 187 71 L 179 73 L 170 81 L 160 110 L 232 110 L 234 112 L 234 116 L 222 146 L 212 155 L 204 157 L 30 157 L 19 161 L 10 169 L 0 196 L 205 197 L 226 193 L 248 180 L 256 172 L 263 162 L 272 135 L 274 133 L 279 117 L 282 112 L 284 104 L 284 93 Z M 475 59 L 522 60 L 526 58 L 530 54 L 536 35 L 539 30 L 539 25 L 491 25 L 483 33 Z M 645 1 L 602 1 L 593 5 L 586 13 L 520 196 L 540 196 L 554 192 L 570 182 L 584 166 Z M 892 1 L 847 1 L 840 4 L 835 9 L 774 176 L 771 181 L 767 196 L 790 196 L 803 192 L 817 184 L 826 176 L 832 168 L 850 122 L 853 110 L 866 77 L 887 15 L 890 10 Z" 
    fill="#ffffff" fill-rule="evenodd"/>
  </svg>
</div>

<div class="tabs-row">
  <div class="tabs" id="tabs"></div>
</div>
<div class="conn-issue" id="connIssue">امکان دسترسی به هاب وجود ندارد - در حال تلاش مجدد…</div>
<div class="panel" id="panel"></div>

<script>
// Index 0-7 lines up with the switch firmware's RGBColor enum
// (COLOR_OFF..COLOR_WHITE in Config.h) - this array only supplies the
// Persian display name, the swatch colors themselves come from the
// .ch-0..ch-7 CSS classes above.
const COLOR_NAMES = ["خاموش","قرمز","سبز","آبی","زرد","فیروزه‌ای","بنفش","سفید"];
const ICON_EDIT = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 20h9"/><path d="M16.5 3.5a2.12 2.12 0 0 1 3 3L7 19l-4 1 1-4Z"/></svg>';
const ICON_DONE = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20 6 9 17l-5-5"/></svg>';

let devices = [];       // stable local order - only ever appended to, never re-sorted by a poll
let activeId = null;
let panelEditMode = false;

const tabsEl = document.getElementById('tabs');
const panelEl = document.getElementById('panel');
const connIssueEl = document.getElementById('connIssue');

function getDevice(id) {
  return devices.find(d => d.id === id) || null;
}
function displayName(dev) {
  return dev.name && dev.name.length ? dev.name : dev.id;
}
function formatAge(sec) {
  if (sec < 60) return sec + ' ثانیه پیش';
  if (sec < 3600) return Math.floor(sec / 60) + ' دقیقه پیش';
  if (sec < 86400) return Math.floor(sec / 3600) + ' ساعت پیش';
  return Math.floor(sec / 86400) + ' روز پیش';
}
function postForm(url, params) {
  return fetch(url, { method: 'POST', headers: {'Content-Type':'application/x-www-form-urlencoded'}, body: new URLSearchParams(params) });
}
function submitOnEnter(input) {
  input.addEventListener('keydown', e => { if (e.key === 'Enter') input.blur(); });
}

/* ---------- polling + local device list ----------
   /api/devices only inlines full detail (name + channels) for a device
   that's currently online - an offline entry is just {id, online:false[,
   lastSeenSec]}. mergeDevices() only ever copies keys a payload actually
   carries, so a device's last-known name/channels stay put across polls
   even after it drops offline or before we've ever fetched its detail;
   ensureDetail() is what fills that in the first time a device is opened,
   same "fetch only on demand" approach as the previous card-based UI.
   Tab order is intentionally never touched here - a device keeps its
   position for as long as the hub still reports it at all, regardless of
   which devices go on/offline between polls. */

function mergeDevices(fresh) {
  fresh.forEach(fd => {
    const existing = getDevice(fd.id);
    if (existing) {
      Object.assign(existing, fd);
    } else {
      devices.push(fd);
      if (activeId === null) activeId = fd.id;
    }
  });
  const freshIds = fresh.map(fd => fd.id);
  devices = devices.filter(d => freshIds.includes(d.id));
  if (activeId !== null && !getDevice(activeId)) {
    activeId = devices.length ? devices[0].id : null;
  }
}

async function ensureDetail(id) {
  const dev = getDevice(id);
  if (!dev || dev.switch !== undefined) return; // already known, online or previously fetched
  try {
    const res = await fetch('/api/device/detail?id=' + id);
    const detail = await res.json();
    if (!detail.found) return;
    Object.assign(dev, detail);
    if (activeId === id) renderPanel();
  } catch (e) { /* stays on the id-only fallback; next poll or reselect retries */ }
}

async function loadDevices() {
  try {
    const res = await fetch('/api/devices');
    const fresh = await res.json();
    connIssueEl.style.display = 'none';
    mergeDevices(fresh);
    renderTabs();
    if (!panelEditMode) renderPanel(); // don't clobber an in-progress edit under the user's hands
    if (activeId) ensureDetail(activeId);
  } catch (e) {
    connIssueEl.style.display = 'block';
  }
}

/* ---------- tabs ---------- */

function renderTabs() {
  tabsEl.innerHTML = '';
  devices.forEach(d => {
    const tab = document.createElement('div');
    tab.className = 'tab' + (d.id === activeId ? ' active' : '');
    tab.dataset.id = d.id;
    const dot = document.createElement('span');
    dot.className = 'dot ' + (d.online ? 'online' : 'offline');
    const label = document.createElement('span');
    label.textContent = displayName(d);
    tab.appendChild(dot);
    tab.appendChild(label);
    tab.addEventListener('click', () => {
      activeId = d.id;
      panelEditMode = false;
      renderTabs();
      renderPanel();
      ensureDetail(d.id);
    });
    tabsEl.appendChild(tab);
  });
}

/* ---------- device panel ---------- */

function swatchRow(dev, chIndex, slot, current) {
  const wrap = document.createElement('div');
  wrap.className = 'swatches';
  for (let c = 0; c < 8; c++) {
    const sw = document.createElement('div');
    sw.className = 'swatch ch-' + c + (c === current ? ' selected' : '');
    sw.title = COLOR_NAMES[c];
    sw.addEventListener('click', () => {
      if (slot === 1) dev.switch.channels[chIndex].colorOn = c;
      else dev.switch.channels[chIndex].colorOff = c;
      wrap.querySelectorAll('.swatch').forEach(s => s.classList.remove('selected'));
      sw.classList.add('selected');
      postForm('/api/device/color', { id: dev.id, channel: chIndex + 1, slot: slot, color: c });
    });
    wrap.appendChild(sw);
  }
  return wrap;
}

function renderPanel() {
  panelEl.innerHTML = '';

  if (!activeId) {
    const note = document.createElement('div');
    note.className = 'info-note';
    note.textContent = devices.length === 0
      ? 'هنوز دستگاهی یافت نشد - در انتظار اتصال اولین دستگاه…'
      : 'در حال بارگذاری…';
    panelEl.appendChild(note);
    return;
  }
  const dev = getDevice(activeId);
  if (!dev) return;

  const header = document.createElement('div');
  header.className = 'panel-header';

  if (panelEditMode) {
    const nameInput = document.createElement('input');
    nameInput.type = 'text';
    nameInput.className = 'name-input';
    nameInput.value = dev.name || '';
    nameInput.placeholder = dev.id;
    submitOnEnter(nameInput);
    nameInput.addEventListener('blur', () => {
      dev.name = nameInput.value;
      postForm('/api/device/name', { id: dev.id, name: nameInput.value });
      renderTabs();
    });
    header.appendChild(nameInput);
  } else {
    const h2 = document.createElement('h2');
    h2.textContent = displayName(dev);
    header.appendChild(h2);
  }

  const headerLeft = document.createElement('div');
  headerLeft.className = 'header-left';
  const editBtn = document.createElement('button');
  editBtn.className = 'icon-btn' + (panelEditMode ? ' active' : '');
  editBtn.innerHTML = panelEditMode ? ICON_DONE : ICON_EDIT;
  editBtn.title = 'ویرایش نام‌ها و رنگ‌ها';
  editBtn.addEventListener('click', () => { panelEditMode = !panelEditMode; renderPanel(); });
  headerLeft.appendChild(editBtn);

  const badge = document.createElement('span');
  badge.className = 'status-badge ' + (dev.online ? 'online' : 'offline');
  const bdot = document.createElement('span');
  bdot.className = 'dot';
  badge.appendChild(bdot);
  badge.appendChild(document.createTextNode(dev.online ? 'آنلاین' : 'آفلاین'));
  headerLeft.appendChild(badge);

  header.appendChild(headerLeft);
  panelEl.appendChild(header);

  if (!dev.online) {
    const offNote = document.createElement('div');
    offNote.className = 'info-note';
    offNote.textContent = 'این دستگاه اکنون آفلاین است. آخرین وضعیت شناخته‌شده نمایش داده می‌شود و قابل تغییر نیست.'
      + (dev.lastSeenSec != null ? (' (آخرین اتصال: ' + formatAge(dev.lastSeenSec) + ')') : '');
    panelEl.appendChild(offNote);
  }

  if (dev.switch === undefined) {
    const loading = document.createElement('div');
    loading.className = 'info-note';
    loading.textContent = 'در حال دریافت اطلاعات کانال‌ها…';
    panelEl.appendChild(loading);
    return;
  }

  if (panelEditMode) {
    dev.switch.channels.forEach((ch, i) => {
      const card = document.createElement('div');
      card.className = 'channel-block';

      const nameField = document.createElement('input');
      nameField.type = 'text';
      nameField.className = 'chan-name-input';
      nameField.value = ch.name || '';
      nameField.placeholder = 'کانال ' + (i + 1);
      submitOnEnter(nameField);
      nameField.addEventListener('blur', () => {
        ch.name = nameField.value;
        postForm('/api/device/channel-name', { id: dev.id, channel: i + 1, name: nameField.value });
      });
      card.appendChild(nameField);

      const onBlock = document.createElement('div');
      onBlock.className = 'colorblock';
      const onLabel = document.createElement('div');
      onLabel.className = 'label';
      onLabel.textContent = 'رنگ هنگام روشن بودن';
      onBlock.appendChild(onLabel);
      onBlock.appendChild(swatchRow(dev, i, 1, ch.colorOn));
      card.appendChild(onBlock);

      const offBlock = document.createElement('div');
      offBlock.className = 'colorblock';
      const offLabel = document.createElement('div');
      offLabel.className = 'label';
      offLabel.textContent = 'رنگ هنگام خاموش بودن';
      offBlock.appendChild(offLabel);
      offBlock.appendChild(swatchRow(dev, i, 0, ch.colorOff));
      card.appendChild(offBlock);

      panelEl.appendChild(card);
    });
    return;
  }

  const grid = document.createElement('div');
  grid.className = 'channel-grid ch-count-' + dev.switch.channels.length;
  dev.switch.channels.forEach((ch, i) => {
    const color = ch.relay ? ch.colorOn : ch.colorOff;
    const btn = document.createElement('button');
    btn.className = 'channel-btn ch-' + color;
    btn.disabled = !dev.online;
    const inner = document.createElement('div');
    inner.className = 'btn-inner';
    const name = document.createElement('div');
    name.className = 'cname';
    name.textContent = ch.name && ch.name.length ? ch.name : ('کانال ' + (i + 1));
    const state = document.createElement('div');
    state.className = 'cstate';
    state.textContent = ch.relay ? 'روشن' : 'خاموش';
    inner.appendChild(name);
    inner.appendChild(state);
    btn.appendChild(inner);
    btn.addEventListener('click', () => {
      const next = !ch.relay;
      ch.relay = next;
      renderPanel();
      postForm('/api/device/relay', { id: dev.id, channel: i + 1, value: next ? 1 : 0 });
    });
    grid.appendChild(btn);
  });
  panelEl.appendChild(grid);

  // justify-content:space-evenly gives the columns a gap based on
  // leftover width, which changes with screen size - there's no
  // equivalent leftover height for row-gap to grow into, so it stays
  // stuck at --ch-gap. Once the grid has actually rendered, measure the
  // real gap space-evenly produced between two buttons on the same row
  // and apply that as the row-gap too, so rows (currently only the
  // 4-switch's 2x2 grid) end up spaced the same as columns.
  const first = grid.children[0];
  const second = grid.children[1];
  if (first && second) {
    const hGap = second.getBoundingClientRect().left - first.getBoundingClientRect().right;
    if (hGap > 0) grid.style.rowGap = hGap + 'px';
  }
}

setInterval(loadDevices, 300);
loadDevices();
</script>
</body>
</html>
)rawliteral";

// The AP configuration page - reachable at all times via /wifi, and the
// *only* page reachable while the hub is still on its default
// "IranodeHub-<id>" / no-password AP (see WebApi::handleRoot()). Kept as
// its own self-contained document rather than a mode of INDEX_HTML: the
// two pages have almost nothing in common (no device list/tabs/polling
// here) and INDEX_HTML is only ever served once the hub is already
// configured, so there's no state it needs to share with this page.
static const char WIFI_CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fa" dir="rtl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>تنظیمات Wi-Fi - Iranode Hub</title>
<style>
:root{
  --bg:#101113;--surface:#17181b;--surface-2:#1e2024;--border:#2a2c30;
  --text:#e8e9ea;--muted:#86898f;--accent:#e05a4e;
  --offline-dot:#8a5a3d;--online-dot:#4a8a63;
  --radius-lg:18px;--radius-md:13px;--radius-sm:8px;
}
*{box-sizing:border-box}
body{
  margin:0;background:var(--bg);color:var(--text);
  font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Tahoma,sans-serif;
  padding:20px 16px 40px;
  padding-top:calc(20px + env(safe-area-inset-top,0));
  padding-bottom:calc(40px + env(safe-area-inset-bottom,0));
}
.wrap{max-width:420px;margin:0 auto}
h1{font-size:1.05rem;margin:0 0 4px;text-align:center}
.status{
  display:flex;align-items:center;gap:8px;justify-content:center;
  font-size:.8rem;color:var(--muted);margin:6px 0 20px;
}
.status .dot{width:8px;height:8px;border-radius:50%;flex:0 0 auto}
.status.unconfigured .dot{background:var(--offline-dot)}
.status.configured .dot{background:var(--online-dot)}
.card{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius-lg);padding:18px}
.card + .card{margin-top:14px}
.card h2{font-size:.85rem;margin:0 0 4px;color:var(--text)}
.card .desc{font-size:.76rem;color:var(--muted);line-height:1.7;margin-bottom:14px}
label{display:block;font-size:.8rem;color:var(--muted);margin-bottom:6px;margin-top:14px}
label:first-of-type{margin-top:0}
input{
  width:100%;font-size:.92rem;color:var(--text);background:var(--surface-2);
  border:1px solid var(--border);border-radius:var(--radius-sm);padding:10px 12px;
  font-family:inherit;direction:ltr;text-align:right;
}
input:focus{outline:none;border-color:var(--muted)}
.hint{font-size:.72rem;color:var(--muted);margin-top:6px;line-height:1.6}
button{
  width:100%;margin-top:18px;padding:12px;border:none;border-radius:var(--radius-sm);
  background:var(--accent);color:#fff;font-size:.92rem;font-weight:700;cursor:pointer;
  font-family:inherit;
}
button:disabled{opacity:.5;cursor:default}
.msg{margin-top:12px;font-size:.8rem;line-height:1.6;display:none}
.msg.show{display:block}
.msg.err{color:#f2a4a4}
.msg.ok{color:#8fd19e}
.back-link{
  display:block;text-align:center;margin-top:18px;color:var(--muted);
  font-size:.8rem;text-decoration:none;
}
.back-link.show{display:block}
.back-link{display:none}
</style>
</head>
<body>
<div class="wrap">
  <h1>تنظیمات دسترسی (Wi-Fi) هاب</h1>
  <div class="status" id="status"><span class="dot"></span><span id="statusText">در حال بارگذاری…</span></div>

  <div class="card">
    <h2>نام و رمز شبکه هاب</h2>
    <div class="desc">دستگاه‌ها (کلیدهای دیواری) برای اتصال به هاب، به همین نام و رمز نیاز دارند. پس از ذخیره، هاب و همه دستگاه‌های متصل باید دوباره به شبکه جدید متصل شوند.</div>

    <label for="ssid">نام شبکه (SSID)</label>
    <input id="ssid" type="text" maxlength="32" autocomplete="off">

    <label for="password">رمز عبور</label>
    <input id="password" type="password" maxlength="64" autocomplete="off">
    <div class="hint" id="passHint">برای شبکه بدون رمز خالی بگذارید، در غیر این صورت حداقل ۸ کاراکتر.</div>

    <label for="maxConn">حداکثر تعداد دستگاه متصل</label>
    <input id="maxConn" type="number" min="1" max="10" step="1">

    <button id="saveBtn">ذخیره و راه‌اندازی مجدد</button>
    <div class="msg" id="msg"></div>
  </div>

  <a class="back-link" id="backLink" href="/">بازگشت به صفحه اصلی هاب</a>
</div>

<script>
const statusEl = document.getElementById('status');
const statusTextEl = document.getElementById('statusText');
const ssidEl = document.getElementById('ssid');
const passEl = document.getElementById('password');
const maxConnEl = document.getElementById('maxConn');
const saveBtn = document.getElementById('saveBtn');
const msgEl = document.getElementById('msg');
const backLink = document.getElementById('backLink');
const passHint = document.getElementById('passHint');

let minPassLen = 8, maxSsidLen = 32, maxPassLen = 64, minMaxConn = 1, maxMaxConn = 10;

function showMsg(text, isError) {
  msgEl.textContent = text;
  msgEl.className = 'msg show ' + (isError ? 'err' : 'ok');
}

function loadStatus() {
  fetch('/api/wifi-config').then(r => r.json()).then(d => {
    minPassLen = d.minPasswordLen; maxSsidLen = d.maxSsidLen; maxPassLen = d.maxPasswordLen;
    minMaxConn = d.minMaxConn; maxMaxConn = d.maxMaxConn;
    ssidEl.maxLength = maxSsidLen; passEl.maxLength = maxPassLen;
    maxConnEl.min = minMaxConn; maxConnEl.max = maxMaxConn;
    passHint.textContent = 'برای شبکه بدون رمز خالی بگذارید، در غیر این صورت حداقل ' + minPassLen + ' کاراکتر.';

    if (d.configured) {
      statusEl.className = 'status configured';
      statusTextEl.textContent = 'هاب پیکربندی شده است';
      backLink.className = 'back-link show';
    } else {
      statusEl.className = 'status unconfigured';
      statusTextEl.textContent = 'هاب هنوز پیکربندی نشده - شبکه پیش‌فرض: ' + d.defaultSsid;
      backLink.className = 'back-link';
    }
    ssidEl.value = d.configured ? d.ssid : '';
    ssidEl.placeholder = d.configured ? '' : d.defaultSsid;
    maxConnEl.value = d.maxConnections;
  }).catch(() => {
    statusTextEl.textContent = 'خطا در دریافت وضعیت';
  });
}
loadStatus();

saveBtn.addEventListener('click', () => {
  const ssid = ssidEl.value.trim();
  const password = passEl.value;
  const maxConn = parseInt(maxConnEl.value, 10);

  if (!ssid || ssid.length > maxSsidLen) {
    showMsg('نام شبکه باید بین ۱ تا ' + maxSsidLen + ' کاراکتر باشد.', true);
    return;
  }
  if (password.length !== 0 && password.length < minPassLen) {
    showMsg('رمز عبور باید خالی یا حداقل ' + minPassLen + ' کاراکتر باشد.', true);
    return;
  }
  if (!maxConn || maxConn < minMaxConn || maxConn > maxMaxConn) {
    showMsg('حداکثر تعداد دستگاه باید بین ' + minMaxConn + ' تا ' + maxMaxConn + ' باشد.', true);
    return;
  }

  saveBtn.disabled = true;
  const body = new URLSearchParams({ ssid, password, maxConnections: String(maxConn) });
  fetch('/api/wifi-config', { method: 'POST', headers: {'Content-Type':'application/x-www-form-urlencoded'}, body })
    .then(async r => {
      const text = await r.text();
      if (r.ok) {
        showMsg('ذخیره شد - هاب در حال راه‌اندازی مجدد با شبکه جدید است…', false);
      } else {
        showMsg(text || 'خطا در ذخیره‌سازی', true);
        saveBtn.disabled = false;
      }
    })
    .catch(() => { showMsg('ارتباط با هاب برقرار نشد.', true); saveBtn.disabled = false; });
});
</script>
</body>
</html>
)rawliteral";

#endif // WEB_UI_H