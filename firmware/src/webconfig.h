#pragma once
// Local web setup page: http://<hostname>.local (mDNS) or http://<device ip>.
// For options the on-device Settings screen does not have: start screen, language, clock and greeting,
// weather location/units, payment QR details, countdown events (names with Vietnamese accents, which the on-screen
// keyboard cannot type), app list and order, device name and page password.
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
:root{color-scheme:dark;--bg:#0a0e1a;--surf:#121829;--surf2:#1a2137;--line:#262e48;--txt:#eef1fb;--mut:#8b93b0;--acc:#4cc9f0;--onacc:#03111c;--accs:rgba(76,201,240,.14);--ok:#34d399;--warn:#fbbf24;--bad:#f87171;--sh:0 1px 2px rgba(0,0,0,.3)}
@media(prefers-color-scheme:light){:root{color-scheme:light;--bg:#f3f5fa;--surf:#fff;--surf2:#f1f3f9;--line:#e1e5ef;--txt:#131a2e;--mut:#667089;--acc:#0a7fd0;--onacc:#fff;--accs:rgba(10,127,208,.1);--ok:#059669;--warn:#d97706;--bad:#dc2626;--sh:0 1px 3px rgba(20,30,60,.08)}}
*{box-sizing:border-box}html{-webkit-text-size-adjust:100%}
body{margin:0;background:var(--bg);color:var(--txt);font:15px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
.i{width:18px;height:18px;flex:none;fill:none;stroke:currentColor;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}
/* top bar */
.top{position:sticky;top:0;z-index:5;background:color-mix(in srgb,var(--bg) 88%,transparent);backdrop-filter:blur(10px);border-bottom:1px solid var(--line)}
.bar{max-width:1060px;margin:auto;padding:10px 16px;display:flex;align-items:center;gap:12px}
.logo{width:36px;height:36px;border-radius:10px;background:linear-gradient(135deg,var(--acc),#7b5cff);color:#fff;display:grid;place-items:center;font-weight:800}
.brand{flex:1;min-width:0}.brand b{display:block;font-size:16px}.brand small{color:var(--mut);font-size:12px;display:block;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.chips{display:flex;gap:6px}.chip{display:flex;align-items:center;gap:6px;padding:4px 10px;border:1px solid var(--line);border-radius:99px;font-size:12px;color:var(--mut);white-space:nowrap}
.dot{width:8px;height:8px;border-radius:50%;background:var(--mut)}.dot.ok{background:var(--ok)}.dot.warn{background:var(--warn)}.dot.bad{background:var(--bad)}
/* layout */
.wrap{max-width:1060px;margin:auto;padding:20px 16px 90px;display:grid;grid-template-columns:200px minmax(0,1fr);gap:28px;align-items:start}
nav{position:sticky;top:76px;display:flex;flex-direction:column;gap:2px}
nav a{display:flex;align-items:center;gap:10px;padding:9px 12px;border-radius:10px;color:var(--mut);text-decoration:none;font-weight:500}
nav a:hover{background:var(--surf2);color:var(--txt)}nav a.on{background:var(--accs);color:var(--acc)}
main{max-width:680px;width:100%}.panel{display:none}.panel.on{display:block;animation:fd .2s}
@keyframes fd{from{opacity:0;transform:translateY(4px)}}
h1.pt{font-size:22px;margin:2px 0 14px}
@media(max-width:820px){.wrap{display:block;padding-top:0}.chips .chip.w{display:none}
 nav{position:sticky;top:57px;z-index:4;flex-direction:row;overflow-x:auto;scrollbar-width:none;margin:0 -16px 14px;padding:8px 16px;background:var(--bg);border-bottom:1px solid var(--line)}
 nav::-webkit-scrollbar{display:none}nav a{flex:none;padding:7px 12px;font-size:14px}nav a .i{width:16px;height:16px}h1.pt{display:none}}
/* cards */
.card{background:var(--surf);border:1px solid var(--line);border-radius:16px;margin:0 0 14px;box-shadow:var(--sh)}
.card>.hd{padding:14px 18px 0}.card h2{font-size:15px;margin:0}.card .hd p{margin:3px 0 0;color:var(--mut);font-size:13px}
.bd{padding:6px 18px 14px}.ft{display:flex;align-items:center;justify-content:flex-end;gap:10px;padding:12px 18px;border-top:1px solid var(--line);flex-wrap:wrap}
.rn{margin-right:auto;font-size:12px;color:var(--warn);display:flex;align-items:center;gap:6px}.rn::before{content:"↻";font-weight:700}
.hint{color:var(--mut);font-size:13px;margin:6px 0}
.fld{margin:12px 0 0}.fld>label,.lbl{display:flex;justify-content:space-between;color:var(--mut);font-size:13px;font-weight:500;margin:0 0 6px}
.lbl output{color:var(--txt);font-variant-numeric:tabular-nums}
input,select,textarea{width:100%;padding:10px 12px;border-radius:10px;border:1px solid var(--line);background:var(--surf2);color:var(--txt);font:inherit;outline:none;transition:border-color .15s,box-shadow .15s}
input:focus,select:focus,textarea:focus{border-color:var(--acc);box-shadow:0 0 0 3px var(--accs)}
select{appearance:none;padding-right:34px;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='8'%3E%3Cpath d='M1 1l5 5 5-5' fill='none' stroke='%238b93b0' stroke-width='2'/%3E%3C/svg%3E");background-repeat:no-repeat;background-position:right 12px center}
textarea{min-height:96px;font:13px ui-monospace,Menlo,monospace;word-break:break-all;resize:vertical}
input[type=range]{padding:0;border:0;background:none;accent-color:var(--acc);box-shadow:none;height:24px}
.two{display:grid;grid-template-columns:1fr 1fr;gap:12px}.two>.fld{margin-top:12px}
.rng{display:flex;align-items:center;gap:10px}.rng span{color:var(--mut)}
/* switch rows */
.sw-row{display:flex;align-items:center;justify-content:space-between;gap:14px;padding:12px 0;border-top:1px solid var(--line);cursor:pointer}
.bd>.sw-row:first-child{border-top:0}
.sw{appearance:none;flex:none;width:42px;height:24px;margin:0;padding:0;border-radius:99px;background:var(--line);position:relative;cursor:pointer;border:0;transition:background .2s}
.sw::after{content:"";position:absolute;top:3px;left:3px;width:18px;height:18px;border-radius:50%;background:#fff;box-shadow:0 1px 2px rgba(0,0,0,.3);transition:transform .2s}
.sw:checked{background:var(--acc)}.sw:checked::after{transform:translateX(18px)}.sw:disabled{opacity:.5}.sw:focus{box-shadow:0 0 0 3px var(--accs)}
/* segmented */
.segw{display:flex;flex-wrap:wrap;gap:4px;padding:4px;background:var(--surf2);border:1px solid var(--line);border-radius:12px}
.segw button{flex:1 1 auto;min-width:0;padding:8px 10px;border:0;border-radius:8px;background:none;color:var(--mut);font:inherit;font-size:14px;font-weight:500;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:6px}
.segw button.on{background:var(--surf);color:var(--txt);box-shadow:var(--sh);outline:1px solid var(--line)}
.segw.th button::before{content:"";width:12px;height:12px;border-radius:50%}
.segw.th button:nth-child(1)::before{background:linear-gradient(135deg,#7dd3fc,#2563eb)}.segw.th button:nth-child(2)::before{background:linear-gradient(135deg,#86efac,#15803d)}
.segw.th button:nth-child(3)::before{background:linear-gradient(135deg,#fdba74,#db2777)}.segw.th button:nth-child(4)::before{background:linear-gradient(135deg,#e5e7eb,#4b5563)}
/* buttons */
.btn{display:inline-flex;align-items:center;justify-content:center;gap:8px;padding:9px 16px;border-radius:10px;border:1px solid transparent;font:inherit;font-weight:600;font-size:14px;cursor:pointer;text-decoration:none;white-space:nowrap;transition:filter .15s,background .15s}
.pri{background:var(--acc);color:var(--onacc);min-width:96px}.pri:hover{filter:brightness(1.08)}
.sec{background:var(--surf2);color:var(--txt);border-color:var(--line)}.sec:hover{border-color:var(--mut)}
.ghost{background:none;color:var(--acc);padding:8px 0}.dan{background:none;color:var(--bad);border-color:color-mix(in srgb,var(--bad) 45%,transparent)}.dan:hover{background:color-mix(in srgb,var(--bad) 10%,transparent)}
.ib{width:34px;height:34px;padding:0;border-radius:9px}.ib .i{width:16px;height:16px}
.btn:disabled{opacity:.4;cursor:default}
.grp{display:flex;gap:8px}.grp input{flex:1}
/* status tiles */
.tiles{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:10px;padding:14px 18px 18px}
.tile{background:var(--surf2);border:1px solid var(--line);border-radius:12px;padding:12px 14px;min-width:0}
.tile small{display:block;color:var(--mut);font-size:12px}.tile b{display:block;font-size:16px;margin-top:2px;overflow-wrap:anywhere}.tile em{font-style:normal;font-size:12px;color:var(--mut)}
.sig{display:inline-flex;gap:2px;align-items:flex-end;height:12px;margin-left:6px}.sig i{width:3px;background:var(--line);border-radius:1px}.sig i.on{background:var(--ok)}
/* lists */
.lst{border:1px solid var(--line);border-radius:12px;overflow:hidden;margin-top:10px}
.it{display:flex;align-items:center;gap:10px;padding:8px 10px 8px 12px;border-top:1px solid var(--line);min-height:50px}.it:first-child{border-top:0}
.it .n{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.it .n small{color:var(--mut);margin-left:6px}
.it.dir{cursor:pointer}.it.dir:hover,.res:hover{background:var(--surf2)}.it .ico{color:var(--mut)}.it.dir .ico{color:var(--acc)}
.it .tag{font-size:12px;color:var(--mut)}.empty{padding:22px;text-align:center;color:var(--mut);font-size:14px}
.res{display:block;width:100%;text-align:left;padding:10px 12px;background:none;border:0;border-top:1px solid var(--line);color:var(--txt);font:inherit;cursor:pointer}.res:first-child{border-top:0}.res small{color:var(--mut);display:block}
.loc{display:flex;align-items:center;gap:12px;padding:12px 14px;background:var(--surf2);border-radius:12px;margin-top:10px}.loc .i{color:var(--acc);width:22px;height:22px}.loc b{display:block}.loc small{color:var(--mut)}
/* card manager */
.crumb{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-top:10px;font-size:13px}
.crumb code{font:13px ui-monospace,Menlo,monospace;color:var(--txt);overflow-wrap:anywhere}
.meter{height:6px;border-radius:9px;background:var(--surf2);overflow:hidden;margin-top:8px}.meter i{display:block;height:100%;background:var(--acc);border-radius:9px}
.drop{display:flex;flex-direction:column;align-items:center;gap:6px;padding:20px;border:1.5px dashed var(--line);border-radius:12px;color:var(--mut);text-align:center;cursor:pointer;margin-top:12px;transition:border-color .15s,background .15s}
.drop:hover,.drop.ov{border-color:var(--acc);background:var(--accs);color:var(--txt)}.drop .i{width:26px;height:26px;color:var(--acc)}
.prog{font-size:13px;color:var(--mut);margin:4px 0 0}.prog:empty{display:none}
canvas#wpPrev{display:block;width:100%;aspect-ratio:3/2;border-radius:12px;background:#000;margin-top:10px;border:1px solid var(--line)}
.badge{display:inline-flex;align-items:center;gap:6px;font-size:12px;padding:3px 10px;border-radius:99px;background:var(--surf2);color:var(--mut);border:1px solid var(--line)}
/* dragon */
.dgh{display:flex;align-items:center;gap:14px;padding:16px 18px 4px}.dgh .av{width:52px;height:52px;border-radius:14px;background:linear-gradient(135deg,#f97316,#db2777);display:grid;place-items:center;color:#fff}.dgh .av .i{width:28px;height:28px}
.dgh b{font-size:18px;display:block}.dgh small{color:var(--mut)}
.pb{padding:10px 18px 0}.pb .meter{height:8px}.pb .lbl{margin:0}
.tl{list-style:none;margin:0;padding:0 18px 16px}.tl li{position:relative;padding:0 0 12px 20px;font-size:14px}.tl li::before{content:"";position:absolute;left:3px;top:6px;width:8px;height:8px;border-radius:50%;background:var(--acc)}
.tl li::after{content:"";position:absolute;left:6.5px;top:16px;bottom:0;width:1px;background:var(--line)}.tl li:last-child::after{display:none}.tl small{display:block;color:var(--mut);font-size:12px}
.card.danger{border-color:color-mix(in srgb,var(--bad) 35%,var(--line))}
/* toast */
#msg{position:fixed;left:50%;bottom:20px;transform:translate(-50%,20px);max-width:calc(100% - 32px);padding:11px 18px;border-radius:12px;background:var(--txt);color:var(--bg);font-weight:600;font-size:14px;box-shadow:0 8px 24px rgba(0,0,0,.3);opacity:0;pointer-events:none;transition:opacity .2s,transform .2s;z-index:9;display:flex;gap:8px;align-items:center}
#msg.show{opacity:1;transform:translate(-50%,0)}#msg::before{content:"";width:8px;height:8px;border-radius:50%;background:var(--ok);flex:none}#msg.err::before{background:var(--bad)}
@media(max-width:480px){.two{grid-template-columns:1fr;gap:0}.ft .pri{flex:1}}
</style></head><body>
<svg width=0 height=0 style="position:absolute"><defs>
<symbol id=i-home viewBox="0 0 24 24"><path d="M3 10.5 12 3l9 7.5V20a1 1 0 0 1-1 1h-5v-6H9v6H4a1 1 0 0 1-1-1z"/></symbol>
<symbol id=i-disp viewBox="0 0 24 24"><rect x=3 y=4 width=18 height=12 rx=2 /><path d="M8 20h8M12 16v4"/></symbol>
<symbol id=i-img viewBox="0 0 24 24"><rect x=3 y=3 width=18 height=18 rx=2 /><circle cx=9 cy=9 r=2 /><path d="m21 15-5-5L5 21"/></symbol>
<symbol id=i-cloud viewBox="0 0 24 24"><path d="M17.5 19H7a5 5 0 1 1 1.6-9.7A6 6 0 0 1 20 11.5 3.8 3.8 0 0 1 17.5 19z"/></symbol>
<symbol id=i-qr viewBox="0 0 24 24"><rect x=3 y=3 width=7 height=7 rx=1 /><rect x=14 y=3 width=7 height=7 rx=1 /><rect x=3 y=14 width=7 height=7 rx=1 /><path d="M14 14h3v3h-3zM21 14v.01M14 21h.01M17 21h4v-4"/></symbol>
<symbol id=i-apps viewBox="0 0 24 24"><rect x=3 y=3 width=7 height=7 rx=2 /><rect x=14 y=3 width=7 height=7 rx=2 /><rect x=3 y=14 width=7 height=7 rx=2 /><rect x=14 y=14 width=7 height=7 rx=2 /></symbol>
<symbol id=i-dg viewBox="0 0 24 24"><path d="M12 3c1 4 5 5.5 5 10a5 5 0 0 1-10 0c0-2.5 1.2-3.8 2.5-5 0 2 .8 3.2 2 3.5C11 9 10.8 6 12 3z"/></symbol>
<symbol id=i-sys viewBox="0 0 24 24"><path d="M4 6h9M17 6h3M4 12h3M11 12h9M4 18h11M19 18h1"/><circle cx=15 cy=6 r=2 /><circle cx=9 cy=12 r=2 /><circle cx=17 cy=18 r=2 /></symbol>
<symbol id=i-dir viewBox="0 0 24 24"><path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/></symbol>
<symbol id=i-file viewBox="0 0 24 24"><path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8zM14 3v5h5"/></symbol>
<symbol id=i-dl viewBox="0 0 24 24"><path d="M12 4v11m-4-4 4 4 4-4M5 20h14"/></symbol>
<symbol id=i-up viewBox="0 0 24 24"><path d="M12 20V9m-4 4 4-4 4 4M5 4h14"/></symbol>
<symbol id=i-del viewBox="0 0 24 24"><path d="M4 7h16M10 11v6M14 11v6M6 7l1 13h10l1-13M9 7V4h6v3"/></symbol>
<symbol id=i-cu viewBox="0 0 24 24"><path d="m6 15 6-6 6 6"/></symbol>
<symbol id=i-cd viewBox="0 0 24 24"><path d="m6 9 6 6 6-6"/></symbol>
<symbol id=i-back viewBox="0 0 24 24"><path d="m15 18-6-6 6-6"/></symbol>
<symbol id=i-pin viewBox="0 0 24 24"><path d="M12 21s7-6.2 7-12a7 7 0 0 0-14 0c0 5.8 7 12 7 12z"/><circle cx=12 cy=9 r=2.5 /></symbol>
<symbol id=i-hg viewBox="0 0 24 24"><path d="M6 3h12M6 21h12M7 3c0 5 10 5 10 9s-10 4-10 9M17 3c0 5-10 5-10 9s10 4 10 9"/></symbol>
<symbol id=i-plus viewBox="0 0 24 24"><path d="M12 5v14M5 12h14"/></symbol>
<symbol id=i-lock viewBox="0 0 24 24"><rect x=5 y=11 width=14 height=10 rx=2 /><path d="M8 11V7a4 4 0 0 1 8 0v4"/></symbol>
</defs></svg>
<header class=top><div class=bar><div class=logo>Z</div><div class=brand><b id=hName>Zin100</b><small id=addr></small></div><div class=chips id=chips></div></div></header>
<div class=wrap>
<nav id=nav>
<a href=#ov data-p=ov><svg class=i><use href=#i-home /></svg><span data-t=nOv></span></a>
<a href=#disp data-p=disp><svg class=i><use href=#i-disp /></svg><span data-t=nDisp></span></a>
<a href=#media data-p=media><svg class=i><use href=#i-img /></svg><span data-t=nMedia></span></a>
<a href=#wx data-p=wx><svg class=i><use href=#i-cloud /></svg><span data-t=wxTitle></span></a>
<a href=#qr data-p=qr><svg class=i><use href=#i-qr /></svg><span data-t=nQr></span></a>
<a href=#cd data-p=cd><svg class=i><use href=#i-hg /></svg><span data-t=nCd></span></a>
<a href=#apps data-p=apps><svg class=i><use href=#i-apps /></svg><span data-t=appsTitle></span></a>
<a href=#dg data-p=dg><svg class=i><use href=#i-dg /></svg><span data-t=nDg></span></a>
<a href=#sys data-p=sys><svg class=i><use href=#i-sys /></svg><span data-t=nSys></span></a>
</nav>
<main>

<div class=panel id=p-ov><h1 class=pt data-t=nOv></h1>
<section class=card><div class=hd><h2 data-t=status></h2></div><div class=tiles id=info></div></section>
<section class=card><div class=hd><h2 data-t=dragonTitle></h2></div><div id=ovDg></div></section>
</div>

<div class=panel id=p-disp><h1 class=pt data-t=nDisp></h1>
<section class=card><div class=hd><h2 data-t=startTitle></h2></div><div class=bd>
<div class=fld><label for=start data-t=startLbl></label><select id=start><option value=0 data-t=tHome><option value=1 data-t=tQr><option value=2 data-t=tApps></select></div>
<div class=fld><label for=idle data-t=idleLbl></label><select id=idle><option value=0 data-t=never><option value=30>30 s<option value=60>1 min<option value=120>2 min<option value=300>5 min<option value=600>10 min</select></div>
<label class=sw-row style="margin-top:12px"><span data-t=qrOn></span><input type=checkbox class=sw id=qrOn></label>
</div><div class=ft><span class=rn data-t=restartNote></span><button class="btn pri" id=saveStart data-t=save></button></div></section>

<section class=card><div class=hd><h2 data-t=lookTitle></h2></div><div class=bd>
<div class=fld><span class=lbl data-t=themeLbl></span><select id=theme class=seg data-k=th><option value=0 data-t=thSky><option value=1 data-t=thForest><option value=2 data-t=thSunset><option value=3 data-t=thMono></select></div>
<label class=sw-row style="margin-top:8px"><span data-t=fxOn></span><input type=checkbox class=sw id=fx></label>
<label class=sw-row><span data-t=lunarOn></span><input type=checkbox class=sw id=lunar></label>
<label class=sw-row><span data-t=quoteOn></span><input type=checkbox class=sw id=quote></label>
<div class=fld style="margin-top:4px"><label for=phS data-t=phLbl></label><select id=phS><option value=5>5 s<option value=10>10 s<option value=20>20 s<option value=30>30 s<option value=60>60 s</select></div>
</div><div class=ft><button class="btn pri" id=saveLook data-t=save></button></div></section>

<section class=card><div class=hd><h2 data-t=clockTitle></h2></div><div class=bd>
<div class=two><div class=fld><span class=lbl data-t=timeFmt></span><select id=c24 class=seg><option value=1 data-t=h24><option value=0 data-t=h12></select></div>
<div class=fld><label for=datef data-t=dateFmt></label><select id=datef><option value=0 data-t=dLong><option value=1 data-t=dNum><option value=2 data-t=dHide></select></div></div>
<label class=sw-row style="margin-top:12px"><span data-t=greetOn></span><input type=checkbox class=sw id=greet></label>
<div class=fld style="margin-top:0"><label for=name data-t=nameLbl></label><input id=name maxlength=20 autocomplete=off></div>
</div><div class=ft><button class="btn pri" id=saveClock data-t=save></button></div></section>

<section class=card><div class=hd><h2 data-t=ambTitle></h2><p data-t=idleHint></p></div><div class=bd>
<div class=fld><label for=idleM data-t=idleMLbl></label><select id=idleM><option value=1 data-t=idleAmb><option value=0 data-t=idleOff></select></div>
<div class=fld><span class=lbl data-t=ambSched></span><div class=rng><select id=ambS></select><span>→</span><select id=ambE></select></div></div>
<div class=fld><span class=lbl><span data-t=volLbl></span><output id=volV></output></span><input type=range id=vol min=0 max=100></div>
</div><div class=ft><button class="btn pri" id=saveAmb data-t=save></button></div></section>
</div>

<div class=panel id=p-media><h1 class=pt data-t=nMedia></h1>
<section class=card><div class=hd><h2 data-t=wpTitle></h2><p data-t=wpHint></p></div><div class=bd>
<canvas id=wpPrev width=480 height=320></canvas>
<div class=crumb><span class=badge id=wpState></span><label class="btn sec"><input type=file id=wpFile accept="image/*" hidden><svg class=i><use href=#i-img /></svg><span data-t=wpChoose></span></label></div>
<div class=fld><span class=lbl><span data-t=wpDim></span><output id=wpDimV></output></span><input type=range id=wpDim min=0 max=70 value=35></div>
</div><div class=ft><button class="btn dan" id=wpOff data-t=wpOff></button><button class="btn pri" id=wpSet data-t=wpSet></button></div></section>

<section class=card><div class=hd><h2 data-t=sdTitle></h2></div><div class=bd>
<div class=crumb><code id=sdPath>/</code><span class=hint style="margin:0" id=sdFree></span></div>
<div class=meter><i id=sdBar style="width:0"></i></div>
<div class=lst id=sdList></div>
<label class=drop id=sdDrop><input type=file id=sdFiles multiple hidden><svg class=i><use href=#i-up /></svg><span id=sdPick data-t=sdChoose></span></label>
<label class=sw-row style="border:0"><span data-t=sdShrink></span><input type=checkbox class=sw id=sdShrink checked></label>
<p class=prog id=sdProg></p>
</div><div class=ft><button class="btn sec" id=sdMk><svg class=i><use href=#i-plus /></svg><span data-t=sdMkdir></span></button><button class="btn pri" id=sdUp data-t=sdUpload></button></div></section>
</div>

<div class=panel id=p-wx><h1 class=pt data-t=wxTitle></h1>
<section class=card><div class=hd><h2 data-t=wxNow></h2></div><div class=bd>
<div class=loc><svg class=i><use href=#i-pin /></svg><div><b id=wxCity>—</b><small id=wxNow></small></div></div>
<div class=fld><label for=wxQ data-t=wxSearch></label><div class=grp><input id=wxQ data-ph=wxPh type=search><button class="btn sec" id=wxFind data-t=search></button></div></div>
<div id=wxRes></div>
<button class="btn ghost" id=wxAuto><svg class=i><use href=#i-pin /></svg><span data-t=wxAuto></span></button>
</div></section>
<section class=card><div class=hd><h2 data-t=unit></h2></div><div class=bd><div class=fld style="margin-top:6px"><select id=tempF class=seg><option value=0>°C<option value=1>°F</select></div></div>
<div class=ft><button class="btn pri" id=saveUnit data-t=save></button></div></section>
</div>

<div class=panel id=p-qr><h1 class=pt data-t=nQr></h1>
<section class=card><div class=hd><h2 data-t=qrList></h2><p data-t=qrListHint></p></div><div class=bd><div class=lst id=qrLst></div>
<div style="margin-top:12px"><button class="btn sec" id=qrNew data-t=qrAdd></button></div></div></section>
<section class=card><div class=hd><h2 id=qrEdTitle></h2><p data-t=qrHint></p></div><div class=bd>
<div class=fld><label for=qrT data-t=qrT></label><input id=qrT maxlength=30 data-ph=qrTPh></div>
<div class=fld><label for=qrB data-t=qrB></label><input id=qrB maxlength=38></div>
<div class=two><div class=fld><label for=qrN data-t=qrN></label><input id=qrN maxlength=38></div><div class=fld><label for=qrA data-t=qrA></label><input id=qrA maxlength=22 inputmode=numeric></div></div>
<div class=fld><label for=qrP><span data-t=qrP></span><output id=qrLen></output></label><textarea id=qrP maxlength=500 spellcheck=false placeholder="000201..."></textarea></div>
</div><div class=ft><button class="btn pri" id=saveQr data-t=save></button></div></section>
</div>

<div class=panel id=p-cd><h1 class=pt data-t=nCd></h1>
<section class=card><div class=hd><h2 data-t=cdList></h2><p data-t=cdHint></p></div><div class=bd><div class=lst id=cdLst></div>
<div style="margin-top:12px"><button class="btn sec" id=cdNew data-t=cdAdd></button></div></div></section>
<section class=card><div class=hd><h2 id=cdEdTitle></h2></div><div class=bd>
<div class=fld><label for=cdN data-t=cdName></label><input id=cdN maxlength=30 data-ph=cdNamePh></div>
<div class=fld><label for=cdK data-t=cdKind></label><select id=cdK><option value=0 data-t=cdOnce><option value=1 data-t=cdYearly><option value=2 data-t=cdLunar></select></div>
<div class=two><div class=fld><label for=cdD data-t=cdDay></label><select id=cdD></select></div><div class=fld><label for=cdM data-t=cdMonth></label><select id=cdM></select></div></div>
<div class=fld id=cdYf><label for=cdY data-t=cdYear></label><input id=cdY type=number min=2024 max=2099 inputmode=numeric></div>
<p class=hint id=cdLunHint data-t=cdLunHint></p>
<label class=sw-row style="margin-top:8px"><span data-t=cdHome></span><input type=checkbox class=sw id=cdH></label>
</div><div class=ft><button class="btn pri" id=saveCd data-t=save></button></div></section>
</div>

<div class=panel id=p-apps><h1 class=pt data-t=appsTitle></h1>
<section class=card><div class=hd><h2 data-t=appsTitle></h2><p data-t=appsHint></p></div><div class=bd><div class=lst id=apps></div></div>
<div class=ft><span class=rn data-t=restartNote></span><button class="btn pri" id=saveApps data-t=save></button></div></section>
</div>

<div class=panel id=p-dg><h1 class=pt data-t=nDg></h1>
<section class=card><div id=dgInfo></div></section>
<section class=card><div class=hd><h2 data-t=dgName></h2></div><div class=bd><div class=fld style="margin-top:6px"><input id=dgNameIn maxlength=20 autocomplete=off></div></div>
<div class=ft><button class="btn pri" id=saveDg data-t=save></button></div></section>
<section class="card danger"><div class=hd><h2 data-t=danger></h2><p data-t=dgConfirm></p></div><div class=ft><button class="btn dan" id=dgReset data-t=dgReset></button></div></section>
</div>

<div class=panel id=p-sys><h1 class=pt data-t=nSys></h1>
<section class=card><div class=hd><h2 data-t=lang></h2></div><div class=bd><div class=fld style="margin-top:6px"><select id=lang class=seg><option value=0>Tiếng Việt<option value=1>English</select></div></div>
<div class=ft><span class=rn data-t=restartNote></span><button class="btn pri" id=saveLang data-t=save></button></div></section>
<section class=card><div class=hd><h2 data-t=hostLbl></h2></div><div class=bd><div class=fld style="margin-top:6px"><div class=grp><input id=host autocapitalize=off autocomplete=off spellcheck=false maxlength=24><span class=hint style="align-self:center;margin:0">.local</span></div></div></div>
<div class=ft><span class=rn data-t=restartNote></span><button class="btn pri" id=saveHost data-t=save></button></div></section>
<section class=card><div class=hd><h2 data-t=pwLbl></h2><p data-t=pwHint></p></div><div class=bd><div class=fld style="margin-top:6px"><input id=wpw type=password autocomplete=new-password></div></div>
<div class=ft><button class="btn pri" id=savePw data-t=save></button></div></section>
</div>
</main></div><div id=msg role=status></div>
<script>
const $=id=>document.getElementById(id);
const D={vi:{status:'Trạng thái',lang:'Ngôn ngữ',restartNote:'Thiết bị sẽ khởi động lại',save:'Lưu',startTitle:'Màn hình chính',startLbl:'Màn hình khi bật máy',tHome:'Trang chủ',tQr:'Mã QR thanh toán',tApps:'Ứng dụng',idleLbl:'Tự quay về màn hình chính sau',never:'Không bao giờ',qrOn:'Hiện trang QR thanh toán',
clockTitle:'Đồng hồ & chào hỏi',timeFmt:'Định dạng giờ',h24:'24 giờ',h12:'12 giờ',dateFmt:'Hiển thị ngày',dLong:'Đầy đủ (Thứ Hai, 19 tháng 10)',dNum:'Số (19/10/2026)',dHide:'Ẩn',greetOn:'Hiện lời chào',nameLbl:'Tên trong lời chào (không bắt buộc)',
wxTitle:'Thời tiết',wxNow:'Vị trí hiện tại',wxSearch:'Đổi thành phố',wxPh:'Ví dụ: Đà Nẵng',search:'Tìm',wxAuto:'Tự xác định vị trí (theo IP)',unit:'Đơn vị nhiệt độ',auto:'Tự động',manual:'Đặt thủ công',noRes:'Không tìm thấy',
qrTitle:'QR thanh toán',qrHint:'Dán chuỗi VietQR/EMV (bắt đầu bằng 000201...) lấy từ ứng dụng ngân hàng.',qrT:'Tiêu đề',qrTPh:'Quét để thanh toán',qrB:'Dòng ngân hàng',qrN:'Tên tài khoản',qrA:'Số tài khoản',qrP:'Nội dung mã QR',qrList:'Tài khoản nhận tiền',qrListHint:'Thêm nhiều tài khoản; tài khoản đang dùng hiển thị trên màn hình chính. Cũng chọn được trong Cài đặt trên thiết bị.',qrUse:'Dùng',qrInUse:'đang dùng',qrEdit:'Sửa',qrAdd:'Thêm tài khoản',qrEdNew:'Tài khoản mới',qrEdEdit:'Sửa tài khoản',qrDelAsk:'Xoá tài khoản này?',qrMax:'Tối đa 8 tài khoản',
nCd:'Đếm ngược',cdList:'Sự kiện',cdHint:'Tên có dấu gõ ở đây; danh sách đồng bộ với ứng dụng Đếm ngược trên thiết bị. Tối đa 12 sự kiện.',cdAdd:'Thêm sự kiện',cdEdNew:'Sự kiện mới',cdEdEdit:'Sửa sự kiện',cdName:'Tên sự kiện',cdNamePh:'Ví dụ: Sinh nhật mẹ',cdKind:'Lặp lại',cdOnce:'Một lần',cdYearly:'Hằng năm (dương lịch)',cdLunar:'Hằng năm (âm lịch)',cdDay:'Ngày',cdMonth:'Tháng',cdYear:'Năm',cdLunHint:'Ngày và tháng theo âm lịch, ví dụ Tết là 1/1, Trung thu là 15/8.',cdHome:'Hiện trên màn hình chính',cdDelAsk:'Xoá sự kiện này?',cdToday:'Hôm nay!',cdTomorrow:'Ngày mai',cdLeft:'Còn ',cdDays:' ngày',cdPast:'Đã qua',cdOnHome:'Trên màn hình chính',cdNone:'Chưa có sự kiện',cdNeedName:'Nhập tên sự kiện',
appsTitle:'Ứng dụng',appsHint:'Bật ứng dụng muốn hiển thị, dùng mũi tên để đổi thứ tự.',lock:'Luôn hiện',
hostLbl:'Tên thiết bị',pwLbl:'Mật khẩu trang web',pwHint:'Tên đăng nhập là admin. Để trống rồi lưu để bỏ mật khẩu.',
wifi:'Wi-Fi',ip:'Địa chỉ IP',fw:'Phần mềm',sd:'Thẻ nhớ',heap:'RAM trống',up:'Thời gian chạy',sdOk:'Có thẻ',sdNo:'Không có thẻ',sigS:'Mạnh',sigM:'Khá',sigW:'Yếu',
nOv:'Tổng quan',nDisp:'Màn hình',nMedia:'Ảnh & thẻ nhớ',nQr:'Thanh toán',nDg:'Luyện rồng',nSys:'Hệ thống',danger:'Nuôi lại từ đầu',
lookTitle:'Giao diện',phLbl:'Khung ảnh: thời gian mỗi ảnh',themeLbl:'Chủ đề màu',thSky:'Bầu trời',thForest:'Rừng',thSunset:'Hoàng hôn',thMono:'Đơn sắc',fxOn:'Hiệu ứng thời tiết trên trang chủ',lunarOn:'Hiện lịch âm',quoteOn:'Hiện câu nói mỗi ngày',ambTitle:'Màn hình chờ & âm thanh',idleMLbl:'Khi không dùng một lúc thì',idleAmb:'Hiện màn hình chờ (đồng hồ lớn, tối dần)',idleOff:'Tắt màn hình',idleHint:'Thời gian chờ chỉnh trong Cài đặt trên thiết bị.',ambSched:'Tự vào màn hình chờ ban đêm (từ → đến)',schedOff:'Tắt',volLbl:'Âm lượng loa',wpTitle:'Hình nền',wpHint:'Chọn ảnh từ máy hoặc bấm biểu tượng ảnh ở một file trong thẻ nhớ. Ảnh được cắt về 480×320 và lưu trong thiết bị.',wpChoose:'Chọn ảnh',wpDim:'Làm tối ảnh để chữ dễ đọc',wpSet:'Đặt làm hình nền',wpOff:'Bỏ hình nền',wpNone:'Chưa có hình nền',wpHas:'Đang dùng hình nền',wpPick:'Hãy chọn ảnh trước',wpSending:'Đang gửi hình nền...',sdTitle:'Thẻ nhớ',sdShrink:'Thu nhỏ ảnh về 480×320 trước khi chép (khuyên dùng)',sdChoose:'Kéo thả file vào đây hoặc bấm để chọn',sdPicked:' file đã chọn',sdUpload:'Chép lên thẻ',sdMkdir:'Thư mục mới',sdNoCard:'Không có thẻ nhớ',sdEmpty:'Thư mục trống',sdDel:'Xóa',sdDelQ:'Xóa ',sdBg:'Đặt làm nền',sdFree:' trống',sdNewName:'Tên thư mục mới',sdPickFiles:'Hãy chọn file trước',sdUploading:'Đang chép ',sdDone:'Đã chép xong',dragonTitle:'Luyện rồng',dgName:'Tên rồng',dgReset:'Xóa dữ liệu rồng',dgConfirm:'Xóa toàn bộ dữ liệu rồng và bắt đầu lại từ quả trứng?',dgNoSd:'Cần thẻ nhớ để nuôi rồng',dgStage:'Cấp độ',dgGrow:'Tiến độ',dgCoins:'Số xu',dgGen:'Thế hệ',dgBest:'Kỷ lục mini game',dgSick:'Đang ốm',dgAch:'Huy hiệu',dgStreak:'Chuỗi ngày',dgBest2:'Kỷ lục Rồng bay',dgJournal:'Nhật ký',dgDays:' ngày',dgNoName:'Rồng chưa có tên',saved:'Đã lưu',restarting:'Đã lưu. Thiết bị đang khởi động lại...',failed:'Lỗi: ',pwSet:'Đã đặt mật khẩu',pwOff:'Đã xóa mật khẩu',wxSaved:'Đã đặt vị trí thời tiết',needName:'Nhập tên thiết bị'},
en:{status:'Status',lang:'Language',restartNote:'The device will restart',save:'Save',startTitle:'Main screen',startLbl:'Screen shown at power-on',tHome:'Home',tQr:'Payment QR',tApps:'Apps',idleLbl:'Return to the main screen after',never:'Never',qrOn:'Show the payment QR page',
clockTitle:'Clock & greeting',timeFmt:'Time format',h24:'24-hour',h12:'12-hour',dateFmt:'Date display',dLong:'Long (Monday, 19 October)',dNum:'Numeric (19/10/2026)',dHide:'Hidden',greetOn:'Show greeting',nameLbl:'Name in the greeting (optional)',
wxTitle:'Weather',wxNow:'Current location',wxSearch:'Change city',wxPh:'e.g. Da Nang',search:'Search',wxAuto:'Detect location automatically (by IP)',unit:'Temperature unit',auto:'Automatic',manual:'Set manually',noRes:'No results',
qrTitle:'Payment QR',qrHint:'Paste the VietQR/EMV string (starts with 000201...) from your banking app.',qrT:'Title',qrTPh:'Scan to pay',qrB:'Bank line',qrN:'Account name',qrA:'Account number',qrP:'QR content',qrList:'Receiving accounts',qrListHint:'Add several accounts; the one in use is shown on the main screen. It can also be chosen in Settings on the device.',qrUse:'Use',qrInUse:'in use',qrEdit:'Edit',qrAdd:'Add an account',qrEdNew:'New account',qrEdEdit:'Edit account',qrDelAsk:'Delete this account?',qrMax:'At most 8 accounts',
nCd:'Countdown',cdList:'Events',cdHint:'Type names with accents here; the list is the same as in the Countdown app on the device. Up to 12 events.',cdAdd:'Add event',cdEdNew:'New event',cdEdEdit:'Edit event',cdName:'Event name',cdNamePh:'e.g. Mum\'s birthday',cdKind:'Repeats',cdOnce:'Once',cdYearly:'Every year (solar)',cdLunar:'Every year (lunar)',cdDay:'Day',cdMonth:'Month',cdYear:'Year',cdLunHint:'Day and month of the lunar calendar: Tết is 1/1, Mid-Autumn 15/8.',cdHome:'Show on the home screen',cdDelAsk:'Delete this event?',cdToday:'Today!',cdTomorrow:'Tomorrow',cdLeft:'',cdDays:' days left',cdPast:'Past',cdOnHome:'On the home screen',cdNone:'No events yet',cdNeedName:'Enter the event name',
appsTitle:'Apps',appsHint:'Switch on the apps to show and use the arrows to reorder.',lock:'Always shown',
hostLbl:'Device name',pwLbl:'Web page password',pwHint:'The user name is admin. Save it empty to remove the password.',
wifi:'Wi-Fi',ip:'IP address',fw:'Firmware',sd:'microSD',heap:'Free RAM',up:'Uptime',sdOk:'Detected',sdNo:'No card',sigS:'Strong',sigM:'Fair',sigW:'Weak',
nOv:'Overview',nDisp:'Display',nMedia:'Photos & card',nQr:'Payment',nDg:'Dragon',nSys:'System',danger:'Start over',
lookTitle:'Look',phLbl:'Photo frame: time per picture',themeLbl:'Colour theme',thSky:'Sky',thForest:'Forest',thSunset:'Sunset',thMono:'Mono',fxOn:'Weather effects on the home screen',lunarOn:'Show the lunar date',quoteOn:'Show the quote of the day',ambTitle:'Ambient screen & sound',idleMLbl:'After a while without use',idleAmb:'Show the ambient clock (dims)',idleOff:'Turn the screen off',idleHint:'The idle time is set in Settings on the device.',ambSched:'Ambient clock at night (from → to)',schedOff:'Off',volLbl:'Speaker volume',wpTitle:'Wallpaper',wpHint:'Pick a picture from this computer or press the picture icon on a file on the card. It is cropped to 480×320 and stored inside the device.',wpChoose:'Choose picture',wpDim:'Darken the picture so text stays readable',wpSet:'Set as wallpaper',wpOff:'Remove wallpaper',wpNone:'No wallpaper',wpHas:'Wallpaper in use',wpPick:'Pick a picture first',wpSending:'Sending the wallpaper...',sdTitle:'Memory card',sdShrink:'Shrink pictures to 480×320 before copying (recommended)',sdChoose:'Drop files here or click to choose',sdPicked:' file(s) selected',sdUpload:'Copy to card',sdMkdir:'New folder',sdNoCard:'No memory card',sdEmpty:'Empty folder',sdDel:'Delete',sdDelQ:'Delete ',sdBg:'Use as wallpaper',sdFree:' free',sdNewName:'New folder name',sdPickFiles:'Pick files first',sdUploading:'Copying ',sdDone:'All files copied',dragonTitle:'Dragon training',dgName:'Dragon name',dgReset:'Erase dragon data',dgConfirm:'Erase all dragon data and start again from an egg?',dgNoSd:'A microSD card is needed to raise a dragon',dgStage:'Stage',dgGrow:'Progress',dgCoins:'Coins',dgGen:'Generation',dgBest:'Mini-game best',dgSick:'Sick',dgAch:'Badges',dgStreak:'Day streak',dgBest2:'Dragon flight best',dgJournal:'Journal',dgDays:' days',dgNoName:'Unnamed dragon',saved:'Saved',restarting:'Saved. The device is restarting...',failed:'Failed: ',pwSet:'Password set',pwOff:'Password removed',wxSaved:'Weather location set',needName:'Enter a device name'}};
let T=D.vi,S=null,order=[];
const ic=n=>'<svg class=i><use href=#i-'+n+' /></svg>';
const esc=s=>String(s).replace(/[&<>"']/g,c=>'&#'+c.charCodeAt(0)+';');
function toast(t,err){const m=$('msg');m.textContent=t;m.className='show'+(err?' err':'');clearTimeout(toast.t);toast.t=setTimeout(()=>m.className=m.className.replace('show',''),3500)}
function go(p){if(!$('p-'+p))p='ov';document.querySelectorAll('.panel').forEach(e=>e.classList.toggle('on',e.id=='p-'+p));
 document.querySelectorAll('nav a').forEach(a=>{const on=a.dataset.p==p;a.classList.toggle('on',on);if(on&&a.scrollIntoView&&innerWidth<820)a.scrollIntoView({block:'nearest',inline:'center'})});scrollTo(0,0)}
onhashchange=()=>go(location.hash.slice(1));
function segs(){document.querySelectorAll('select.seg').forEach(s=>{let w=s.nextElementSibling;
 if(!w||!w.classList.contains('segw')){w=document.createElement('div');w.className='segw '+(s.dataset.k||'');s.after(w);s.hidden=true}
 w.innerHTML=[...s.options].map(o=>'<button type=button data-v="'+o.value+'"'+(o.value==s.value?' class=on':'')+'>'+o.textContent+'</button>').join('');
 w.querySelectorAll('button').forEach(b=>b.onclick=()=>{s.value=b.dataset.v;segs()})})}
function applyLang(l){T=D[l==1?'en':'vi'];document.documentElement.lang=l==1?'en':'vi';
 document.querySelectorAll('[data-t]').forEach(e=>e.textContent=T[e.dataset.t]);
 document.querySelectorAll('[data-ph]').forEach(e=>e.placeholder=T[e.dataset.ph])}
async function post(path,body){try{const r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(body)});
 if(!r.ok)throw new Error(await r.text()||r.status);return await r.json().catch(()=>({}))}catch(e){toast(T.failed+e.message,1);return null}}
async function busy(b,fn){b.disabled=true;try{await fn()}finally{b.disabled=false}}
async function saveCfg(body){const r=await post('/api/cfg',body);if(!r)return;
 if(r.restart){toast(T.restarting);setTimeout(()=>location.reload(),12000)}else{toast(T.saved);load()}}
function tile(k,v,sub){return '<div class=tile><small>'+k+'</small><b>'+v+'</b>'+(sub?'<em>'+sub+'</em>':'')+'</div>'}
function sig(r){const n=r>-60?4:r>-67?3:r>-75?2:1;return '<span class=sig>'+[4,7,10,13].map((h,i)=>'<i style="height:'+h+'px"'+(i<n?' class=on':'')+'></i>').join('')+'</span>'}
function drawApps(){$('apps').innerHTML=order.map((a,i)=>'<div class=it><input type=checkbox class=sw '+(a.on||a.lock?'checked':'')+(a.lock?' disabled':'')+' data-i='+i+' aria-label="'+esc(a.name)+'"><span class=n>'+esc(a.name)+(a.lock?'<small>'+T.lock+'</small>':'')+'</span>'+
 '<button class="btn sec ib" data-up='+i+(i?'':' disabled')+' aria-label=up>'+ic('cu')+'</button><button class="btn sec ib" data-dn='+i+(i<order.length-1?'':' disabled')+' aria-label=down>'+ic('cd')+'</button></div>').join('');
 $('apps').querySelectorAll('input').forEach(c=>c.onchange=()=>{order[c.dataset.i].on=c.checked});
 $('apps').querySelectorAll('[data-up]').forEach(b=>b.onclick=()=>mv(+b.dataset.up,-1));
 $('apps').querySelectorAll('[data-dn]').forEach(b=>b.onclick=()=>mv(+b.dataset.dn,1))}
function mv(i,d){const j=i+d;if(j<0||j>=order.length)return;[order[i],order[j]]=[order[j],order[i]];drawApps()}
function dragon(g,full){if(!g.ok)return '<div class=empty>'+T.dgNoSd+'</div>';
 let h='<div class=dgh><div class=av>'+ic('dg')+'</div><div><b>'+esc(g.name||T.dgNoName)+'</b><small>'+esc(g.stageName)+(g.sick?' · '+T.dgSick:'')+' · '+T.dgGen+' '+g.gen+'</small></div></div>'+
 '<div class=pb><span class=lbl><span>'+T.dgGrow+'</span><output>'+g.pct+'%</output></span><div class=meter><i style="width:'+g.pct+'%"></i></div></div>'+
 '<div class=tiles>'+tile(T.dgCoins,g.coins)+tile(T.dgStreak,g.streak+T.dgDays)+tile(T.dgAch,g.ach+'/12')+(full?tile(T.dgBest,g.best)+tile(T.dgBest2,g.best2):'')+'</div>';
 if(full&&g.journal&&g.journal.length)h+='<h2 style="font-size:15px;margin:4px 18px 12px">'+T.dgJournal+'</h2><ul class=tl>'+g.journal.map(e=>'<li>'+esc(e.text)+'<small>'+(e.ts?new Date(e.ts*1000).toLocaleString():'—')+'</small></li>').join('')+'</ul>';
 return h}
async function load(){S=await (await fetch('/api/state')).json();applyLang(S.lang);
 $('hName').textContent=S.host;document.title=S.host+' · Zin100';$('addr').textContent=S.host+'.local · '+S.ip;
 const sq=S.rssi>-60?T.sigS:S.rssi>-75?T.sigM:T.sigW;
 $('chips').innerHTML='<span class="chip w" title="Wi-Fi '+S.rssi+' dBm"><i class="dot '+(S.rssi>-75?'ok':'warn')+'"></i><span>'+esc(S.ssid)+'</span></span>';
 $('info').innerHTML=tile(T.wifi,esc(S.ssid)+sig(S.rssi),sq+' · '+S.rssi+' dBm')+tile(T.ip,S.ip,S.host+'.local')+tile(T.sd,S.sd?T.sdOk:T.sdNo)+tile(T.heap,Math.round(S.heap/1024)+' KB')+tile(T.up,S.up)+tile(T.fw,esc(S.fw));
 $('lang').value=S.lang;$('start').value=S.start;$('idle').value=S.idle;$('qrOn').checked=S.qrOn;
 $('c24').value=S.c24?1:0;$('datef').value=S.datef;$('greet').checked=S.greet;$('name').value=S.name;
 $('wxCity').textContent=S.wx.city||'—';$('wxNow').textContent=(S.wx.manual?T.manual:T.auto)+' · '+S.wx.lat.toFixed(2)+', '+S.wx.lon.toFixed(2);$('tempF').value=S.tempF?1:0;
 if(qrEd>=S.qrList.length)qrEd=S.qrSel;drawQr();qrFill(qrEd);
 if(cdEd>=S.cd.length)cdEd=-1;drawCd();if(!cdFilled){cdFill(cdEd);cdFilled=1}
 $('host').value=S.host;
 $('wpState').textContent=S.wall?T.wpHas:T.wpNone;
 $('theme').value=S.theme;$('phS').value=S.phS;$('fx').checked=S.fx;$('lunar').checked=S.lunar;$('quote').checked=S.quote;$('idleM').value=S.idleM;$('vol').value=S.vol;$('volV').textContent=S.vol+'%';
 fillHours('ambS',S.ambS);fillHours('ambE',S.ambE);
 const g=S.dragon;$('dgInfo').innerHTML=dragon(g,1);$('ovDg').innerHTML=dragon(g,0);
 if(document.activeElement!==$('dgNameIn'))$('dgNameIn').value=g.ok?g.name:'';
 $('saveDg').disabled=$('dgReset').disabled=!g.ok;
 order=S.apps.map(a=>({id:a.id,name:a.name,on:a.on,lock:a.lock}));drawApps();segs();wpDimV()}
function fillHours(id,v){const s=$(id);s.innerHTML=(id=='ambS'?'<option value=-1>'+T.schedOff:'')+Array.from({length:24},(_,h)=>'<option value='+h+'>'+h+':00').join('');s.value=v}
let cdEd=-1,cdFilled=0;  // event being edited: -1 = a new one
function cdWhen(e){return e.left==null?'':e.left==0?T.cdToday:e.left==1?T.cdTomorrow:e.left<0?T.cdPast:T.cdLeft+e.left+T.cdDays}
function cdDate(e){return e.d+'/'+e.m+(e.k==0?'/'+e.y:e.k==2?' ('+T.cdLunar.replace(/^.*\(|\)$/g,'')+')':'')}
function drawCd(){const L=S.cd;$('cdLst').innerHTML=L.length?L.map((e,i)=>'<div class=it><span class=n>'+esc(e.n)+'<small>'+cdDate(e)+' · '+cdWhen(e)+(e.h?' · '+T.cdOnHome:'')+'</small></span>'+
 '<button class="btn sec" data-ed='+i+'>'+T.qrEdit+'</button><button class="btn sec ib" data-rm='+i+' aria-label=delete>'+ic('del')+'</button></div>').join(''):'<div class=empty>'+T.cdNone+'</div>';
 $('cdNew').disabled=L.length>=12;
 $('cdLst').querySelectorAll('[data-ed]').forEach(b=>b.onclick=()=>{cdEd=+b.dataset.ed;cdFill(cdEd);$('cdEdTitle').scrollIntoView({behavior:'smooth',block:'center'})});
 $('cdLst').querySelectorAll('[data-rm]').forEach(b=>b.onclick=async()=>{if(confirm(T.cdDelAsk)&&await post('/api/cd/delete',{idx:b.dataset.rm})){cdEd=-1;cdFilled=0;toast(T.saved);load()}})}
function cdKind(){const k=+$('cdK').value;$('cdYf').style.display=k==0?'':'none';$('cdLunHint').style.display=k==2?'':'none';
 const dm=k==2?30:31,v=+$('cdD').value||1;$('cdD').innerHTML=Array.from({length:dm},(_,i)=>'<option>'+(i+1)).join('');$('cdD').value=Math.min(v,dm)}
function cdFill(i){const now=new Date(),e=i<0?{n:'',k:0,d:now.getDate(),m:now.getMonth()+1,y:now.getFullYear(),h:1}:S.cd[i];
 $('cdEdTitle').textContent=i<0?T.cdEdNew:T.cdEdEdit;$('cdN').value=e.n;$('cdK').value=e.k;
 $('cdM').innerHTML=Array.from({length:12},(_,i)=>'<option>'+(i+1)).join('');$('cdD').innerHTML='';$('cdD').dataset.v=e.d;cdKind();$('cdD').value=e.d;
 $('cdM').value=e.m;$('cdY').value=e.y||now.getFullYear();$('cdH').checked=!!e.h}
$('cdK').onchange=cdKind;
$('cdNew').onclick=()=>{cdEd=-1;cdFill(-1);$('cdN').focus()};
$('saveCd').onclick=()=>{const n=$('cdN').value.trim();if(!n){toast(T.cdNeedName,1);return $('cdN').focus()}
 busy($('saveCd'),async()=>{const r=await post('/api/cd/save',{idx:cdEd,n:n,k:$('cdK').value,d:$('cdD').value,m:$('cdM').value,y:$('cdY').value,h:$('cdH').checked?1:0});
 if(r){cdEd=r.idx;toast(T.saved);load()}})};
function qrLen(){$('qrLen').textContent=$('qrP').value.trim().length+'/500'}
let qrEd=0;  // account being edited: -1 = a new one
function drawQr(){const L=S.qrList;$('qrLst').innerHTML=L.map((a,i)=>'<div class=it><span class=n>'+esc(a.n||a.t||'#'+(i+1))+'<small>'+esc(a.a)+(i==S.qrSel?' · '+T.qrInUse:'')+'</small></span>'+
 (i==S.qrSel?'':'<button class="btn sec" data-use='+i+'>'+T.qrUse+'</button>')+'<button class="btn sec" data-ed='+i+'>'+T.qrEdit+'</button>'+
 (L.length>1?'<button class="btn sec ib" data-rm='+i+' aria-label=delete>'+ic('del')+'</button>':'')+'</div>').join('');
 $('qrNew').disabled=L.length>=8;
 $('qrLst').querySelectorAll('[data-use]').forEach(b=>b.onclick=async()=>{if(await post('/api/qr/select',{idx:b.dataset.use})){toast(T.saved);load()}});
 $('qrLst').querySelectorAll('[data-ed]').forEach(b=>b.onclick=()=>{qrEd=+b.dataset.ed;qrFill(qrEd);$('qrEdTitle').scrollIntoView({behavior:'smooth',block:'center'})});
 $('qrLst').querySelectorAll('[data-rm]').forEach(b=>b.onclick=async()=>{if(confirm(T.qrDelAsk)&&await post('/api/qr/delete',{idx:b.dataset.rm})){qrEd=0;toast(T.saved);load()}})}
function qrFill(i){const a=i<0?{t:'',b:'',n:'',a:'',p:''}:S.qrList[i];$('qrEdTitle').textContent=i<0?T.qrEdNew:T.qrEdEdit;
 $('qrT').value=a.t;$('qrB').value=a.b;$('qrN').value=a.n;$('qrA').value=a.a;$('qrP').value=a.p;qrLen()}
$('qrNew').onclick=()=>{qrEd=-1;qrFill(-1);$('qrT').focus()}
$('qrP').oninput=qrLen;$('vol').oninput=()=>$('volV').textContent=$('vol').value+'%';
$('saveLook').onclick=()=>saveCfg({phS:$('phS').value,theme:$('theme').value,fx:$('fx').checked?1:0,lunar:$('lunar').checked?1:0,quote:$('quote').checked?1:0});
$('saveAmb').onclick=()=>saveCfg({idleM:$('idleM').value,ambS:$('ambS').value,ambE:$('ambE').value,vol:$('vol').value});
$('saveLang').onclick=()=>saveCfg({lang:$('lang').value});
$('saveStart').onclick=()=>saveCfg({start:$('start').value,idle:$('idle').value,qrOn:$('qrOn').checked?1:0});
$('saveClock').onclick=()=>saveCfg({c24:$('c24').value,datef:$('datef').value,greet:$('greet').checked?1:0,name:$('name').value});
$('saveUnit').onclick=()=>saveCfg({tempF:$('tempF').value});
$('saveQr').onclick=()=>busy($('saveQr'),async()=>{const r=await post('/api/qr/save',{idx:qrEd,t:$('qrT').value,b:$('qrB').value,n:$('qrN').value,a:$('qrA').value,p:$('qrP').value.trim()});if(r){qrEd=r.idx;toast(T.saved);load()}});
$('saveApps').onclick=()=>saveCfg({apps:order.filter(a=>a.on||a.lock).map(a=>a.id).join(',')});
$('wxFind').onclick=()=>{const q=$('wxQ').value.trim();if(!q)return $('wxQ').focus();busy($('wxFind'),async()=>{
 try{const r=await (await fetch('https://geocoding-api.open-meteo.com/v1/search?count=6&name='+encodeURIComponent(q)+'&language='+(S.lang==1?'en':'vi'))).json();
  const list=r.results||[];const box=$('wxRes');box.className=list.length?'lst':'';box.innerHTML=list.length?'':'<p class=hint>'+T.noRes+'</p>';
  list.forEach(c=>{const d=document.createElement('button');d.className='res';d.innerHTML=esc(c.name)+'<small>'+esc([c.admin1,c.country].filter(Boolean).join(', '))+'</small>';
   d.onclick=async()=>{if(await post('/api/weather',{lat:c.latitude,lon:c.longitude,city:c.name})){toast(T.wxSaved);box.innerHTML='';box.className='';$('wxQ').value='';setTimeout(load,800)}};box.appendChild(d)})}
 catch(e){toast(T.failed+e.message,1)}})};
$('wxQ').onkeydown=e=>{if(e.key==='Enter')$('wxFind').click()};
$('wxAuto').onclick=()=>busy($('wxAuto'),async()=>{if(await post('/api/weather',{auto:1})){toast(T.saved);setTimeout(load,1500)}});
$('saveHost').onclick=async()=>{const h=$('host').value.trim().toLowerCase();if(!h)return toast(T.needName,1);
 if(await post('/api/host',{h})){toast(T.restarting);setTimeout(()=>location.href='http://'+h+'.local/',12000)}};
$('savePw').onclick=async()=>{if(await post('/api/password',{pw:$('wpw').value})){toast($('wpw').value?T.pwSet:T.pwOff);$('wpw').value=''}};
$('saveDg').onclick=async()=>{if(await post('/api/dragon',{name:$('dgNameIn').value})){toast(T.saved);load()}};
$('dgReset').onclick=async()=>{if(confirm(T.dgConfirm)&&await post('/api/dragon',{reset:1})){toast(T.saved);load()}};

// ---------- wallpaper: crop to 480x320, darken, RGB565 ----------
let wpImg=null;
function wpDimV(){$('wpDimV').textContent=$('wpDim').value+'%'}
function wpDraw(){const c=$('wpPrev'),x=c.getContext('2d');x.fillStyle='#000';x.fillRect(0,0,480,320);if(!wpImg)return;
 const s=Math.max(480/wpImg.width,320/wpImg.height),w=wpImg.width*s,h=wpImg.height*s;x.drawImage(wpImg,(480-w)/2,(320-h)/2,w,h);
 x.fillStyle='rgba(0,0,0,'+($('wpDim').value/100)+')';x.fillRect(0,0,480,320)}
function wpLoad(src){const i=new Image();i.onload=()=>{wpImg=i;wpDraw();$('wpPrev').scrollIntoView({behavior:'smooth',block:'center'})};i.src=src}
$('wpFile').onchange=()=>{const f=$('wpFile').files[0];if(f)wpLoad(URL.createObjectURL(f))};
$('wpDim').oninput=()=>{wpDimV();wpDraw()};
$('wpSet').onclick=()=>{if(!wpImg)return toast(T.wpPick,1);wpDraw();
 const d=$('wpPrev').getContext('2d').getImageData(0,0,480,320).data,buf=new Uint8Array(16+480*320*2);
 buf.set([90,87,80,49,224,1,64,1]);  // "ZWP1", 480, 320
 for(let i=0,o=16;i<d.length;i+=4,o+=2){const v=((d[i]>>3)<<11)|((d[i+1]>>2)<<5)|(d[i+2]>>3);buf[o]=v&255;buf[o+1]=v>>8}
 const fd=new FormData();fd.append('f',new Blob([buf]),'wall.bin');toast(T.wpSending);
 busy($('wpSet'),()=>send('/api/wallpaper',fd,p=>$('wpState').textContent=T.wpSending+' '+p+'%').then(ok=>{if(ok){toast(T.saved);load()}}))};
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
 if(!r.ok){$('sdList').innerHTML='<div class=empty>'+T.sdNoCard+'</div>';$('sdUp').disabled=$('sdMk').disabled=true;return}
 const j=await r.json();sdDir=j.dir;$('sdUp').disabled=$('sdMk').disabled=false;$('sdPath').textContent=sdDir;$('sdFree').textContent=(j.total-j.used)+' MB'+T.sdFree+' / '+j.total+' MB';
 $('sdBar').style.width=(j.total?Math.round(j.used*100/j.total):0)+'%';
 const items=j.items.sort((a,b)=>(b.d-a.d)||a.n.localeCompare(b.n));let h='';
 if(sdDir!='/')h+='<div class="it dir" data-up=1><span class=ico>'+ic('back')+'</span><span class=n>..</span></div>';
 for(const it of items){const p=join(sdDir,it.n),e=encodeURIComponent(p),a=esc(p);
  if(it.d)h+='<div class="it dir" data-dir="'+a+'"><span class=ico>'+ic('dir')+'</span><span class=n>'+esc(it.n)+'</span><button class="btn sec ib" data-del="'+a+'" title="'+T.sdDel+'">'+ic('del')+'</button></div>';
  else h+='<div class=it><span class=ico>'+ic('file')+'</span><span class=n>'+esc(it.n)+'<small>'+human(it.s)+'</small></span>'+(/\.(jpe?g|png|bmp|gif)$/i.test(it.n)?'<button class="btn sec ib" data-bg="'+a+'" title="'+T.sdBg+'">'+ic('img')+'</button>':'')+
   '<a class="btn sec ib" href="/api/sd/get?path='+e+'" download="'+esc(it.n)+'">'+ic('dl')+'</a><button class="btn sec ib" data-del="'+a+'" title="'+T.sdDel+'">'+ic('del')+'</button></div>'}
 $('sdList').innerHTML=h||'<div class=empty>'+T.sdEmpty+'</div>';
 if(h&&!items.length)$('sdList').innerHTML+='<div class=empty>'+T.sdEmpty+'</div>';
 $('sdList').querySelectorAll('[data-dir]').forEach(b=>b.onclick=e=>{if(!e.target.closest('button'))sdLoad(b.dataset.dir)});
 $('sdList').querySelectorAll('[data-up]').forEach(b=>b.onclick=()=>sdLoad(sdDir.replace(/\/[^\/]+\/?$/,'')||'/'));
 $('sdList').querySelectorAll('[data-bg]').forEach(b=>b.onclick=()=>wpLoad('/api/sd/get?path='+encodeURIComponent(b.dataset.bg)));
 $('sdList').querySelectorAll('[data-del]').forEach(b=>b.onclick=async()=>{if(confirm(T.sdDelQ+b.dataset.del+'?')&&await post('/api/sd/delete',{path:b.dataset.del}))sdLoad(sdDir)})}
function shrink(file){return new Promise(res=>{const i=new Image();i.onload=()=>{const s=Math.min(1,480/i.width,320/i.height),c=document.createElement('canvas');
 c.width=Math.round(i.width*s);c.height=Math.round(i.height*s);c.getContext('2d').drawImage(i,0,0,c.width,c.height);
 c.toBlob(b=>res(b?new File([b],file.name.replace(/\.[^.]+$/,'')+'.jpg'):file),'image/jpeg',0.85)};i.onerror=()=>res(file);i.src=URL.createObjectURL(file)})}
function sdPicked(){const n=$('sdFiles').files.length;$('sdPick').textContent=n?n+T.sdPicked:T.sdChoose}
$('sdFiles').onchange=sdPicked;
const dz=$('sdDrop');dz.ondragover=e=>{e.preventDefault();dz.classList.add('ov')};dz.ondragleave=()=>dz.classList.remove('ov');
dz.ondrop=e=>{e.preventDefault();dz.classList.remove('ov');$('sdFiles').files=e.dataTransfer.files;sdPicked()};
$('sdUp').onclick=()=>{const fs=[...$('sdFiles').files];if(!fs.length)return toast(T.sdPickFiles,1);busy($('sdUp'),async()=>{
 let ok=true;for(let k=0;k<fs.length&&ok;k++){let f=fs[k];if($('sdShrink').checked&&/^image\//.test(f.type))f=await shrink(f);
  const fd=new FormData();fd.append('f',f,f.name);
  ok=await send('/api/sd/upload?dir='+encodeURIComponent(sdDir),fd,p=>$('sdProg').textContent=T.sdUploading+(k+1)+'/'+fs.length+': '+f.name+' '+p+'%')}
 $('sdProg').textContent=ok?T.sdDone:'';if(ok)toast(T.sdDone);$('sdFiles').value='';sdPicked();sdLoad(sdDir)})};
$('sdMk').onclick=async()=>{const n=prompt(T.sdNewName);if(n&&await post('/api/sd/mkdir',{path:join(sdDir,n)}))sdLoad(join(sdDir,n))};
go(location.hash.slice(1));wpDraw();load();sdLoad(sdDir);
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
  cdLoad();  // countdown events, with the days left (null while the clock is not set)
  long cdNow = cdToday();
  if (cdNow) cdRefresh(cdNow);
  JsonArray cl = d["cd"].to<JsonArray>();
  for (int i = 0; i < cdCount; i++) {
    JsonObject o = cl.add<JsonObject>();
    o["n"] = cdEv[i].name;
    o["k"] = (int)cdEv[i].kind;
    o["d"] = cdEv[i].d;
    o["m"] = cdEv[i].m;
    o["y"] = cdEv[i].y;
    o["h"] = !cdEv[i].hidden;
    if (cdNow) o["left"] = cdNextJd[i] - cdNow;
    else o["left"] = nullptr;
  }
  d["qrSel"] = qrSel;
  JsonArray ql = d["qrList"].to<JsonArray>();
  for (int i = 0; i < qrCount; i++) {
    JsonObject o = ql.add<JsonObject>();
    o["t"] = qrAccs[i].title;
    o["b"] = qrAccs[i].bank;
    o["n"] = qrAccs[i].name;
    o["a"] = qrAccs[i].acct;
    o["p"] = qrAccs[i].payload;
  }
  JsonObject au = d["audio"].to<JsonObject>();  // what the speaker is doing (sleep sounds + files on the card)
  au["files"] = audioNameCount;
  au["bg"] = audioBgPlaying() ? audioBgPlaying() : "";
  au["left"] = audioBgLeftSec();
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
    if (idx < 0 || idx >= APP_COUNT || seen[idx] || !appInLauncher(idx)) continue;
    seen[idx] = true;
    JsonObject o = apps.add<JsonObject>();
    o["id"] = idx;
    o["name"] = tr(appName(idx));
    o["on"] = true;
    o["lock"] = idx == SETTINGS_APP;
  }
  for (int i = 0; i < APP_COUNT; i++) {
    if (seen[i] || !appInLauncher(i)) continue;
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
  // ---- countdown events ----
  web.on("/api/cd/save", HTTP_POST, []() {
    if (!webAuthOk()) return;
    cdLoad();
    int idx = web.arg("idx").toInt();
    if (idx < 0) {
      if (cdCount >= CD_MAX) return web.send(400, "text/plain", "at most 12 events");
      idx = cdCount++;
    }
    if (idx >= cdCount) return web.send(400, "text/plain", "bad event");
    CdEvent e = {};
    String n = web.arg("n");
    n.trim();
    // cut at a whole UTF-8 character so the name fits its 40 bytes
    int len = min((int)n.length(), (int)sizeof(e.name) - 1);
    while (len > 0 && len < (int)n.length() && ((uint8_t)n[len] & 0xC0) == 0x80) len--;
    memcpy(e.name, n.c_str(), len);
    e.name[len] = 0;
    e.kind = constrain(web.arg("k").toInt(), 0, 2);
    e.m = constrain(web.arg("m").toInt(), 1, 12);
    e.d = constrain(web.arg("d").toInt(), 1, e.kind == CD_LUNAR ? 30 : 31);
    e.y = constrain(web.arg("y").toInt(), 2000, 2099);
    e.hidden = web.arg("h").toInt() ? 0 : 1;
    cdEv[idx] = e;
    cdSave();
    if (cdScr && !cdEdOv) cdBuildList();  // the Countdown app is open on the device: show the change
    web.send(200, "application/json", String("{\"idx\":") + idx + "}");
  });
  web.on("/api/cd/delete", HTTP_POST, []() {
    if (!webAuthOk()) return;
    cdLoad();
    int idx = web.arg("idx").toInt();
    if (idx < 0 || idx >= cdCount) return web.send(400, "text/plain", "bad event");
    for (int i = idx; i < cdCount - 1; i++) cdEv[i] = cdEv[i + 1];
    cdCount--;
    cdSave();
    if (cdScr && !cdEdOv) cdBuildList();
    web.send(200, "application/json", "{}");
  });
  // ---- payment QR accounts ----
  web.on("/api/qr/save", HTTP_POST, []() {
    if (!webAuthOk()) return;
    auto field = [&](const char *k, size_t maxLen) {
      String v = web.arg(k);
      v.trim();
      return v.length() > maxLen ? v.substring(0, maxLen) : v;
    };
    int idx = web.arg("idx").toInt();
    if (idx < 0) {
      if (qrCount >= QR_MAX) return web.send(400, "text/plain", "at most 8 accounts");
      idx = qrCount++;
    }
    if (idx >= qrCount) return web.send(400, "text/plain", "bad account");
    qrAccs[idx] = {field("t", 30), field("b", 38), field("n", 38), field("a", 22), field("p", 500)};
    qrSave();
    if (idx == qrSel) {
      qrApply();
      cfgSave();
      qrRebuild();
    }
    web.send(200, "application/json", String("{\"idx\":") + idx + "}");
  });
  web.on("/api/qr/select", HTTP_POST, []() {
    if (!webAuthOk()) return;
    int idx = web.arg("idx").toInt();
    if (idx < 0 || idx >= qrCount) return web.send(400, "text/plain", "bad account");
    qrSel = idx;
    qrApply();
    qrSave();
    cfgSave();
    qrRebuild();
    web.send(200, "application/json", "{}");
  });
  web.on("/api/qr/delete", HTTP_POST, []() {
    if (!webAuthOk()) return;
    int idx = web.arg("idx").toInt();
    if (idx < 0 || idx >= qrCount || qrCount < 2) return web.send(400, "text/plain", "keep at least one account");
    for (int i = idx; i < qrCount - 1; i++) qrAccs[i] = qrAccs[i + 1];
    qrAccs[--qrCount] = QrAccount();
    if (qrSel == idx) qrSel = 0;
    else if (qrSel > idx) qrSel--;
    qrApply();
    qrSave();
    cfgSave();
    qrRebuild();
    web.send(200, "application/json", "{}");
  });
  web.on("/api/audio/stop", HTTP_POST, []() {
    if (!webAuthOk()) return;
    audioBgStopNow();
    audioFxStop();
    web.send(200, "application/json", "{}");
  });
  web.on("/api/audio/play", HTTP_POST, []() {  // dev: /api/audio/play?name=rain&min=0 (background) or &fx=1 (one-shot)
    if (!webAuthOk()) return;
    bool ok = web.hasArg("fx") ? audioFx(web.arg("name").c_str(), false) : audioBg(web.arg("name").c_str(), web.arg("min").toInt());
    web.send(ok ? 200 : 404, "text/plain", ok ? "ok" : "no such sound");
  });
  web.on("/api/radio", HTTP_ANY, []() {  // ?ch=0..5 plays a channel, ?stop=1 stops; answers the state
    if (!webAuthOk()) return;
    radioRelayLoad();
    if (web.hasArg("relay")) radioRelaySet(web.arg("relay"));  // "ip:port" of tools/radio_relay.py, "" = direct
    if (web.hasArg("stop")) radioStop();
    else if (web.hasArg("ch")) radioPlay(web.arg("ch").toInt());
    char b[300];
    snprintf(b, sizeof(b), "{\"ch\":%d,\"state\":%d,\"playing\":%s,\"err\":\"%s\",\"heap\":%u,\"heap8\":%u,\"big8\":%u,\"iram\":%u,\"fill\":%u,\"relay\":\"%s\"}",
             radioWant, radioState, aRadioPlaying ? "true" : "false", radioErr, (unsigned)ESP.getFreeHeap(),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_32BIT) - heap_caps_get_free_size(MALLOC_CAP_8BIT)),
             (unsigned)(aRadioHead - aRadioTail), radioRelay);
    web.send(200, "application/json", b);
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
    // row buffers live only for the request (they used to sit in RAM permanently)
    uint16_t *line = (uint16_t *)malloc(W * sizeof(uint16_t));
    uint8_t *out = (uint8_t *)malloc(row);
    if (!line || !out) {
      free(line);
      free(out);
      return web.send(503, "text/plain", "out of memory");
    }
    web.setContentLength(size);
    web.send(200, "image/bmp", "");
    web.sendContent((const char *)h, 54);
    for (int y = H - 1; y >= 0; y--) {
      tftIdle();  // the last DMA strip of LVGL's frame may still hold the bus
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
    free(line);
    free(out);
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
          audioDirty = true;  // a sound file may have arrived
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
    audioDirty = true;
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
