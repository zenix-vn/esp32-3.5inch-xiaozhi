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
.f{display:flex;align-items:center;gap:8px;padding:7px 0;border-top:1px solid var(--line);font-size:15px}.f:first-child{border-top:0}
.f .n{flex:1;word-break:break-all;cursor:default}.f .n.dir{color:var(--acc);cursor:pointer}.f small{color:var(--mut)}
.f a,.f button{width:auto;margin:0;padding:6px 10px;font-size:13px;border-radius:8px;text-decoration:none}
.f a{background:#1f2849;color:var(--txt);border:1px solid var(--line)}
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

<section><h2 data-t=lookTitle></h2>
<label data-t=themeLbl></label><select id=theme><option value=0 data-t=thSky><option value=1 data-t=thForest><option value=2 data-t=thSunset><option value=3 data-t=thMono></select>
<label class=chk><input type=checkbox id=fx><span data-t=fxOn></span></label>
<label class=chk><input type=checkbox id=lunar><span data-t=lunarOn></span></label>
<label class=chk><input type=checkbox id=quote><span data-t=quoteOn></span></label>
<label data-t=phLbl></label><select id=phS><option value=5>5 s<option value=10>10 s<option value=20>20 s<option value=30>30 s<option value=60>60 s</select>
<button id=saveLook data-t=save></button></section>

<section><h2 data-t=ambTitle></h2>
<label data-t=idleMLbl></label><select id=idleM><option value=1 data-t=idleAmb><option value=0 data-t=idleOff></select>
<p class=hint data-t=idleHint></p>
<label data-t=ambSched></label>
<div class=two><select id=ambS></select><select id=ambE></select></div>
<label data-t=volLbl></label><input type=range id=vol min=0 max=100>
<button id=saveAmb data-t=save></button></section>

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

<section><h2 data-t=wpTitle></h2>
<p class=hint data-t=wpHint></p>
<canvas id=wpPrev width=480 height=320 style="width:100%;border-radius:10px;background:#000"></canvas>
<p class=hint id=wpState></p>
<input type=file id=wpFile accept="image/*">
<label data-t=wpDim></label><input type=range id=wpDim min=0 max=70 value=35>
<div class=two><button id=wpSet data-t=wpSet></button><button class=sec id=wpOff data-t=wpOff></button></div></section>

<section><h2 data-t=sdTitle></h2>
<div class=row><span id=sdPath>/</span><span id=sdFree></span></div>
<div id=sdList></div>
<label class=chk><input type=checkbox id=sdShrink checked><span data-t=sdShrink></span></label>
<input type=file id=sdFiles multiple>
<div class=two><button id=sdUp data-t=sdUpload></button><button class=sec id=sdMk data-t=sdMkdir></button></div>
<p class=hint id=sdProg></p></section>

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
lookTitle:'Giao diện',phLbl:'Khung ảnh: thời gian mỗi ảnh',themeLbl:'Chủ đề màu',thSky:'Bầu trời',thForest:'Rừng xanh',thSunset:'Hoàng hôn',thMono:'Đơn sắc',fxOn:'Hiệu ứng thời tiết trên trang chủ (mưa, sao, nắng...)',lunarOn:'Hiện lịch âm',quoteOn:'Hiện câu nói mỗi ngày',ambTitle:'Màn hình chờ & âm thanh',idleMLbl:'Khi không dùng một lúc thì',idleAmb:'Hiện màn hình chờ (đồng hồ lớn, tối dần)',idleOff:'Tắt màn hình',idleHint:'Thời gian chờ chỉnh trong Cài đặt trên thiết bị.',ambSched:'Tự vào màn hình chờ ban đêm (từ - đến)',schedOff:'Tắt',volLbl:'Âm lượng loa',wpTitle:'Hình nền',wpHint:'Chọn ảnh từ máy tính hoặc bấm "Nền" ở một ảnh trong thẻ nhớ. Ảnh được cắt và nén về 480x320 rồi lưu vào bộ nhớ trong của thiết bị.',wpDim:'Làm tối ảnh để chữ dễ đọc',wpSet:'Đặt làm hình nền',wpOff:'Bỏ hình nền',wpNone:'Chưa có hình nền',wpHas:'Đang dùng hình nền',wpPick:'Hãy chọn ảnh trước',wpSending:'Đang gửi hình nền...',sdTitle:'Thẻ nhớ',sdShrink:'Thu nhỏ ảnh về 480x320 trước khi chép (khuyên dùng cho khung ảnh)',sdUpload:'Chép file lên thẻ',sdMkdir:'Thư mục mới',sdNoCard:'Không có thẻ nhớ',sdEmpty:'Thư mục trống',sdDel:'Xóa',sdDelQ:'Xóa ',sdBg:'Nền',sdFree:' trống',sdNewName:'Tên thư mục mới',sdPickFiles:'Hãy chọn file trước',sdUploading:'Đang chép ',sdDone:'Đã chép xong',dragonTitle:'Luyện rồng',dgName:'Tên rồng',dgReset:'Nuôi lại từ đầu (xóa dữ liệu rồng)',dgConfirm:'Xóa toàn bộ dữ liệu rồng và bắt đầu lại từ quả trứng?',dgNoSd:'Chưa có thẻ nhớ',dgStage:'Cấp độ',dgGrow:'Tiến độ',dgCoins:'Số xu',dgGen:'Thế hệ',dgBest:'Kỷ lục mini game',dgSick:'Đang ốm',dgAch:'Huy hiệu',dgStreak:'Chuỗi ngày',dgBest2:'Kỷ lục Rồng bay',dgJournal:'Nhật ký rồng',dgDays:' ngày',saved:'Đã lưu',restarting:'Đã lưu. Thiết bị đang khởi động lại...',failed:'Lỗi: ',pwSet:'Đã đặt mật khẩu',pwOff:'Đã xóa mật khẩu',wxSaved:'Đã đặt vị trí thời tiết',needName:'Nhập tên thiết bị'},
en:{status:'Status',lang:'Language',restartNote:'The device restarts to apply this.',save:'Save',startTitle:'Main screen',startLbl:'Screen shown at power-on',tHome:'Home',tQr:'Payment QR',tApps:'Apps',idleLbl:'Return to the main screen after',never:'Never',qrOn:'Show the payment QR page',
clockTitle:'Clock & greeting',timeFmt:'Time format',h24:'24-hour',h12:'12-hour (AM/PM)',dateFmt:'Date display',dLong:'Long (Monday, 19 October)',dNum:'Numeric (19/10/2026)',dHide:'Hidden',greetOn:'Show greeting',nameLbl:'Name in the greeting (optional)',
wxTitle:'Weather',wxNow:'Current location',wxSearch:'Search city',wxPh:'e.g. Da Nang',search:'Search',wxAuto:'Detect location automatically (by IP)',unit:'Temperature unit',auto:'automatic',noRes:'No results',
qrTitle:'Payment QR',qrHint:'Paste the VietQR/EMV string (starts with 000201...) from your banking app.',qrT:'Title',qrTPh:'Scan to pay',qrB:'Bank line',qrN:'Account name',qrA:'Account number',qrP:'QR content',
appsTitle:'Apps',appsHint:'Tick the apps to show and use the arrows to reorder. Settings is always shown.',
accessTitle:'Access',hostLbl:'Device name (opens as name.local)',pwLbl:'Web page password (empty = none)',
wifi:'Wi-Fi',ip:'IP address',fw:'Firmware',sd:'microSD',heap:'Free RAM',up:'Uptime',sdOk:'Detected',sdNo:'No card',
lookTitle:'Look',phLbl:'Photo frame: time per picture',themeLbl:'Colour theme',thSky:'Sky',thForest:'Forest',thSunset:'Sunset',thMono:'Mono',fxOn:'Weather effects on the home screen (rain, stars, sun...)',lunarOn:'Show the lunar date',quoteOn:'Show the quote of the day',ambTitle:'Ambient screen & sound',idleMLbl:'After a while without use',idleAmb:'Show the ambient clock (dims)',idleOff:'Turn the screen off',idleHint:'The idle time is set in Settings on the device.',ambSched:'Ambient clock at night (from - to)',schedOff:'Off',volLbl:'Speaker volume',wpTitle:'Wallpaper',wpHint:'Pick a picture from this computer or press "Bg" on a picture on the card. It is cropped and compressed to 480x320 and stored inside the device.',wpDim:'Darken the picture so text stays readable',wpSet:'Set as wallpaper',wpOff:'Remove wallpaper',wpNone:'No wallpaper',wpHas:'Wallpaper in use',wpPick:'Pick a picture first',wpSending:'Sending the wallpaper...',sdTitle:'Memory card',sdShrink:'Shrink pictures to 480x320 before copying (recommended for the photo frame)',sdUpload:'Copy files to the card',sdMkdir:'New folder',sdNoCard:'No memory card',sdEmpty:'Empty folder',sdDel:'Delete',sdDelQ:'Delete ',sdBg:'Bg',sdFree:' free',sdNewName:'New folder name',sdPickFiles:'Pick files first',sdUploading:'Copying ',sdDone:'All files copied',dragonTitle:'Dragon training',dgName:'Dragon name',dgReset:'Start over (erase dragon data)',dgConfirm:'Erase all dragon data and start again from an egg?',dgNoSd:'No microSD card',dgStage:'Stage',dgGrow:'Progress',dgCoins:'Coins',dgGen:'Generation',dgBest:'Mini-game best',dgSick:'Sick',dgAch:'Badges',dgStreak:'Day streak',dgBest2:'Dragon flight best',dgJournal:'Dragon journal',dgDays:' days',saved:'Saved',restarting:'Saved. The device is restarting...',failed:'Failed: ',pwSet:'Password set',pwOff:'Password removed',wxSaved:'Weather location set',needName:'Enter a device name'}};
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
 $('wpState').textContent=S.wall?T.wpHas:T.wpNone;
 $('theme').value=S.theme;$('phS').value=S.phS;$('fx').checked=S.fx;$('lunar').checked=S.lunar;$('quote').checked=S.quote;$('idleM').value=S.idleM;$('vol').value=S.vol;
 fillHours('ambS',S.ambS);fillHours('ambE',S.ambE);
 const g=S.dragon;$('dgInfo').innerHTML=g.ok?row(T.dgStage,g.stageName+(g.sick?' ('+T.dgSick+')':''))+row(T.dgGrow,g.pct+'%')+row(T.dgCoins,g.coins)+row(T.dgGen,g.gen)+row(T.dgBest,g.best)+row(T.dgBest2,g.best2)+row(T.dgAch,g.ach+'/12')+row(T.dgStreak,g.streak+T.dgDays)+(g.journal&&g.journal.length?'<h2 style="margin-top:12px">'+T.dgJournal+'</h2>'+g.journal.map(e=>row(e.ts?new Date(e.ts*1000).toLocaleString():'-',e.text)).join(''):''):row(T.dgStage,T.dgNoSd);
 if(document.activeElement!==$('dgNameIn'))$('dgNameIn').value=g.ok?g.name:'';
 order=S.apps.map(a=>({id:a.id,name:a.name,on:a.on,lock:a.lock}));drawApps()}
