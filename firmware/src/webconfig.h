#pragma once
// Local web setup page: http://<hostname>.local (mDNS) or http://<device ip>.
// For options the on-device Settings screen does not have: start screen, language, clock and greeting,
// weather location/units, payment QR details, app list and order, device name and page password.
// Included by main.cpp after pages.h (uses cfg, sdOk, FW_VERSION, appName()).
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

static WebServer web(80);
static String webHostName = "zin100";
static String webPassword;  // empty = no login; user name is always "admin"
static bool webStarted = false, webWasConnected = false;
static uint32_t webRestartAt = 0;
static const char *appName(int i);  // main.cpp

static const char WEB_PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1"><title>Zin100</title>
<style>
:root{color-scheme:dark;--bg:#0b1020;--card:#151b33;--line:#2a3359;--txt:#f2f5ff;--mut:#8a93b8;--acc:#38e1ff;--warn:#ffb84a;--bad:#ff5c7a;--ok:#3deba0}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--txt);font:16px system-ui,sans-serif}
main{max-width:560px;margin:auto;padding:20px 16px 60px}h1{font-size:22px;margin:4px 0 2px}
.sub{color:var(--mut);font-size:14px;margin-bottom:12px}
section{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:14px 16px;margin:12px 0}
h2{font-size:13px;letter-spacing:.08em;color:var(--mut);margin:0 0 6px;font-weight:600;text-transform:uppercase}
p.hint{color:var(--mut);font-size:13px;margin:2px 0 8px}
label{display:block;color:var(--mut);font-size:14px;margin:10px 0 4px}
input,select,textarea,button{width:100%;padding:11px 12px;border-radius:10px;border:1px solid var(--line);background:#0b1020;color:var(--txt);font:inherit}
textarea{min-height:92px;font-family:ui-monospace,monospace;font-size:13px;word-break:break-all}
input[type=checkbox]{width:20px;height:20px;padding:0;accent-color:var(--acc)}
.chk{display:flex;align-items:center;gap:10px;color:var(--txt);margin:12px 0 2px}
button{background:var(--acc);color:#04101a;font-weight:600;border:0;margin-top:12px;cursor:pointer}
button.sec{background:#1f2849;color:var(--txt);border:1px solid var(--line)}
.row{display:flex;justify-content:space-between;gap:12px;padding:5px 0;font-size:15px}.row span:first-child{color:var(--mut)}
.row span:last-child{text-align:right;word-break:break-all}
.two{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.app{display:flex;align-items:center;gap:8px;padding:8px 0;border-top:1px solid var(--line)}.app:first-child{border-top:0}
.app span{flex:1}.app button{width:40px;margin:0;padding:6px 0}.app input{margin:0}
.res{padding:10px 12px;border:1px solid var(--line);border-radius:10px;margin-top:8px;cursor:pointer}.res:hover{border-color:var(--acc)}
.res small{color:var(--mut);display:block}
#msg{position:fixed;left:16px;right:16px;bottom:16px;max-width:528px;margin:auto;padding:12px 14px;border-radius:12px;background:var(--ok);color:#04101a;font-weight:600;display:none}
#msg.err{background:var(--bad)}
</style></head><body><main>
<h1>Zin100</h1><div class=sub id=addr></div>
<section><h2 data-t=status></h2><div id=info></div></section>

<section><h2 data-t=lang></h2>
<select id=lang><option value=0>Tiếng Việt<option value=1>English</select>
<p class=hint data-t=restartNote></p>
<button id=saveLang data-t=save></button></section>

<section><h2 data-t=startTitle></h2>
<label data-t=startLbl></label><select id=start><option value=0 data-t=tHome><option value=1 data-t=tQr><option value=2 data-t=tApps></select>
<label data-t=idleLbl></label><select id=idle><option value=0 data-t=never><option value=30>30 s<option value=60>1 min<option value=120>2 min<option value=300>5 min<option value=600>10 min</select>
<label class=chk><input type=checkbox id=qrOn><span data-t=qrOn></span></label>
<p class=hint data-t=restartNote></p>
<button id=saveStart data-t=save></button></section>

<section><h2 data-t=clockTitle></h2>
<div class=two><div><label data-t=timeFmt></label><select id=c24><option value=1 data-t=h24><option value=0 data-t=h12></select></div>
<div><label data-t=dateFmt></label><select id=datef><option value=0 data-t=dLong><option value=1 data-t=dNum><option value=2 data-t=dHide></select></div></div>
<label class=chk><input type=checkbox id=greet><span data-t=greetOn></span></label>
<label data-t=nameLbl></label><input id=name maxlength=20 autocomplete=off>
<button id=saveClock data-t=save></button></section>

<section><h2 data-t=wxTitle></h2>
<div class=row><span data-t=wxNow></span><span id=wxNow></span></div>
<label data-t=wxSearch></label><div class=two><input id=wxQ data-ph=wxPh><button class=sec id=wxFind data-t=search style="margin:0"></button></div>
<div id=wxRes></div>
<button class=sec id=wxAuto data-t=wxAuto></button>
<label data-t=unit></label><select id=tempF><option value=0>°C<option value=1>°F</select>
<button id=saveUnit data-t=save></button></section>

<section><h2 data-t=qrTitle></h2>
<p class=hint data-t=qrHint></p>
<label data-t=qrT></label><input id=qrT maxlength=30 data-ph=qrTPh>
<label data-t=qrB></label><input id=qrB maxlength=38>
<div class=two><div><label data-t=qrN></label><input id=qrN maxlength=38></div><div><label data-t=qrA></label><input id=qrA maxlength=22 inputmode=numeric></div></div>
<label data-t=qrP></label><textarea id=qrP maxlength=500 spellcheck=false></textarea>
<p class=hint data-t=restartNote></p>
<button id=saveQr data-t=save></button></section>

<section><h2 data-t=appsTitle></h2>
<p class=hint data-t=appsHint></p><div id=apps></div>
<p class=hint data-t=restartNote></p>
<button id=saveApps data-t=save></button></section>

<section><h2 data-t=dragonTitle></h2>
<div id=dgInfo></div>
<label data-t=dgName></label><input id=dgNameIn maxlength=20 autocomplete=off>
<button id=saveDg data-t=save></button>
<button class=sec id=dgReset data-t=dgReset></button></section>

<section><h2 data-t=accessTitle></h2>
<label data-t=hostLbl></label><input id=host autocapitalize=off maxlength=24>
<button id=saveHost data-t=save></button>
<label data-t=pwLbl></label><input id=wpw type=password autocomplete=new-password>
<button id=savePw data-t=save></button></section>
</main><div id=msg></div>
<script>
const $=id=>document.getElementById(id);
const D={vi:{status:'Trạng thái',lang:'Ngôn ngữ',restartNote:'Thiết bị sẽ tự khởi động lại để áp dụng.',save:'Lưu',startTitle:'Màn hình chính',startLbl:'Màn hình khi bật máy',tHome:'Trang chủ',tQr:'Mã QR thanh toán',tApps:'Ứng dụng',idleLbl:'Tự quay về màn hình chính sau',never:'Không bao giờ',qrOn:'Hiện trang QR thanh toán',
clockTitle:'Đồng hồ & chào hỏi',timeFmt:'Định dạng giờ',h24:'24 giờ',h12:'12 giờ (SA/CH)',dateFmt:'Hiển thị ngày',dLong:'Đầy đủ (Thứ Hai, 19 tháng 10)',dNum:'Số (19/10/2026)',dHide:'Ẩn',greetOn:'Hiện lời chào',nameLbl:'Tên trong lời chào (để trống nếu không cần)',
wxTitle:'Thời tiết',wxNow:'Vị trí hiện tại',wxSearch:'Tìm thành phố',wxPh:'Ví dụ: Đà Nẵng',search:'Tìm',wxAuto:'Tự xác định vị trí (theo IP)',unit:'Đơn vị nhiệt độ',auto:'tự động',noRes:'Không tìm thấy',
qrTitle:'QR thanh toán',qrHint:'Dán chuỗi VietQR/EMV (bắt đầu bằng 000201...) lấy từ ứng dụng ngân hàng.',qrT:'Tiêu đề',qrTPh:'Quét để thanh toán',qrB:'Dòng ngân hàng',qrN:'Tên tài khoản',qrA:'Số tài khoản',qrP:'Nội dung mã QR',
appsTitle:'Ứng dụng',appsHint:'Chọn ứng dụng hiển thị và dùng mũi tên để đổi thứ tự. Cài đặt luôn hiển thị.',
accessTitle:'Truy cập',hostLbl:'Tên thiết bị (mở bằng tên.local)',pwLbl:'Mật khẩu trang web (để trống = không cần)',
wifi:'Wi-Fi',ip:'Địa chỉ IP',fw:'Phần mềm',sd:'Thẻ nhớ',heap:'RAM trống',up:'Thời gian chạy',sdOk:'Có thẻ',sdNo:'Không có thẻ',
dragonTitle:'Nuôi rồng ảo',dgName:'Tên rồng',dgReset:'Nuôi lại từ đầu (xóa dữ liệu rồng)',dgConfirm:'Xóa toàn bộ dữ liệu rồng và bắt đầu lại từ quả trứng?',dgNoSd:'Chưa có thẻ nhớ',dgStage:'Cấp độ',dgGrow:'Tiến độ',dgCoins:'Số xu',dgGen:'Thế hệ',dgBest:'Kỷ lục mini game',dgSick:'Đang ốm',saved:'Đã lưu',restarting:'Đã lưu. Thiết bị đang khởi động lại...',failed:'Lỗi: ',pwSet:'Đã đặt mật khẩu',pwOff:'Đã xóa mật khẩu',wxSaved:'Đã đặt vị trí thời tiết',needName:'Nhập tên thiết bị'},
en:{status:'Status',lang:'Language',restartNote:'The device restarts to apply this.',save:'Save',startTitle:'Main screen',startLbl:'Screen shown at power-on',tHome:'Home',tQr:'Payment QR',tApps:'Apps',idleLbl:'Return to the main screen after',never:'Never',qrOn:'Show the payment QR page',
clockTitle:'Clock & greeting',timeFmt:'Time format',h24:'24-hour',h12:'12-hour (AM/PM)',dateFmt:'Date display',dLong:'Long (Monday, 19 October)',dNum:'Numeric (19/10/2026)',dHide:'Hidden',greetOn:'Show greeting',nameLbl:'Name in the greeting (optional)',
wxTitle:'Weather',wxNow:'Current location',wxSearch:'Search city',wxPh:'e.g. Da Nang',search:'Search',wxAuto:'Detect location automatically (by IP)',unit:'Temperature unit',auto:'automatic',noRes:'No results',
qrTitle:'Payment QR',qrHint:'Paste the VietQR/EMV string (starts with 000201...) from your banking app.',qrT:'Title',qrTPh:'Scan to pay',qrB:'Bank line',qrN:'Account name',qrA:'Account number',qrP:'QR content',
appsTitle:'Apps',appsHint:'Tick the apps to show and use the arrows to reorder. Settings is always shown.',
accessTitle:'Access',hostLbl:'Device name (opens as name.local)',pwLbl:'Web page password (empty = none)',
wifi:'Wi-Fi',ip:'IP address',fw:'Firmware',sd:'microSD',heap:'Free RAM',up:'Uptime',sdOk:'Detected',sdNo:'No card',
dragonTitle:'Virtual dragon',dgName:'Dragon name',dgReset:'Start over (erase dragon data)',dgConfirm:'Erase all dragon data and start again from an egg?',dgNoSd:'No microSD card',dgStage:'Stage',dgGrow:'Progress',dgCoins:'Coins',dgGen:'Generation',dgBest:'Mini-game best',dgSick:'Sick',saved:'Saved',restarting:'Saved. The device is restarting...',failed:'Failed: ',pwSet:'Password set',pwOff:'Password removed',wxSaved:'Weather location set',needName:'Enter a device name'}};
let T=D.vi,S=null,order=[];
function toast(t,err){const m=$('msg');m.textContent=t;m.className=err?'err':'';m.style.display='block';clearTimeout(toast.t);toast.t=setTimeout(()=>m.style.display='none',3500)}
function applyLang(l){T=D[l==1?'en':'vi'];document.documentElement.lang=l==1?'en':'vi';
 document.querySelectorAll('[data-t]').forEach(e=>e.textContent=T[e.dataset.t]);
 document.querySelectorAll('[data-ph]').forEach(e=>e.placeholder=T[e.dataset.ph])}
async function post(path,body){try{const r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(body)});
 if(!r.ok)throw new Error(await r.text()||r.status);return await r.json().catch(()=>({}))}catch(e){toast(T.failed+e.message,1);return null}}
async function saveCfg(body){const r=await post('/api/cfg',body);if(!r)return;
 if(r.restart){toast(T.restarting);setTimeout(()=>location.reload(),12000)}else{toast(T.saved);load()}}
function row(k,v){return '<div class=row><span>'+k+'</span><span>'+v+'</span></div>'}
function drawApps(){$('apps').innerHTML=order.map((a,i)=>'<div class=app><input type=checkbox '+(a.on?'checked':'')+(a.lock?' disabled':'')+' data-i='+i+'><span>'+a.name+'</span>'+
 '<button class=sec data-up='+i+'>↑</button><button class=sec data-dn='+i+'>↓</button></div>').join('');
 $('apps').querySelectorAll('input').forEach(c=>c.onchange=()=>{order[c.dataset.i].on=c.checked});
 $('apps').querySelectorAll('[data-up]').forEach(b=>b.onclick=()=>mv(+b.dataset.up,-1));
 $('apps').querySelectorAll('[data-dn]').forEach(b=>b.onclick=()=>mv(+b.dataset.dn,1))}
function mv(i,d){const j=i+d;if(j<0||j>=order.length)return;[order[i],order[j]]=[order[j],order[i]];drawApps()}
async function load(){S=await (await fetch('/api/state')).json();applyLang(S.lang);
 $('addr').textContent='http://'+S.host+'.local  ·  '+S.ip;
 $('info').innerHTML=row(T.wifi,S.ssid+' ('+S.rssi+' dBm)')+row(T.ip,S.ip)+row('Xiaozhi',S.xiaozhi)+row(T.fw,S.fw)+row(T.sd,S.sd?T.sdOk:T.sdNo)+row(T.heap,Math.round(S.heap/1024)+' KB')+row(T.up,S.up);
 $('lang').value=S.lang;$('start').value=S.start;$('idle').value=S.idle;$('qrOn').checked=S.qrOn;
 $('c24').value=S.c24?1:0;$('datef').value=S.datef;$('greet').checked=S.greet;$('name').value=S.name;
 $('wxNow').textContent=(S.wx.city||'—')+' ('+(S.wx.manual?'':T.auto+', ')+S.wx.lat.toFixed(2)+', '+S.wx.lon.toFixed(2)+')';$('tempF').value=S.tempF?1:0;
 $('qrT').value=S.qrT;$('qrB').value=S.qrB;$('qrN').value=S.qrN;$('qrA').value=S.qrA;$('qrP').value=S.qrP;
 $('host').value=S.host;
 const g=S.dragon;$('dgInfo').innerHTML=g.ok?row(T.dgStage,g.stageName+(g.sick?' ('+T.dgSick+')':''))+row(T.dgGrow,g.pct+'%')+row(T.dgCoins,g.coins)+row(T.dgGen,g.gen)+row(T.dgBest,g.best):row(T.dgStage,T.dgNoSd);
 if(document.activeElement!==$('dgNameIn'))$('dgNameIn').value=g.ok?g.name:'';
 order=S.apps.map(a=>({id:a.id,name:a.name,on:a.on,lock:a.lock}));drawApps()}
$('saveLang').onclick=()=>saveCfg({lang:$('lang').value});
$('saveStart').onclick=()=>saveCfg({start:$('start').value,idle:$('idle').value,qrOn:$('qrOn').checked?1:0});
$('saveClock').onclick=()=>saveCfg({c24:$('c24').value,datef:$('datef').value,greet:$('greet').checked?1:0,name:$('name').value});
$('saveUnit').onclick=()=>saveCfg({tempF:$('tempF').value});
$('saveQr').onclick=()=>saveCfg({qrT:$('qrT').value,qrB:$('qrB').value,qrN:$('qrN').value,qrA:$('qrA').value,qrP:$('qrP').value.trim()});
$('saveApps').onclick=()=>saveCfg({apps:order.filter(a=>a.on||a.lock).map(a=>a.id).join(',')});
$('wxFind').onclick=async()=>{const q=$('wxQ').value.trim();if(!q)return;
 try{const r=await (await fetch('https://geocoding-api.open-meteo.com/v1/search?count=6&name='+encodeURIComponent(q)+'&language='+(S.lang==1?'en':'vi'))).json();
  const list=r.results||[];$('wxRes').innerHTML=list.length?'':'<p class=hint>'+T.noRes+'</p>';
  list.forEach(c=>{const d=document.createElement('div');d.className='res';d.innerHTML=c.name+'<small>'+[c.admin1,c.country].filter(Boolean).join(', ')+'</small>';
   d.onclick=async()=>{if(await post('/api/weather',{lat:c.latitude,lon:c.longitude,city:c.name})){toast(T.wxSaved);$('wxRes').innerHTML='';setTimeout(load,800)}};$('wxRes').appendChild(d)})}
 catch(e){toast(T.failed+e.message,1)}};
$('wxQ').onkeydown=e=>{if(e.key==='Enter')$('wxFind').click()};
$('wxAuto').onclick=async()=>{if(await post('/api/weather',{auto:1})){toast(T.saved);setTimeout(load,1500)}};
$('saveHost').onclick=async()=>{const h=$('host').value.trim().toLowerCase();if(!h)return toast(T.needName,1);
 if(await post('/api/host',{h})){toast(T.restarting);setTimeout(()=>location.href='http://'+h+'.local/',12000)}};
$('savePw').onclick=async()=>{if(await post('/api/password',{pw:$('wpw').value})){toast($('wpw').value?T.pwSet:T.pwOff);$('wpw').value=''}};
$('saveDg').onclick=async()=>{if(await post('/api/dragon',{name:$('dgNameIn').value})){toast(T.saved);load()}};
$('dgReset').onclick=async()=>{if(confirm(T.dgConfirm)&&await post('/api/dragon',{reset:1})){toast(T.saved);load()}};
load();
</script></body></html>)HTML";

static bool webAuthOk() {
  if (webPassword.length() == 0) return true;
  if (web.authenticate("admin", webPassword.c_str())) return true;
  web.requestAuthentication(BASIC_AUTH, "Zin100");
  return false;
}

static String webUptime() {
  uint32_t s = millis() / 1000;
  char b[32];
  snprintf(b, sizeof(b), "%ud %02u:%02u", (unsigned)(s / 86400), (unsigned)(s / 3600 % 24), (unsigned)(s / 60 % 60));
  return b;
}

static String webSanitizeHost(String h) {
  h.toLowerCase();
  String o;
  for (char c : h)
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') o += c;
  while (o.startsWith("-")) o.remove(0, 1);
  if (o.length() > 24) o = o.substring(0, 24);
  return o;
}

static const char *webHost() { return webHostName.c_str(); }

static void webStartMdns() {
  MDNS.end();
  if (MDNS.begin(webHostName.c_str())) MDNS.addService("http", "tcp", 80);
}

static void webSendState() {
  JsonDocument d;
  d["host"] = webHostName;
  d["ip"] = WiFi.localIP().toString();
  d["ssid"] = WiFi.SSID();
  d["rssi"] = WiFi.RSSI();
  d["xiaozhi"] = xiaozhiState() == XZ_ACTIVATED ? String(tr("Connected")) : String(tr(xiaozhiMessage()));
  d["fw"] = FW_VERSION;
  d["sd"] = sdOk;
  d["heap"] = ESP.getFreeHeap();
  d["up"] = webUptime();
  d["lang"] = gLang;
  d["start"] = cfg.startTile;
  d["idle"] = cfg.idleReturn;
  d["qrOn"] = cfg.qrOn;
  d["c24"] = cfg.clock24;
  d["datef"] = cfg.dateFmt;
  d["greet"] = cfg.greetOn;
  d["name"] = cfg.userName;
  d["tempF"] = cfg.tempF;
  d["qrT"] = cfg.qrTitle;
  d["qrB"] = cfg.qrBank;
  d["qrN"] = cfg.qrName;
  d["qrA"] = cfg.qrAcct;
  d["qrP"] = cfg.qrPayload;
  JsonObject dr = d["dragon"].to<JsonObject>();
  dr["ok"] = dgOk;
  if (dgOk) {
    dr["name"] = dg.name.length() ? dg.name : String("");
    dr["stageName"] = dgStageName(dg.stage);
    dr["pct"] = dg.stage == ST_DIVINE ? 100 : (int)(dg.growth * 100 / dgNeed());
    dr["coins"] = dg.coins;
    dr["gen"] = dg.gen;
    dr["best"] = dg.best;
    dr["sick"] = dg.sick;
  }
  float la, lo;
  char city[40];
  weatherGetLocation(la, lo, city, sizeof(city));
  d["wx"]["lat"] = la;
  d["wx"]["lon"] = lo;
  d["wx"]["city"] = city;
  d["wx"]["manual"] = weatherIsManual();
  // visible apps first (in launcher order), then the hidden ones
  JsonArray apps = d["apps"].to<JsonArray>();
  bool seen[APP_COUNT] = {};
  int from = 0;
  while (from <= (int)cfg.apps.length()) {
    int to = cfg.apps.indexOf(',', from);
    if (to < 0) to = cfg.apps.length();
    int idx = cfg.apps.substring(from, to).toInt();
    from = to + 1;
    if (idx < 0 || idx >= APP_COUNT || seen[idx]) continue;
    seen[idx] = true;
    JsonObject o = apps.add<JsonObject>();
    o["id"] = idx;
    o["name"] = tr(appName(idx));
    o["on"] = true;
    o["lock"] = idx == SETTINGS_APP;
  }
  for (int i = 0; i < APP_COUNT; i++) {
    if (seen[i]) continue;
    JsonObject o = apps.add<JsonObject>();
    o["id"] = i;
    o["name"] = tr(appName(i));
    o["on"] = false;
    o["lock"] = false;
  }
  String out;
  serializeJson(d, out);
  web.send(200, "application/json", out);
}

// Applies the submitted options; returns true when a restart is needed for them to show.
static bool webApplyCfg() {
  bool restart = false;
  auto has = [](const char *k) { return web.hasArg(k); };
  if (has("lang")) {
    uint8_t l = web.arg("lang").toInt() == LANG_EN ? LANG_EN : LANG_VI;
    if (l != gLang) {
      i18nSave(l);
      restart = true;
    }
  }
  if (has("start")) {
    uint8_t v = constrain(web.arg("start").toInt(), 0, 2);
    if (v != cfg.startTile) restart = true;
    cfg.startTile = v;
  }
  if (has("idle")) cfg.idleReturn = constrain(web.arg("idle").toInt(), 0, 3600);
  if (has("qrOn")) {
    bool v = web.arg("qrOn").toInt() != 0;
    if (v != cfg.qrOn) restart = true;
    cfg.qrOn = v;
    if (!cfg.qrOn && cfg.startTile == 1) cfg.startTile = 0;
  }
  if (has("c24")) cfg.clock24 = web.arg("c24").toInt() != 0;
  if (has("datef")) cfg.dateFmt = constrain(web.arg("datef").toInt(), 0, 2);
  if (has("greet")) cfg.greetOn = web.arg("greet").toInt() != 0;
  if (has("name")) {
    cfg.userName = web.arg("name");
    cfg.userName.trim();
    if (cfg.userName.length() > 20) cfg.userName = cfg.userName.substring(0, 20);
  }
  if (has("tempF")) cfg.tempF = web.arg("tempF").toInt() != 0;
  auto text = [&](const char *key, String &dst, size_t maxLen) {
    if (!has(key)) return;
    String v = web.arg(key);
    v.trim();
    if (v.length() > maxLen) v = v.substring(0, maxLen);
    if (v != dst) restart = true;
    dst = v;
  };
  text("qrT", cfg.qrTitle, 30);
  text("qrB", cfg.qrBank, 38);
  text("qrN", cfg.qrName, 38);
  text("qrA", cfg.qrAcct, 22);
  text("qrP", cfg.qrPayload, 500);
  if (has("apps")) {
    String v = cfgCleanApps(web.arg("apps"));
    if (v != cfg.apps) restart = true;
    cfg.apps = v;
  }
  cfgSave();
  return restart;
}

static void webBegin() {
  Preferences p;
  p.begin("ui", true);
  webHostName = webSanitizeHost(p.getString("host", "zin100"));
  webPassword = p.getString("webpw", "");
  p.end();
  if (webHostName.length() == 0) webHostName = "zin100";

  web.on("/", HTTP_GET, []() {
    if (!webAuthOk()) return;
    web.send_P(200, "text/html", WEB_PAGE);
  });
  web.on("/api/state", HTTP_GET, []() {
    if (!webAuthOk()) return;
    webSendState();
  });
  web.on("/api/cfg", HTTP_POST, []() {
    if (!webAuthOk()) return;
    bool restart = webApplyCfg();
    web.send(200, "application/json", restart ? "{\"restart\":true}" : "{\"restart\":false}");
    if (restart) webRestartAt = millis() + 800;
  });
  web.on("/api/weather", HTTP_POST, []() {
    if (!webAuthOk()) return;
    if (web.hasArg("auto")) {
      weatherAutoLocate();
    } else {
      float la = web.arg("lat").toFloat(), lo = web.arg("lon").toFloat();
      String c = web.arg("city");
      if (c.length() == 0 || la < -90 || la > 90 || lo < -180 || lo > 180) return web.send(400, "text/plain", "bad location");
      weatherSetLocation(la, lo, c.c_str());
    }
    web.send(200, "application/json", "{}");
  });
  web.on("/api/dragon", HTTP_POST, []() {
    if (!webAuthOk()) return;
    if (!dgOk) return web.send(409, "text/plain", "no microSD card");
    if (web.hasArg("reset")) dgNewEgg(false);
    if (web.hasArg("name")) dgSetName(web.arg("name"));
    if (web.hasArg("speed")) dgSpeed = constrain(web.arg("speed").toFloat(), 1.0f, 2000.0f);  // testing only
    dSig = -1;
    web.send(200, "application/json", "{}");
  });
  web.on("/api/host", HTTP_POST, []() {
    if (!webAuthOk()) return;
    String h = webSanitizeHost(web.arg("h"));
    if (h.length() == 0) return web.send(400, "text/plain", "use letters, digits and -");
    Preferences p;
    p.begin("ui", false);
    p.putString("host", h);
    p.end();
    web.send(200, "application/json", "{}");
    webRestartAt = millis() + 800;
  });
  web.on("/api/password", HTTP_POST, []() {
    if (!webAuthOk()) return;
    webPassword = web.arg("pw");
    Preferences p;
    p.begin("ui", false);
    p.putString("webpw", webPassword);
    p.end();
    web.send(200, "application/json", "{}");
  });
  web.onNotFound([]() { web.send(404, "text/plain", "not found"); });
}

// Call from loop(): serves the page while joined to a network and keeps the .local name registered.
static void webTick() {
  if (webRestartAt && (int32_t)(millis() - webRestartAt) >= 0) ESP.restart();
  bool up = wifiConnected() && !wifiPortalActive();
  if (up && !webStarted) {
    webBegin();
    web.begin();
    webStarted = true;
  }
  if (!webStarted) return;
  if (up && !webWasConnected) webStartMdns();  // (re)announce after every reconnect
  webWasConnected = up;
  if (up) web.handleClient();
}
