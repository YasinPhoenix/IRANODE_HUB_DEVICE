#ifndef CONFIG_UI_H
#define CONFIG_UI_H

// Served only while the switch is in Wi-Fi configuration mode (see
// WifiConfigManager) - styled to match the hub's own dark-theme, Persian
// RTL web UI (see IranodeHub/WebUI.h), since this project's only other web
// UI to be consistent with is the hub's.
static const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fa" dir="rtl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>تنظیمات Wi-Fi - Iranode Switch</title>
<style>
:root{
  --bg:#101113;--surface:#17181b;--surface-2:#1e2024;--border:#2a2c30;
  --text:#e8e9ea;--muted:#86898f;--accent:#e05a4e;
  --radius-lg:18px;--radius-sm:8px;
}
*{box-sizing:border-box}
body{
  margin:0;background:var(--bg);color:var(--text);
  font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Tahoma,sans-serif;
  padding:24px 16px;
  padding-top:calc(24px + env(safe-area-inset-top,0));
  padding-bottom:calc(24px + env(safe-area-inset-bottom,0));
}
.card{max-width:400px;margin:0 auto;background:var(--surface);border:1px solid var(--border);border-radius:var(--radius-lg);padding:20px}
h1{font-size:1.05rem;margin:0 0 6px}
.sub{color:var(--muted);font-size:.8rem;margin-bottom:18px;line-height:1.7}
label{display:block;font-size:.8rem;color:var(--muted);margin-bottom:6px;margin-top:14px}
input{
  width:100%;font-size:.92rem;color:var(--text);background:var(--surface-2);
  border:1px solid var(--border);border-radius:var(--radius-sm);padding:10px 12px;
  font-family:inherit;direction:ltr;text-align:right;
}
input:focus{outline:none;border-color:var(--muted)}
button{
  width:100%;margin-top:20px;padding:12px;border:none;border-radius:var(--radius-sm);
  background:var(--accent);color:#fff;font-size:.92rem;font-weight:700;cursor:pointer;
  font-family:inherit;
}
button:disabled{opacity:.5;cursor:default}
.msg{margin-top:14px;font-size:.8rem;line-height:1.6;display:none}
.msg.show{display:block}
.msg.err{color:#f2a4a4}
.msg.ok{color:#8fd19e}
.hint{font-size:.72rem;color:var(--muted);margin-top:6px;line-height:1.6}
</style>
</head>
<body>
<div class="card">
  <h1>تنظیمات Wi-Fi دستگاه</h1>
  <div class="sub" id="subtitle">نام و رمز شبکه هاب را وارد کنید تا این کلید به آن متصل شود.</div>

  <label for="ssid">نام شبکه (SSID)</label>
  <input id="ssid" type="text" maxlength="32" autocomplete="off">

  <label for="password">رمز عبور</label>
  <input id="password" type="password" maxlength="64" autocomplete="off">
  <div class="hint">برای شبکه بدون رمز خالی بگذارید، در غیر این صورت حداقل ۸ کاراکتر.</div>

  <button id="saveBtn">ذخیره و اتصال</button>
  <div class="msg" id="msg"></div>
</div>

<script>
const ssidEl = document.getElementById('ssid');
const passEl = document.getElementById('password');
const btnEl = document.getElementById('saveBtn');
const msgEl = document.getElementById('msg');
const subEl = document.getElementById('subtitle');

function showMsg(text, isError) {
  msgEl.textContent = text;
  msgEl.className = 'msg show ' + (isError ? 'err' : 'ok');
}

fetch('/api/wifi').then(r => r.json()).then(d => {
  if (d.apSsid) {
    subEl.textContent = 'شما به شبکه تنظیمات «' + d.apSsid + '» متصل هستید. نام و رمز شبکه هاب را وارد کنید.';
  }
}).catch(() => {});

btnEl.addEventListener('click', () => {
  const ssid = ssidEl.value.trim();
  if (!ssid) {
    showMsg('نام شبکه الزامی است.', true);
    return;
  }
  btnEl.disabled = true;
  const body = new URLSearchParams({ ssid: ssid, password: passEl.value });
  fetch('/api/wifi', { method: 'POST', headers: {'Content-Type':'application/x-www-form-urlencoded'}, body: body })
    .then(async r => {
      const text = await r.text();
      if (r.ok) {
        showMsg('ذخیره شد - دستگاه در حال راه‌اندازی مجدد است و به شبکه جدید متصل می‌شود…', false);
      } else {
        showMsg(text || 'خطا در ذخیره‌سازی', true);
        btnEl.disabled = false;
      }
    })
    .catch(() => { showMsg('ارتباط با دستگاه برقرار نشد.', true); btnEl.disabled = false; });
});
</script>
</body>
</html>
)rawliteral";

#endif // CONFIG_UI_H