function fillHours(id,v){const s=$(id);s.innerHTML=(id=='ambS'?'<option value=-1>'+T.schedOff:'')+Array.from({length:24},(_,h)=>'<option value='+h+'>'+h+':00').join('');s.value=v}
$('saveLook').onclick=()=>saveCfg({phS:$('phS').value,theme:$('theme').value,fx:$('fx').checked?1:0,lunar:$('lunar').checked?1:0,quote:$('quote').checked?1:0});
$('saveAmb').onclick=()=>saveCfg({idleM:$('idleM').value,ambS:$('ambS').value,ambE:$('ambE').value,vol:$('vol').value});
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

// ---------- wallpaper: crop to 480x320, darken, RGB565 ----------
let wpImg=null;
function wpDraw(){const c=$('wpPrev'),x=c.getContext('2d');x.fillStyle='#000';x.fillRect(0,0,480,320);if(!wpImg)return;
 const s=Math.max(480/wpImg.width,320/wpImg.height),w=wpImg.width*s,h=wpImg.height*s;x.drawImage(wpImg,(480-w)/2,(320-h)/2,w,h);
 x.fillStyle='rgba(0,0,0,'+($('wpDim').value/100)+')';x.fillRect(0,0,480,320)}
function wpLoad(src){const i=new Image();i.onload=()=>{wpImg=i;wpDraw();$('wpPrev').scrollIntoView({behavior:'smooth',block:'center'})};i.src=src}
$('wpFile').onchange=()=>{const f=$('wpFile').files[0];if(f)wpLoad(URL.createObjectURL(f))};
$('wpDim').oninput=wpDraw;
$('wpSet').onclick=()=>{if(!wpImg)return toast(T.wpPick,1);wpDraw();
 const d=$('wpPrev').getContext('2d').getImageData(0,0,480,320).data,buf=new Uint8Array(16+480*320*2);
 buf.set([90,87,80,49,224,1,64,1]);  // "ZWP1", 480, 320
 for(let i=0,o=16;i<d.length;i+=4,o+=2){const v=((d[i]>>3)<<11)|((d[i+1]>>2)<<5)|(d[i+2]>>3);buf[o]=v&255;buf[o+1]=v>>8}
 const fd=new FormData();fd.append('f',new Blob([buf]),'wall.bin');toast(T.wpSending);
 send('/api/wallpaper',fd,p=>$('wpState').textContent=T.wpSending+' '+p+'%').then(ok=>{if(ok){toast(T.saved);load()}})};
$('wpOff').onclick=async()=>{if(await post('/api/wallpaper/off',{})){toast(T.saved);load()}};

// ---------- memory card ----------
let sdDir='/photos';
function send(url,fd,prog){return new Promise(res=>{const r=new XMLHttpRequest();r.open('POST',url);
 r.upload.onprogress=e=>{if(e.lengthComputable&&prog)prog(Math.round(e.loaded*100/e.total))};
 r.onload=()=>{if(r.status==200)res(true);else{toast(T.failed+(r.responseText||r.status),1);res(false)}};
 r.onerror=()=>{toast(T.failed+'network',1);res(false)};r.send(fd)})}
function human(n){return n>1048576?(n/1048576).toFixed(1)+' MB':n>1024?Math.round(n/1024)+' KB':n+' B'}
function join(a,b){return (a.endsWith('/')?a:a+'/')+b}
async function sdLoad(dir){const r=await fetch('/api/sd/list?dir='+encodeURIComponent(dir));
 if(r.status==404&&dir!='/')return sdLoad('/');
 if(!r.ok){$('sdList').innerHTML='<p class=hint>'+T.sdNoCard+'</p>';return}
 const j=await r.json();sdDir=j.dir;$('sdPath').textContent=sdDir;$('sdFree').textContent=(j.total-j.used)+' MB'+T.sdFree;
 const items=j.items.sort((a,b)=>(b.d-a.d)||a.n.localeCompare(b.n));let h='';
 if(sdDir!='/')h+='<div class=f><span class="n dir" data-up=1>.. /</span></div>';
 for(const it of items){const p=join(sdDir,it.n),e=encodeURIComponent(p);
  if(it.d)h+='<div class=f><span class="n dir" data-dir="'+p+'">'+it.n+' /</span><button class=sec data-del="'+p+'">'+T.sdDel+'</button></div>';
  else h+='<div class=f><span class=n>'+it.n+' <small>'+human(it.s)+'</small></span>'+(/\.(jpe?g|png|bmp|gif)$/i.test(it.n)?'<button class=sec data-bg="'+p+'">'+T.sdBg+'</button>':'')+
   '<a href="/api/sd/get?path='+e+'" download="'+it.n+'">&#8681;</a><button class=sec data-del="'+p+'">'+T.sdDel+'</button></div>'}
 $('sdList').innerHTML=h||'<p class=hint>'+T.sdEmpty+'</p>';
 $('sdList').querySelectorAll('[data-dir]').forEach(b=>b.onclick=()=>sdLoad(b.dataset.dir));
 $('sdList').querySelectorAll('[data-up]').forEach(b=>b.onclick=()=>sdLoad(sdDir.replace(/\/[^\/]+\/?$/,'')||'/'));
 $('sdList').querySelectorAll('[data-bg]').forEach(b=>b.onclick=()=>wpLoad('/api/sd/get?path='+encodeURIComponent(b.dataset.bg)));
 $('sdList').querySelectorAll('[data-del]').forEach(b=>b.onclick=async()=>{if(confirm(T.sdDelQ+b.dataset.del+'?')&&await post('/api/sd/delete',{path:b.dataset.del}))sdLoad(sdDir)})}
function shrink(file){return new Promise(res=>{const i=new Image();i.onload=()=>{const s=Math.min(1,480/i.width,320/i.height),c=document.createElement('canvas');
 c.width=Math.round(i.width*s);c.height=Math.round(i.height*s);c.getContext('2d').drawImage(i,0,0,c.width,c.height);
 c.toBlob(b=>res(b?new File([b],file.name.replace(/\.[^.]+$/,'')+'.jpg'):file),'image/jpeg',0.85)};i.onerror=()=>res(file);i.src=URL.createObjectURL(file)})}
$('sdUp').onclick=async()=>{const fs=[...$('sdFiles').files];if(!fs.length)return toast(T.sdPickFiles,1);
 for(let k=0;k<fs.length;k++){let f=fs[k];if($('sdShrink').checked&&/^image\//.test(f.type))f=await shrink(f);
  const fd=new FormData();fd.append('f',f,f.name);
  const ok=await send('/api/sd/upload?dir='+encodeURIComponent(sdDir),fd,p=>$('sdProg').textContent=T.sdUploading+(k+1)+'/'+fs.length+': '+f.name+' '+p+'%');
  if(!ok)break}
 $('sdProg').textContent=T.sdDone;$('sdFiles').value='';sdLoad(sdDir)};
$('sdMk').onclick=async()=>{const n=prompt(T.sdNewName);if(n&&await post('/api/sd/mkdir',{path:join(sdDir,n)}))sdLoad(join(sdDir,n))};
load();sdLoad(sdDir);
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

static File upFile;
static bool upError = false;

// Card paths from the web page: absolute, no "..".
static bool sdPathOk(const String &p) { return p.startsWith("/") && p.indexOf("..") < 0 && p.length() < 200; }
// FAT-friendly file name: keep letters, digits and . _ -; everything else (spaces, accents, symbols) becomes _
static String sdSafeName(String n) {
  int sl = n.lastIndexOf('/');
  if (sl >= 0) n = n.substring(sl + 1);
  sl = n.lastIndexOf('\\');
  if (sl >= 0) n = n.substring(sl + 1);
  String o;
  for (size_t i = 0; i < n.length() && o.length() < 60; i++) {
    char c = n[i];
    o += (isalnum((unsigned char)c) || c == '.' || c == '-' || c == '_') && (unsigned char)c < 128 ? c : '_';
  }
  return o.length() ? o : String("file");
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

static bool mdnsOk = false;
static uint32_t mdnsTriedAt = 0;
static void webStartMdns() {
  MDNS.end();
  mdnsOk = MDNS.begin(webHostName.c_str());
  if (mdnsOk) MDNS.addService("http", "tcp", 80);
  mdnsTriedAt = millis();
  Serial.printf("MDNS %s.local %s\n", webHostName.c_str(), mdnsOk ? "ok" : "failed");
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
  d["maxblk"] = ESP.getMaxAllocHeap();
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
  d["theme"] = cfg.theme;
  d["wall"] = cfg.wallOn;
  d["phS"] = cfg.photoSec;
  d["fx"] = cfg.fxOn;
  d["lunar"] = cfg.lunarOn;
  d["quote"] = cfg.quoteOn;
  d["idleM"] = cfg.idleMode;
  d["ambS"] = cfg.ambStart;
  d["ambE"] = cfg.ambEnd;
  d["vol"] = cfg.volume;
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
    dr["best2"] = dg.best2;
    dr["streak"] = dg.streak;
    int got = 0;
    for (int i = 0; i < 12; i++) got += (dg.ach >> i) & 1;
    dr["ach"] = got;
    JsonArray jn = dr["journal"].to<JsonArray>();
    for (int i = dg.jnCount - 1; i >= 0; i--) {  // newest first
      const DragonSave::Entry &e = dg.jn[i];
      JsonObject o = jn.add<JsonObject>();
      o["ts"] = e.ts;
      String text;
      if (e.type == 1) text = String(DL("Tiến hóa thành ", "Evolved into ")) + dgStageName(e.arg < ST_COUNT ? e.arg : 0);
      else if (e.type == 2) text = String(DL("Đạt huy hiệu ", "Earned the badge ")) + (isVi() ? ACH[e.arg % 12].vi : ACH[e.arg % 12].en);
      else if (e.type == 3) text = String(DL("Luân hồi - thế hệ ", "Reborn - generation ")) + e.arg;
      else if (e.type == 4) text = String(DL("Mua ", "Bought ")) + (isVi() ? DECOR[e.arg % 6].vi : DECOR[e.arg % 6].en);
      o["text"] = text;
    }
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
  if (has("theme")) {
    cfg.theme = web.arg("theme").toInt() & 3;
    themeTick(true);
  }
  if (has("phS")) cfg.photoSec = constrain(web.arg("phS").toInt(), 3, 120);
  if (has("fx")) cfg.fxOn = web.arg("fx").toInt() != 0;
  if (has("lunar")) cfg.lunarOn = web.arg("lunar").toInt() != 0;
  if (has("quote")) cfg.quoteOn = web.arg("quote").toInt() != 0;
  if (has("idleM")) cfg.idleMode = web.arg("idleM").toInt() ? 1 : 0;
  if (has("ambS")) cfg.ambStart = constrain(web.arg("ambS").toInt(), -1, 23);
  if (has("ambE")) cfg.ambEnd = constrain(web.arg("ambE").toInt(), 0, 23);
  if (has("vol")) cfg.volume = constrain(web.arg("vol").toInt(), 0, 100);
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
  // Developer tools: read the screen back as a BMP, and inject a tap or swipe.
  web.on("/api/screenshot", HTTP_GET, []() {
    if (!webAuthOk()) return;
    const int W = 480, H = 320;
    const uint32_t row = W * 3, size = 54 + row * H;
    uint8_t h[54] = {'B', 'M'};
    auto le32 = [&](int o, uint32_t v) {
      for (int i = 0; i < 4; i++) h[o + i] = (v >> (8 * i)) & 255;
    };
    le32(2, size);
    le32(10, 54);
    le32(14, 40);
    le32(18, W);
    le32(22, H);
    h[26] = 1;
    h[28] = 24;
    le32(34, row * H);
    web.setContentLength(size);
    web.send(200, "image/bmp", "");
    web.sendContent((const char *)h, 54);
    static uint16_t line[480];
    static uint8_t out[480 * 3];
    for (int y = H - 1; y >= 0; y--) {
      tft.readRect(0, y, W, 1, line);
      for (int x = 0; x < W; x++) {
        uint16_t c = line[x];
        c = (c >> 8) | (c << 8);
        out[x * 3 + 0] = (c & 31) << 3;
        out[x * 3 + 1] = ((c >> 5) & 63) << 2;
        out[x * 3 + 2] = (c >> 11) << 3;
      }
      web.sendContent((const char *)out, row);
    }
  });
  web.on("/api/touch", HTTP_ANY, []() {
    if (!webAuthOk()) return;
    int x = web.arg("x").toInt(), y = web.arg("y").toInt();
    int x2 = web.hasArg("x2") ? web.arg("x2").toInt() : x, y2 = web.hasArg("y2") ? web.arg("y2").toInt() : y;
    touchInject(x, y, x2, y2, web.hasArg("ms") ? web.arg("ms").toInt() : 120);
    web.send(200, "application/json", "{}");
  });
  // ---- wallpaper ----
  web.on(
      "/api/wallpaper", HTTP_POST,
      []() {
        if (!webAuthOk()) return;
        bool ok = wallpaperWriteEnd();
        cfg.wallOn = ok;
        cfgSave();
        web.send(ok ? 200 : 400, "text/plain", ok ? "ok" : "bad wallpaper data");
      },
      []() {
        HTTPUpload &u = web.upload();
        if (u.status == UPLOAD_FILE_START) wallpaperWriteBegin();
        else if (u.status == UPLOAD_FILE_WRITE) wallpaperWrite(u.buf, u.currentSize);
      });
  web.on("/api/wallpaper/off", HTTP_POST, []() {
    if (!webAuthOk()) return;
    wallpaperOff();
    cfg.wallOn = false;
    cfgSave();
    web.send(200, "application/json", "{}");
  });

  // ---- memory card: list / download / upload / delete / new folder ----
  web.on("/api/sd/list", HTTP_GET, []() {
    if (!webAuthOk()) return;
    if (!sdOk) return web.send(409, "text/plain", "no card");
    String dir = web.hasArg("dir") ? web.arg("dir") : String("/");
    if (!sdPathOk(dir)) return web.send(400, "text/plain", "bad path");
    File d = SD.open(dir);
    if (!d || !d.isDirectory()) return web.send(404, "text/plain", "not found");
    JsonDocument doc;
    doc["dir"] = dir;
    doc["total"] = (uint32_t)(SD.totalBytes() / 1048576ULL);
    doc["used"] = (uint32_t)(SD.usedBytes() / 1048576ULL);
    JsonArray a = doc["items"].to<JsonArray>();
    for (File f = d.openNextFile(); f && a.size() < 300; f = d.openNextFile()) {
      String n = f.name();
      int sl = n.lastIndexOf('/');
      if (sl >= 0) n = n.substring(sl + 1);
      JsonObject o = a.add<JsonObject>();
      o["n"] = n;
      o["s"] = (uint32_t)f.size();
      o["d"] = f.isDirectory();
      f.close();
    }
    d.close();
    String out;
    serializeJson(doc, out);
    web.send(200, "application/json", out);
  });
  web.on("/api/sd/get", HTTP_GET, []() {
    if (!webAuthOk()) return;
    String path = web.arg("path");
    if (!sdOk || !sdPathOk(path)) return web.send(400, "text/plain", "bad path");
    File f = SD.open(path);
    if (!f || f.isDirectory()) return web.send(404, "text/plain", "not found");
    String low = path;
    low.toLowerCase();
    const char *ct = low.endsWith(".jpg") || low.endsWith(".jpeg") ? "image/jpeg"
                     : low.endsWith(".png")                         ? "image/png"
                     : low.endsWith(".bmp")                         ? "image/bmp"
                     : low.endsWith(".gif")                         ? "image/gif"
                     : low.endsWith(".json") || low.endsWith(".txt") ? "text/plain"
                                                                      : "application/octet-stream";
    web.streamFile(f, ct);
    f.close();
  });
  web.on(
      "/api/sd/upload", HTTP_POST,
      []() {
        if (!webAuthOk()) return;
        web.send(upError ? 500 : 200, "text/plain", upError ? "write failed" : "ok");
      },
      []() {
        HTTPUpload &u = web.upload();
        if (u.status == UPLOAD_FILE_START) {
          upError = false;
          String dir = web.arg("dir");
          if (!sdOk || !sdPathOk(dir) || (webPassword.length() && !web.authenticate("admin", webPassword.c_str()))) {
            upError = true;
            return;
          }
          SD.mkdir(dir);
          String path = (dir.endsWith("/") ? dir : dir + "/") + sdSafeName(u.filename);
          upFile = SD.open(path, FILE_WRITE);
          if (!upFile) upError = true;
        } else if (u.status == UPLOAD_FILE_WRITE) {
          if (!upFile || upError) return;
          size_t done = 0;  // a short write is retried a few times before giving up
          for (int tries = 0; done < u.currentSize && tries < 4; tries++) {
            if (tries) delay(20);
            done += upFile.write(u.buf + done, u.currentSize - done);
          }
          if (done != u.currentSize) {
            upError = true;
            Serial.printf("SD upload: write failed at %u bytes\n", (unsigned)upFile.size());
          }
        } else if (u.status == UPLOAD_FILE_END || u.status == UPLOAD_FILE_ABORTED) {
          if (upFile) {
            Serial.printf("SD upload %s: received %u, file %u\n", u.status == UPLOAD_FILE_END ? "end" : "ABORTED", (unsigned)u.totalSize,
                          (unsigned)upFile.size());
            if (u.status == UPLOAD_FILE_ABORTED) upError = true;
            upFile.close();
          }
        }
      });
  web.on("/api/sd/delete", HTTP_POST, []() {
    if (!webAuthOk()) return;
    String path = web.arg("path");
    if (!sdOk || !sdPathOk(path) || path == "/") return web.send(400, "text/plain", "bad path");
    File f = SD.open(path);
    bool dir = f && f.isDirectory();
    if (f) f.close();
    bool ok = dir ? SD.rmdir(path) : SD.remove(path);
    web.send(ok ? 200 : 409, "text/plain", ok ? "ok" : "could not delete (folders must be empty)");
  });
  web.on("/api/sd/mkdir", HTTP_POST, []() {
    if (!webAuthOk()) return;
    String path = web.arg("path");
    if (!sdOk || !sdPathOk(path)) return web.send(400, "text/plain", "bad path");
    String base = path.substring(0, path.lastIndexOf('/') + 1) + sdSafeName(path.substring(path.lastIndexOf('/') + 1));
    web.send(SD.mkdir(base) ? 200 : 409, "text/plain", "ok");
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
  // (re)announce after every reconnect; retry if it failed (e.g. memory was short during boot)
  if (up && (!webWasConnected || (!mdnsOk && millis() - mdnsTriedAt > 30000))) webStartMdns();
  webWasConnected = up;
  if (up) web.handleClient();
}
