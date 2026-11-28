/*
 * WebPages.h
 * Web page texts for esolard: styles, HTML and JavaScript of the dashboard (/) and the setup page (/config).
 * Included by esolard.c; every page is one string.
 */
#ifndef WEBPAGES_H
#define WEBPAGES_H

static const char UI_BASE_CSS[] = R"UIBASE(
:root{--bg:#f0f3ee;--surf:#fff;--ink:#14211b;--mut:#5a6a61;--line:#d2dad3;--board:#0d3a2e;--pick:#0d3a2e;--on:#d9341f;--amb:#e59a00;--focus:#1a6fd6;--hh:calc(76px + env(safe-area-inset-top,0px));color-scheme:light dark}
@media(prefers-color-scheme:dark){:root{--bg:#09130f;--surf:#102019;--ink:#e6eee9;--mut:#95a79c;--line:#243a2f;--board:#0a2a21;--pick:#2b7b5e;--on:#ff5a45;--amb:#ffb62e;--focus:#6aa9ff}}
*{box-sizing:border-box}
html{scroll-behavior:smooth}
body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,"Helvetica Neue",sans-serif}
a{color:inherit}
.top{position:sticky;top:0;z-index:30;height:var(--hh);background:var(--board);color:#eaf3ee;display:flex;align-items:center;gap:10px;padding:env(safe-area-inset-top,0px) 20px 0}
.top .brand{flex:1;min-width:0}
.top b{display:block;font-size:26px;letter-spacing:-.015em;line-height:1.15;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.top span{font-size:13px;opacity:.75}
.ghost{background:none;border:1px solid rgba(255,255,255,.35);color:inherit;border-radius:6px;padding:7px 12px;font:inherit;font-size:14px;text-decoration:none;cursor:pointer}
.ghost:hover{background:rgba(255,255,255,.1)}
.sh{border-bottom:2px solid var(--ink);padding-bottom:8px}
.sh.g{margin-top:30px}
.sh h2{font-size:26px;line-height:1.15;letter-spacing:-.015em;margin:0 0 4px}
.sh h3{font-size:15px;margin:0}
.lead{color:var(--mut);margin:0;max-width:62ch}
.lead a{text-decoration-thickness:1px}
:focus-visible{outline:2px solid var(--focus);outline-offset:2px}
.btn{display:inline-block;font:inherit;font-size:14px;font-weight:600;padding:8px 14px;border-radius:6px;border:1px solid var(--line);background:var(--surf);color:var(--ink);cursor:pointer;text-decoration:none;white-space:nowrap}
.btn:hover{border-color:var(--ink)}
.btn.pri{background:var(--on);border-color:var(--on);color:#fff}
.pill{display:inline-flex;align-items:center;gap:8px;font-size:14px;font-weight:600}
.pill::before{content:"";width:10px;height:10px;border-radius:50%;background:var(--amb)}
.pill.ok::before{background:#1f9d62}
.pill.n::before{background:var(--mut)}
.foot{color:var(--mut);font-size:13.5px;margin:18px 0 0}
@media(prefers-reduced-motion:reduce){*{transition:none!important;scroll-behavior:auto!important}}
)UIBASE";

static const char SETUP_CSS[] = R"SETUPCSS(
.top .brand>span{display:block;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;opacity:1;color:rgba(234,243,238,.75);line-height:22px}
body{padding-bottom:90px}
.shell{max-width:1000px;margin:0 auto;padding:0 20px;display:grid;grid-template-columns:200px minmax(0,1fr);gap:40px}
nav{position:sticky;top:var(--hh);align-self:start;padding:28px 0;display:flex;flex-direction:column}
nav a{padding:6px 12px;border-left:3px solid var(--line);text-decoration:none;color:var(--mut);font-size:15px}
nav a:hover{color:var(--ink)}
nav a.on{border-color:var(--on);color:var(--ink);font-weight:600}
#f{min-width:0;padding-bottom:40px}
section{padding-top:32px;scroll-margin-top:calc(var(--hh) + 12px)}
.row{display:grid;grid-template-columns:minmax(0,1fr) minmax(150px,280px);gap:6px 24px;align-items:center;padding:12px 0;border-bottom:1px solid var(--line);transition:opacity .15s}
.row.inl{grid-template-columns:minmax(0,1fr) auto}
.lab{display:block}
.lab b{display:block;font-weight:600}
.lab small{display:block;color:var(--mut);font-size:13.5px;line-height:1.35;margin-top:1px}
.ctl{display:flex;align-items:center;gap:8px;justify-content:flex-end;min-width:0}
.ctl>*{min-width:0}
.u{color:var(--mut);font-size:14px;white-space:nowrap;display:inline-block;min-width:24px}
input[type=text],input[type=number]{font:inherit;width:100%;padding:9px 10px;border:1px solid var(--line);border-radius:6px;background:var(--surf);color:inherit}
input[type=number]{max-width:130px}
input.mono{font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;font-size:14px}
input[type=password]{font:inherit;width:100%;padding:9px 10px;border:1px solid var(--line);border-radius:6px;background:var(--surf);color:inherit}
.sw{position:relative;display:inline-block;width:46px;height:26px;flex:none}
.sw input{position:absolute;inset:0;width:100%;height:100%;margin:0;opacity:0;cursor:pointer;z-index:1}
.sw i{position:absolute;inset:0;border-radius:13px;background:var(--line);transition:background .15s}
.sw i::after{content:"";position:absolute;top:3px;left:3px;width:20px;height:20px;border-radius:50%;background:#fff;box-shadow:0 1px 2px rgba(0,0,0,.35);transition:transform .15s}
.sw input:checked+i{background:var(--on)}
.sw input:checked+i::after{transform:translateX(20px)}
.sw input:focus-visible+i{outline:2px solid var(--focus);outline-offset:2px}
.btn.eye{padding:8px 10px;font-weight:500}
.row.dim{opacity:.5}
.blk{padding:14px 0;border-bottom:1px solid var(--line)}
#bar{position:fixed;left:0;right:0;bottom:0;z-index:20;background:var(--board);color:#eaf3ee;display:flex;flex-wrap:wrap;align-items:center;justify-content:center;gap:12px 14px;padding:12px 20px calc(12px + env(safe-area-inset-bottom));transform:translateY(120%);transition:transform .2s}
#bar.show{transform:none}
#bar .msg{display:flex;align-items:center;gap:8px;font-weight:600}
#bar .msg::before{content:"";width:10px;height:10px;border-radius:50%;background:var(--amb)}
#bar .btn{background:none;color:inherit;border-color:rgba(255,255,255,.4)}
#bar .btn.pri{background:var(--on);border-color:var(--on);color:#fff}
#toast{position:fixed;top:calc(var(--hh) + 10px);left:50%;transform:translate(-50%,-160%);background:var(--ink);color:var(--bg);padding:10px 16px;border-radius:8px;z-index:30;opacity:0;visibility:hidden;transition:transform .2s,opacity .2s,visibility .2s;font-weight:600;max-width:90vw}
#toast.show{transform:translate(-50%,0);opacity:1;visibility:visible}
#toast.err{background:var(--on);color:#fff}
#wait{position:fixed;inset:0;background:var(--bg);display:none;place-items:center;text-align:center;z-index:40;padding:24px}
#wait.show{display:grid}
@media(max-width:860px){
.shell{display:block;padding:0}
nav{flex-direction:row;overflow-x:auto;padding:0 10px;background:var(--bg);border-bottom:1px solid var(--line);z-index:10;scrollbar-width:none}
nav a{border-left:0;border-bottom:3px solid transparent;white-space:nowrap;padding:13px 10px}
nav a.on{border-bottom-color:var(--on)}
#f{padding:0 16px 40px}
section{scroll-margin-top:calc(var(--hh) + 52px)}
}
@media(max-width:560px){
.row{grid-template-columns:1fr}
.row.inl{grid-template-columns:minmax(0,1fr) auto}
.ctl{justify-content:flex-start}
input[type=number]{max-width:none}
}
)SETUPCSS";

static const char SETUP_MONITOR_JS[] = R"SETUPMON(
<script>
(function(){
var g=function(i){return document.getElementById(i)},tt;
function toast(t,err){var e=g('toast');e.textContent=t;e.className='show'+(err?' err':'');clearTimeout(tt);tt=setTimeout(function(){e.className=''},err?5000:2200)}
function ls(k){try{return localStorage.getItem(k)==='1'}catch(e){return false}}
function lset(k,v){try{localStorage.setItem(k,v?'1':'0')}catch(e){}}
function lsd(k){try{return localStorage.getItem(k)!=='0'}catch(e){return true}}   // on unless switched off
function gridDefault(){try{var v=localStorage.getItem('showGridChart');
  return v===null?(localStorage.getItem('showImportChart')!=='0'||localStorage.getItem('showExportChart')!=='0'):v!=='0';}catch(e){return true}}
// dashboard parts: [switch, localStorage key, on by default]; defaultChecked too, so Discard doesn't flip them
[['m_flow','showFlow',1],['m_garden','showGarden',1],['m_chart','showChart',1],['m_bchart','showBatChart',1],['m_grid','showGridChart',1],['m_det','showDetails',0]].forEach(function(x){
var c=g(x[0]);c.checked=c.defaultChecked=x[0]==='m_grid'?gridDefault():(x[2]?lsd(x[1]):ls(x[1]));
c.addEventListener('change',function(){lset(x[1],c.checked);c.defaultChecked=c.checked;toast((c.checked?'Shown':'Hidden')+' on the dashboard (this browser)')})});
})();
</script>
)SETUPMON";

static const char SETUP_TAIL[] = R"SETUPTAIL(
<div id="bar" role="region" aria-label="Unsaved changes">
<span class="msg" id="cnt" aria-live="polite">0 changes not saved</span>
<button type="button" class="btn" id="discard">Discard</button>
<button type="submit" form="f" class="btn pri">Save changes</button>
</div>
<div id="toast" role="status"></div>
<div id="wait"><div><h2>Restarting eSolar</h2><p class="lead">This page reloads when the monitor is back.</p></div></div>
<noscript><style>#bar{transform:none}</style></noscript>
<script>
(function(){
var $=function(s,r){return(r||document).querySelector(s)},$$=function(s,r){return Array.prototype.slice.call((r||document).querySelectorAll(s))};
var f=$('#f'),bar=$('#bar'),cnt=$('#cnt'),tl=$('#toast'),base={},busy=false,tt;
var H={'Content-Type':'application/x-www-form-urlencoded'};
function snap(){var m={};new FormData(f).forEach(function(v,k){m[k]=v});return m}
function changed(){var n=0,c=snap(),k;for(k in c)if(c[k]!==base[k])n++;return n}
function update(){var n=changed();bar.classList.toggle('show',n>0);cnt.textContent=n+(n==1?' change':' changes')+' not saved'}
function toast(t,err){tl.textContent=t;tl.className='show'+(err?' err':'');clearTimeout(tt);tt=setTimeout(function(){tl.className=''},err?5000:2200)}
function save(){
if(busy)return Promise.resolve(false);busy=true;
return fetch('/setup',{method:'POST',headers:H,body:new URLSearchParams(new FormData(f)).toString()})
.then(function(r){return r.text().then(function(t){if(!r.ok)throw t||'Could not save.';base=snap();update();toast('Saved');busy=false;return true})})
.catch(function(e){busy=false;toast(typeof e=='string'&&e?e:'Could not save. Check that the monitor is reachable.',1);return false})}
f.addEventListener('input',update);f.addEventListener('change',update);
f.addEventListener('submit',function(e){e.preventDefault();save()});
$('#discard').addEventListener('click',function(){f.reset();update()});
document.addEventListener('keydown',function(e){if((e.ctrlKey||e.metaKey)&&e.key=='s'){e.preventDefault();if(changed())save()}});
window.addEventListener('beforeunload',function(e){if(changed()){e.preventDefault();e.returnValue=''}});
function poll(){fetch('/',{cache:'no-store'}).then(function(r){if(r.ok)location.reload();else setTimeout(poll,2000)}).catch(function(){setTimeout(poll,2000)})}
$('#restart').addEventListener('click',function(){
if(!confirm('Restart eSolar now? The dashboard is unavailable for a few seconds.'))return;
function go(){$('#wait').classList.add('show');fetch('/setup',{method:'POST',headers:H,body:'reboot=1'}).catch(function(){});setTimeout(poll,9000)}
if(changed())save().then(function(ok){if(ok)go()});else go()});
var links=$$('#nav a');
if('IntersectionObserver' in window){
var io=new IntersectionObserver(function(es){es.forEach(function(e){if(!e.isIntersecting)return;
links.forEach(function(a){var on=a.getAttribute('href')=='#'+e.target.id;a.classList.toggle('on',on);
if(on&&a.scrollIntoView&&getComputedStyle(a.parentNode).flexDirection=='row')a.scrollIntoView({inline:'center',block:'nearest'})})})},{rootMargin:'-20% 0px -70% 0px'});
$$('section').forEach(function(s){io.observe(s)})}
base=snap();update();
})();
</script>
</body></html>
)SETUPTAIL";

static const char DASH_CSS[] = R"rawliteral(
:root{--sun:#e59a00;--pos:#1f9d62;--exp:#086b3b;--neg:#d9341f;--bat:#1a6fd6}
@media(prefers-color-scheme:dark){:root{--sun:#ffb62e;--pos:#3ccf8e;--exp:#14874d;--neg:#ff5a45;--bat:#6aa9ff}}
:root{--hh:calc(56px + env(safe-area-inset-top,0px))}
.top b{font-size:20px}
.top .ghost{padding:5px 10px;font-size:13px}
.top .brand{display:grid;grid-template-columns:minmax(0,max-content) auto;column-gap:12px;align-items:center;justify-content:start}
.top .brand>b,.top .brand>span{grid-column:1}
.top .brand>span{display:block;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;opacity:1;color:rgba(234,243,238,.75);font-size:12px;line-height:17px}
#age:not(:empty)::before{content:" \00b7  ";color:rgba(234,243,238,.75);font-weight:400}
#age{overflow:hidden;text-overflow:ellipsis}
#age:empty{display:none}
#age.old{color:#ffb62e;opacity:1;font-weight:600}
#tick{position:relative;grid-column:2;grid-row:1/3;align-self:center;margin-top:3px;width:26px;height:26px;opacity:1;color:#fff}
#tick[hidden]{display:none}
#tick svg{display:block;width:26px;height:26px;transform:rotate(-90deg)}
#tick circle{fill:none;stroke-width:2.4}
#tick .tr{stroke:rgba(255,255,255,.22)}
#tick .tp{stroke:currentColor;stroke-linecap:round;stroke-dasharray:62.83;transition:stroke-dashoffset .3s linear}
#tick b{position:absolute;inset:0;display:grid;place-items:center;font-size:11px;font-weight:700;font-variant-numeric:tabular-nums;color:#fff;opacity:1}
#tick.wait svg{animation:tspin 1s linear infinite}
#tick.wait .tp{stroke-dashoffset:47!important;transition:none}
@keyframes tspin{to{transform:rotate(270deg)}}
@media(prefers-reduced-motion:reduce){#tick.wait svg{animation:none}}
main{max-width:960px;margin:auto;padding:8px 20px 32px}
.pc{--e1:#22a45d;--e2:#3b7ddd;--e3:#e8912d;--u1:#e0a800;--u2:#3b7ddd;--u3:#d9434b;margin:0 0 12px}
@media(prefers-color-scheme:dark){.pc{--e1:#34c46f;--e2:#4c8ff0;--e3:#f2a23c;--u1:#f2c230;--u2:#4c8ff0;--u3:#ef5a5f}}
.pc .pcs{display:grid;grid-template-columns:minmax(0,1fr);gap:20px}@media(min-width:760px){.pc .pcs{grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:32px}}.pc .pcp{min-width:0}
.pc .pcs>.pcp+.pcp{border-top:1px solid var(--line);padding-top:18px}@media(min-width:760px){.pc .pcs>.pcp+.pcp{border-top:0;padding-top:0}}
.pc .pct{font-size:16px;font-weight:600}.pc .pcv{font-size:34px;font-weight:800;line-height:1.15;font-variant-numeric:tabular-nums}
.pc .pcv small{font-size:15px;font-weight:500;color:var(--mut)}
.pc .pcb{display:flex;height:12px;border-radius:6px;overflow:hidden;background:var(--line);margin:8px 0 14px}.pc .pcb i{display:block;height:100%}
.pc .pcb .e1,.pc .dot.e1{background:var(--e1)}.pc .pcb .e2,.pc .dot.e2{background:var(--e2)}.pc .pcb .e3,.pc .dot.e3{background:var(--e3)}
.pc .pcb .u1,.pc .dot.u1{background:var(--u1)}.pc .pcb .u2,.pc .dot.u2{background:var(--u2)}.pc .pcb .u3,.pc .dot.u3{background:var(--u3)}
.pc .pcr{display:flex;align-items:center;gap:16px}.pc svg.dn{width:88px;height:88px;flex:none}
.pc .dn circle{fill:none;stroke-width:11}.pc .dn .dn0{stroke:var(--line)}
.pc .dn .e1{stroke:var(--e1)}.pc .dn .e2{stroke:var(--e2)}.pc .dn .e3{stroke:var(--e3)}.pc .dn .u1{stroke:var(--u1)}.pc .dn .u2{stroke:var(--u2)}.pc .dn .u3{stroke:var(--u3)}
.pc .pcl{flex:1;min-width:0}.pc .li{display:flex;align-items:center;gap:7px;padding:4px 0;font-size:14px;white-space:nowrap}
.pc .dot{width:10px;height:10px;border-radius:50%;flex:none}
.pc .ic{width:16px;height:16px;flex:none;fill:none;stroke:var(--mut);stroke-width:1.3;stroke-linecap:round;stroke-linejoin:round}
.pc .li .n{flex:1;color:var(--mut);overflow:hidden;text-overflow:ellipsis}.pc .li b{font-variant-numeric:tabular-nums}
.pc .li .pp{color:var(--mut);width:46px;text-align:right;font-size:13px}
.pc .pcf{display:flex;align-items:center;gap:6px;margin-top:12px;font-size:14px;color:var(--mut);flex-wrap:wrap}.pc .pcf b{color:var(--ink)}
@media(max-width:520px){.pc svg.dn{display:none}.pc .li{font-size:16px;gap:8px;padding:6px 0}.pc .li b{font-size:16px}.pc .li .pp{width:48px;font-size:14px}.pc .li .n{color:var(--ink)}}
.card{background:var(--surf);border:1px solid var(--line);border-radius:6px;padding:12px 14px}
.l{color:var(--mut);font-size:13px;font-weight:600}
.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:10px}
.card h2{font-size:15px;margin:0 0 8px;padding-bottom:6px;border-bottom:2px solid var(--ink)}
table{width:100%;border-collapse:collapse}
td{padding:5px 0;border-top:1px solid var(--line)}
tr:first-child td{border-top:0}
td:last-child{text-align:right;font-variant-numeric:tabular-nums}
.err{color:var(--neg);margin:0}
.chart svg{display:block;width:100%;height:auto;margin-top:6px}
.chart .gl{stroke:var(--line);stroke-width:1}
.chart .ax{stroke:var(--mut);stroke-width:1.5}
.chart .now{stroke:var(--ink);stroke-width:1}
.chart .nr{fill:var(--surf);stroke:var(--sun);stroke-width:1.2}.chart .nr.bt{stroke:var(--bat)}
.chart text{fill:var(--mut);font-size:12px;font-family:inherit}
.chart path.a{stroke:none;fill-opacity:.35}
.chart path.o{fill:none;stroke-width:1;stroke-linejoin:round;stroke-linecap:round}
.chart path.a.cp{fill:var(--pos)}.chart path.a.cc{fill:var(--sun)}.chart path.a.cf{fill:var(--bat)}
@supports (fill:color-mix(in srgb,red,blue)){.chart path.a{fill-opacity:1}
.chart path.a.cp{fill:color-mix(in srgb,var(--pos) 38%,var(--surf))}.chart path.a.cc{fill:color-mix(in srgb,var(--sun) 40%,var(--surf))}
.chart path.a.cf{fill:color-mix(in srgb,var(--bat) 42%,var(--surf))}}
.chart path.o.cp{stroke:var(--pos)}.chart path.o.cc{stroke:var(--sun)}.chart path.o.cf{stroke:var(--bat)}
.chart .ch{display:flex;align-items:center;justify-content:space-between;gap:8px}
.chart .ch .l{flex:1;min-width:0;text-align:center;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.chart .cht{text-align:center;margin-top:6px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.chart .lgd{flex-direction:column;flex-wrap:nowrap;gap:4px}.chart .lgd span{white-space:nowrap}
.chart text.cm{font-size:14px;fill:var(--mut)}
.chart .hb{width:32px;height:28px;border-radius:8px;border:1px solid var(--line);background:var(--surf);color:var(--ink);font-size:20px;line-height:1;padding:0 0 3px;cursor:pointer;font-family:inherit}
.chart .hb:disabled{opacity:.3;cursor:default}
.chart .seg{display:inline-flex;flex:none;border:1px solid var(--line);border-radius:8px;overflow:hidden}
.chart .seg button{border:0;background:var(--surf);color:var(--mut);font:inherit;font-size:12px;height:26px;padding:0 5px;cursor:pointer}
.chart .seg button+button{border-left:1px solid var(--line)}
.chart .seg button.on{background:var(--ink);color:var(--surf);font-weight:600}
.chart rect.wb{fill-opacity:.35;stroke-width:1.2}
.chart rect.wb.cp{fill:var(--pos);stroke:var(--pos)}.chart rect.wb.cc{fill:var(--sun);stroke:var(--sun)}.chart rect.wb.cf{fill:var(--bat);stroke:var(--bat)}
.chart rect.wb.ce{fill:var(--exp);stroke:var(--exp);fill-opacity:.75}
.chart rect.wb.ci{fill:var(--neg);stroke:var(--neg);fill-opacity:.6}
@supports (fill:color-mix(in srgb,red,blue)){.chart rect.wb{fill-opacity:1}
.chart rect.wb.cp{fill:color-mix(in srgb,var(--pos) 38%,var(--surf))}.chart rect.wb.cc{fill:color-mix(in srgb,var(--sun) 40%,var(--surf))}
.chart rect.wb.cf{fill:color-mix(in srgb,var(--bat) 42%,var(--surf))}}
.chart rect.wb.ce{fill:var(--exp);fill-opacity:.75;stroke:none}
.chart rect.wb.ci{fill:var(--neg);fill-opacity:.6;stroke:none}
.chart rect.wb.cday{fill:#ff8578;fill-opacity:.75;stroke:none}
.chart rect.wb.cnight{fill:#bd3026;fill-opacity:.75;stroke:none}
.chart .rate-cut{stroke:var(--mut);stroke-width:1;stroke-dasharray:3 2}
.chart .lgd .import-legend{white-space:normal}
.chart .rate-breakdown{display:inline-flex;flex-wrap:wrap;gap:4px 10px}
.chart .rate-breakdown i.day{background:#ff8578}.chart .rate-breakdown i.night{background:#bd3026}
.chart svg[data-cur]{touch-action:pan-y;cursor:crosshair;-webkit-user-select:none;user-select:none}
.chart .cur{pointer-events:none}.chart .cur line{stroke:var(--ink);stroke-width:1;stroke-dasharray:3 3}
.chart .cur .cd{stroke:var(--surf);stroke-width:1}.chart .cur .bx{fill:var(--surf);stroke:var(--line);opacity:.96}
.chart .cur text.ct{fill:var(--ink);font-size:16px}.chart .cur .tb{font-weight:700}
.chart .cur .cp{fill:var(--pos)}.chart .cur .cc{fill:var(--sun)}.chart .cur .cf{fill:var(--bat)}.chart .cur .ce{fill:var(--exp)}.chart .cur .ci{fill:var(--neg)}
.lgd{display:flex;flex-wrap:wrap;gap:6px 20px;margin-top:8px;font-size:13px;color:var(--mut)}
.lgd b{color:var(--ink);font-weight:600}
.lgd i{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:6px;vertical-align:-1px}
.lgd i.cp{background:var(--pos)}.lgd i.cc{background:var(--sun)}.lgd i.cf{background:var(--bat)}.lgd i.ce{background:var(--exp)}.lgd i.ci{background:var(--neg)}
.card.flow{padding:0;overflow:hidden;margin:16px 0 10px}
.flow svg{display:block;width:100%;aspect-ratio:600/415;max-height:450px}
.flow{--sky1:#d6ecf8;--sky2:#f2f9fd;--gnd:#e9edf0;--pv:#3f63d3;--win:#f8e6a0;--wof:#c3cee0;
  --hw1:#b9c6dd;--hw2:#dde5f1;--hrf:#2a3557;--hol:#1d2742;--trk:#7a5536;--tl1:#4fae5c;--tl2:#3a8f4a;--tlh:#7cc983;--wxc:#ffffff;--wxc2:#aab8c8;--wxm0:#b4c4d6;--wxst:#ffffff;--wxsn:#ffffff;
  --gb1:#7cbf55;--gb2:#4f9a45;--fpt:#d567ec;--fct:#ffd84a;--fn1:#e2b66b;--fn2:#a8783a}
@media(prefers-color-scheme:dark){.flow{--sky1:#123542;--sky2:#0e2530;--gnd:#0b1813;--pv:#3d62cf;--win:#f3dc93;--wof:#2c3a5c;
  --hw1:#2b3c69;--hw2:#46598f;--hrf:#0d1430;--hol:#070b1c;--trk:#4a3524;--tl1:#2f6b45;--tl2:#245738;--tlh:#3f8456;--wxc:#8496ad;--wxc2:#5c6c82;--wxm0:#3a5068;--wxst:#e9eef7;--wxsn:#e9eef7;
  --gb1:#4d8a3f;--gb2:#336b33;--fpt:#c45ee0;--fct:#f2cc3c;--fn1:#c99a52;--fn2:#7a5428}}
.flow .s1{stop-color:var(--sky1)}.flow .s2{stop-color:var(--sky2)}
.flow .gnd{fill:var(--gnd)}.flow .cl{fill:var(--surf);opacity:.55}
.flow .nd{fill:var(--surf);stroke:var(--line);stroke-width:4}
.flow .nd.sun{stroke:var(--sun)}.flow .nd.imp{stroke:var(--neg)}.flow .nd.exp{stroke:var(--pos)}
.flow .trk{fill:var(--trk)}.flow .trb{stroke:var(--trk);stroke-width:3;stroke-linecap:round;fill:none}
.flow .tl1{fill:var(--tl1)}.flow .tl2{fill:var(--tl2)}.flow .tlh{fill:var(--tlh)}
.flow .tree{transform-box:fill-box;transform-origin:50% 100%;animation:sway 6s ease-in-out infinite}
@keyframes sway{0%,100%{transform:rotate(-1.2deg)}50%{transform:rotate(1.2deg)}}
.flow .fn{fill:var(--fn1);stroke:var(--fn2);stroke-width:1}.flow .fnn{fill:var(--fn2)}
.flow .gb1{fill:var(--gb1)}.flow .gb2{fill:var(--gb2)}.flow .fst{fill:none;stroke:var(--gb2);stroke-width:1.5}
.flow .fpt{fill:var(--fpt);stroke:var(--surf);stroke-width:.6}.flow .fpb{fill:var(--fpt)}.flow .fct{fill:var(--fct)}
.flow .fwr{transform-box:fill-box;transform-origin:50% 100%;animation:sway 4s ease-in-out infinite}
@media(prefers-reduced-motion:reduce){.flow .tree,.flow .fwr{animation:none}}
.flow .hw1{fill:var(--hw1)}.flow .hw2{fill:var(--hw2)}.flow .hrf{fill:var(--hrf)}
.flow .ho{fill:none;stroke:var(--hol);stroke-width:4;stroke-linejoin:round;stroke-linecap:round}
.flow .pv{fill:var(--pv)}.flow .pvl{fill:none;stroke:var(--hrf);stroke-width:2}
.flow .win{fill:var(--wof)}.flow .win.lit{fill:var(--win)}.flow .wgl{fill:#fff;opacity:.45}
.flow .wfr{fill:none;stroke:var(--hol);stroke-width:3;stroke-linejoin:round}
.flow .inv{fill:#e8eef9;stroke:var(--hol);stroke-width:3}.flow .wave{fill:none;stroke:var(--sun);stroke-width:2.5;stroke-linecap:round}
.flow .py{fill:none;stroke:var(--mut);stroke-width:2;stroke-linejoin:round}
.flow .bt{fill:var(--surf);stroke:var(--mut);stroke-width:2}.flow .btf{fill:var(--bat);opacity:.8}.flow .btc{fill:var(--mut)}
.flow .wxs{fill:var(--sun)}.flow .wxr{stroke:var(--sun);stroke-width:2.5;stroke-linecap:round}
.flow .wxc{fill:var(--wxc)}.flow .wxc2{fill:var(--wxc2)}.flow .wxm{fill:#f4e9b8}.flow .wxm0{fill:var(--wxm0)}
.flow .wxst{fill:var(--wxst)}.flow .wxd{stroke:var(--bat);stroke-width:2.5;stroke-linecap:round}
.flow .wxn{fill:var(--wxsn);stroke:var(--wxc2);stroke-width:.8}.flow .wxb{fill:var(--sun)}
.flow .wxf{stroke:var(--wxc2);stroke-width:3.5;stroke-linecap:round}
.flow .wxt{fill:var(--ink);font-size:20px;font-weight:700}.flow .wxl{fill:var(--mut);font-size:13px}
.flow .fw{stroke:var(--line);stroke-width:3;stroke-linecap:round;fill:none}
.flow .fw.on{stroke-width:5;stroke-dasharray:1 11;animation:fdot .9s linear infinite}
.flow .fa{fill:none;stroke-width:3.5;stroke-linecap:round;stroke-linejoin:round}
.flow .c-sun{stroke:var(--sun)}.flow .c-imp{stroke:var(--neg)}.flow .c-exp{stroke:var(--pos)}.flow .c-bat{stroke:var(--bat)}.flow .c-home{stroke:var(--ink)}
.flow text{fill:var(--ink);font-family:inherit}.flow .fs{fill:var(--mut);font-size:17px;font-weight:600}.flow .fb{font-size:30px;font-weight:700}
.flow .fu{fill:var(--mut);font-size:17px;font-weight:500}.flow .fl{fill:var(--mut);font-size:15px;font-weight:500}
@keyframes fdot{to{stroke-dashoffset:-12}}
@media(prefers-reduced-motion:reduce){.flow .fw.on{animation:none}}
)rawliteral";

static const char INDEX_HTML[] = R"rawliteral(<main id="app"></main>
<script>
const D='\u2013';
function f(v,d){if(v===null||v===undefined)return D;if(typeof v==='string')return v;
  return v.toLocaleString(undefined,{minimumFractionDigits:d,maximumFractionDigits:d});}
function rows(list,o){return '<table>'+list.map(function(r){var v=o[r[1]];
  return '<tr><td>'+r[0]+'</td><td>'+f(v,r[3])+(v==null||r[2]===''?'':' '+r[2])+'</td></tr>';}).join('')+'</table>';}
const INV=[['Status','status_text','',0],['AC power','ac_power_w','W',0],['DC power','dc_power_w','W',0],['Efficiency','efficiency_pct','%',1],
 ['Voltage L1','ac_voltage_l1_v','V',1],['Voltage L2','ac_voltage_l2_v','V',1],['Voltage L3','ac_voltage_l3_v','V',1],
 ['AC current','ac_current_a','A',2],['Frequency','frequency_hz','Hz',2],['DC voltage','dc_voltage_v','V',1],['DC current','dc_current_a','A',2],
 ['Heatsink','heatsink_temp_c','\u00b0C',1],['Energy total','energy_total_kwh','kWh',1]];
const MET=[['Power','power_w','W',0],['Power L1','power_l1_w','W',0],['Power L2','power_l2_w','W',0],['Power L3','power_l3_w','W',0],
 ['Voltage L1','voltage_l1_v','V',1],['Voltage L2','voltage_l2_v','V',1],['Voltage L3','voltage_l3_v','V',1],
 ['Current','current_a','A',2],['Frequency','frequency_hz','Hz',2],['Exported','energy_exported_kwh','kWh',1],['Imported','energy_imported_kwh','kWh',1]];
const BAT=[['Status','status_text','',0],['Charge level','soc_pct','%',1],['Power','power_w','W',0],['Voltage','voltage_v','V',1],['Current','current_a','A',2],
 ['Available energy','available_energy_kwh','kWh',2],['Max energy','max_energy_kwh','kWh',2],['Health','soh_pct','%',1],
 ['Temperature','temp_avg_c','\u00b0C',1],['Lifetime export','lifetime_export_kwh','kWh',1],['Lifetime import','lifetime_import_kwh','kWh',1]];
// Countdown ring: full right after a reading, empty when the next one is due, spinning while it is being read
var C=62.83,lastAt=0,period=20,live=false,lastLoad=0;
function msg(t,old){var a=document.getElementById('age');a.textContent=t;a.className=old?'old':'';
  document.getElementById('tick').hidden=!!t;if(t)live=false;}
function tick(){
  if(!live)return;
  var k=document.getElementById('tick'),n=document.getElementById('tickn'),rem=period-(Date.now()-lastAt)/1000;
  if(rem>0){k.className='';k.querySelector('.tp').style.strokeDashoffset=(C*(1-rem/period)).toFixed(2);n.textContent=Math.ceil(rem);}
  else{k.className='wait';n.textContent='';if(Date.now()-lastLoad>1000)load();}   // due: check every second
  var t=rem>0?'Next update in '+Math.ceil(rem)+' s':'Reading the inverter\u2026';
  k.title=t;k.setAttribute('aria-label',t);
}
setInterval(tick,250);
// the dashboard parts are switched on the setup page (Monitor) and remembered in this browser
var showDetails=false,showChart=true,showBatChart=true,showGridChart=true,showFlow=true,showGarden=true,lastData=null,hist=null,histAt=0,hView=0,hOld={};
var wx=null;                                   // weather now, from /json
function gridDefault(){try{var v=localStorage.getItem('showGridChart');
  return v===null?(localStorage.getItem('showImportChart')!=='0'||localStorage.getItem('showExportChart')!=='0'):v!=='0';}catch(e){return true}}
try{showDetails=localStorage.getItem('showDetails')==='1';showGarden=localStorage.getItem('showGarden')!=='0';
  showChart=localStorage.getItem('showChart')!=='0';showBatChart=localStorage.getItem('showBatChart')!=='0';showGridChart=gridDefault();showFlow=localStorage.getItem('showFlow')!=='0';}catch(e){}
// Power flow picture (switched on the setup page, on by default). Moving dots + an arrow only on the
// lines where power flows, in its direction: sun -> inverter, inverter -> home, grid <-> inverter, battery <-> inverter.
var FLOW_MIN=20;                                                       // W: below this a line is idle
// Weather now: esolard fetches it from Open-Meteo every 15 minutes and sends it in /json ("weather")
function wxCheck(d){
  wx=d.weather&&d.weather.code!=null?{t:d.weather.temp_c,c:d.weather.code,day:!!d.weather.is_day}:null;
}
// weather icon top right of the power flow picture: sun or moon (with today's phase), clouds, rain, snow, fog, thunder
function wxIcon(night){
  var x=433,y=30,v='',c=wx?wx.c:0,lab='';
  function sun(x,y,r){var p='';for(var a=0;a<8;a++){var t=a*Math.PI/4,c1=Math.cos(t),s1=Math.sin(t);
      p+='M'+(x+c1*(r+4)).toFixed(1)+','+(y+s1*(r+4)).toFixed(1)+'L'+(x+c1*(r+8)).toFixed(1)+','+(y+s1*(r+8)).toFixed(1);}
    return '<circle class="wxs" cx="'+x+'" cy="'+y+'" r="'+r+'"/><path class="wxr" d="'+p+'"/>';}
  function moon(x,y,r){                                    // lit part from the moon's age (northern hemisphere)
    var age=((Date.now()-Date.UTC(2000,0,6,18,14))/864e5)%29.530589,ph=age/29.530589,k=Math.cos(2*Math.PI*ph),
        rx=(Math.abs(k)*r).toFixed(1),t=x+','+(y-r),b=x+','+(y+r),
        d=ph<.5?'M'+t+'A'+r+','+r+' 0 0 1 '+b+'A'+rx+','+r+' 0 0 '+(k>0?0:1)+' '+t
               :'M'+t+'A'+r+','+r+' 0 0 0 '+b+'A'+rx+','+r+' 0 0 '+(k>0?1:0)+' '+t;
    return '<circle class="wxm0" cx="'+x+'" cy="'+y+'" r="'+r+'"/><path class="wxm" d="'+d+'Z"/>';}
  function sky(x,y,r){return night?moon(x,y,r):sun(x,y,r);}
  function cloud(x,y,k){return '<g class="'+(k||'wxc')+'"><circle cx="'+(x-9)+'" cy="'+(y+2)+'" r="8"/><circle cx="'+x+'" cy="'+(y-4)+'" r="11"/>'+
    '<circle cx="'+(x+11)+'" cy="'+(y+3)+'" r="7"/><rect x="'+(x-17)+'" y="'+(y+2)+'" width="35" height="8" rx="4"/></g>';}
  function stars(){return '<circle class="wxst" cx="407" cy="14" r="1.3"/><circle class="wxst" cx="475" cy="10" r="1.1"/><circle class="wxst" cx="485" cy="44" r="1.3"/><circle class="wxst" cx="417" cy="44" r="1"/>';}
  if(!wx){if(!night)return '';return stars()+moon(x,y,12);}   // no location set: only the moon at night
  if(c===0){v=(night?stars():'')+sky(x,y,12);lab='Clear';}
  else if(c<=2){v=(night?stars():'')+sky(x+8,y-7,9)+cloud(x-2,y+4);lab=c===1?'Mostly clear':'Partly cloudy';}
  else if(c===3){v=cloud(x+6,y-6,'wxc2')+cloud(x-3,y+3);lab='Overcast';}
  else if(c===45||c===48){v='<path class="wxf" d="M'+(x-17)+','+(y-8)+'h30M'+(x-12)+','+(y+1)+'h32M'+(x-17)+','+(y+10)+'h28"/>';lab='Fog';}
  else if(c>=95){v=cloud(x,y-4)+'<path class="wxb" d="M'+(x+1)+','+(y+8)+'l-7,12h6l-3,10l10,-14h-6l3,-8z"/>';lab='Thunder';}
  else if((c>=71&&c<=77)||c===85||c===86){v=cloud(x,y-4);[-9,0,9,-4,5].forEach(function(o,i){
      v+='<circle class="wxn" cx="'+(x+o)+'" cy="'+(y+(i<3?12:19))+'" r="2.4"/>';});lab='Snow';}
  else if(c>=51&&c<=82){var n=c<=57?2:3;v=cloud(x,y-4);
    for(var i=0;i<n;i++){var dx=x-8+i*(16/(n-1));v+='<path class="wxd" d="M'+dx+','+(y+10)+'l-3,8"/>';}
    lab=c<=57?'Drizzle':c>=80?'Showers':'Rain';}
  else{v=cloud(x,y);lab='Cloudy';}
  return v+'<text class="wxt" x="489" y="36" text-anchor="end">'+Math.round(wx.t)+'°</text>'+
    '<text class="wxl" x="489" y="60" text-anchor="end">'+lab+'</text>';
}
function productionTotal(d){
  if(!d||!d.p||!d.s)return null;
  var total=0,any=false;
  for(var i=0;i<d.p.length;i++)if(d.s[i]!=null&&d.p[i]!=null){total+=d.p[i];any=true;}
  return any?total*d.step/60000:null;
}
function flow(s){
  var P=s.production_w,Hw=s.house_w,G=s.grid_w,B=s.battery_w,soc=s.battery_soc_pct;
  var prodToday=productionTotal(hist);
  var ph=((Date.now()%900)/1000).toFixed(2);                           // keeps the dots moving smoothly across redraws
  function kwv(w){return w==null?D:f(Math.abs(w)/1000,2);}
  function on(w){return w!=null&&Math.abs(w)>=FLOW_MIN;}
  function ln(x1,y1,x2,y2,dir,c){                                      // dir 1: from (x1,y1) to (x2,y2), -1: back, 0: idle
    if(!dir)return '<line class="fw" x1="'+x1+'" y1="'+y1+'" x2="'+x2+'" y2="'+y2+'"/>';
    if(dir<0){var t=x1;x1=x2;x2=t;t=y1;y1=y2;y2=t;}
    var a=Math.atan2(y2-y1,x2-x1)*180/Math.PI,mx=(x1+x2)/2,my=(y1+y2)/2;
    return '<line class="fw on '+c+'" style="animation-delay:-'+ph+'s" x1="'+x1+'" y1="'+y1+'" x2="'+x2+'" y2="'+y2+'"/>'+
      '<path class="fa '+c+'" d="M-6,-7L3,0L-6,7" transform="translate('+mx+','+my+') rotate('+a+')"/>';
  }
  // layout: the sun stays at the top; house, pylon and grid bubble are moved down by HY, the battery by BY;
  // sun bubble, house and battery are moved right by DX so the sun -> inverter line runs down the middle (x 300)
  var HY=36,BY=64,DX=50,sunOn=on(P)&&P>0,v='<svg viewBox="0 0 600 415" role="img" aria-label="Power flow now">';
  v+='<defs><linearGradient id="fsky" x1="0" y1="0" x2="0" y2="1"><stop offset="0" class="s1"/><stop offset="1" class="s2"/></linearGradient></defs>';
  v+='<rect x="-1000" y="-500" width="2600" height="'+(745+HY)+'" fill="url(#fsky)"/><rect class="gnd" x="-1000" y="'+(245+HY)+'" width="2600" height="1000"/>';
  v+='<ellipse class="cl" cx="480" cy="34" rx="40" ry="12"/><ellipse class="cl" cx="120" cy="140" rx="54" ry="15"/><ellipse class="cl" cx="560" cy="220" rx="40" ry="11"/>';
  // tree left of the house (behind it), swaying gently in the wind
  v+='<g transform="translate(0,'+HY+')"><g class="tree">'+
    '<path class="trk" d="M33,245L35,198Q36,192 38,198L41,245Z"/><path class="trb" d="M37,214L48,200M36,222L26,211"/>'+
    '<circle class="tl2" cx="22" cy="186" r="18"/><circle class="tl2" cx="53" cy="184" r="19"/>'+
    '<circle class="tl1" cx="37" cy="164" r="27"/><circle class="tl1" cx="24" cy="176" r="15"/><circle class="tl1" cx="50" cy="172" r="16"/>'+
    '<circle class="tlh" cx="29" cy="156" r="9"/><circle class="tlh" cx="47" cy="164" r="6"/></g></g>';
  // the fence, the grass and the flowers ("Garden") can be switched off on the setup page (Monitor)
  // wooden fence from the left edge up to the house wall (x 90), in front of the tree: posts, two rails, nails
  if(showGarden){
  v+='<g transform="translate(0,'+HY+')">';
  var fp=[];for(var fx=-130;fx<80;fx+=24)fp.push(fx);fp.push(84);
  fp.forEach(function(x){v+='<rect class="fn" x="'+x+'" y="217" width="6" height="28" rx="1"/>';});
  v+='<rect class="fn" x="-136" y="223" width="226" height="5" rx="1"/><rect class="fn" x="-136" y="234" width="226" height="5" rx="1"/>';
  fp.forEach(function(x){v+='<circle class="fnn" cx="'+(x+3)+'" cy="225.5" r=".9"/><circle class="fnn" cx="'+(x+3)+'" cy="236.5" r=".9"/>';});
  v+='</g>';}
  // flower bed between the house and the pylon (x 368-486), low enough to stay under the grid line (y 206)
  v+='<g transform="translate(0,'+HY+')">';
  var i,gx,gh,gl;
  if(showGarden){
  for(i=0;i<20;i++){gx=368+i*6;gh=13+(i*7)%10;gl=(i*5)%9-4;                         // grass blades
    v+='<path class="'+(i%2?'gb1':'gb2')+'" d="M'+(gx-3)+',245Q'+(gx+gl/2)+','+(245-gh/2)+' '+(gx+gl)+','+(245-gh)+'Q'+(gx+1+gl/2)+','+(245-gh/2)+' '+(gx+3)+',245Z"/>';}
  for(i=0;i<48;i++){gx=488+i*6;gh=11+(i*7)%9;gl=(i*5)%9-4;                          // grass from the flower bed under the pylon to the right edge (no flowers)
    v+='<path class="'+(i%2?'gb1':'gb2')+'" d="M'+(gx-3)+',245Q'+(gx+gl/2)+','+(245-gh/2)+' '+(gx+gl)+','+(245-gh)+'Q'+(gx+1+gl/2)+','+(245-gh/2)+' '+(gx+3)+',245Z"/>';}
  }
  if(showGarden)[[401,225,4.9],[422,231,3.9],[443,224,4.9],[464,230,4.2]].forEach(function(q,k){
    var x=q[0],y=q[1],r=q[2],w='<path class="fst" d="M'+x+',245Q'+(x-2)+','+((245+y)/2)+' '+x+','+y+'"/>';
    for(var a=0;a<5;a++){var t=a*1.2566+k;w+='<circle class="fpt" cx="'+(x+r*Math.cos(t)).toFixed(1)+'" cy="'+(y+r*Math.sin(t)).toFixed(1)+'" r="'+(r*.58).toFixed(1)+'"/>';}
    v+='<g class="fwr" style="animation-delay:-'+k*.7+'s">'+w+'<circle class="fpb" cx="'+x+'" cy="'+y+'" r="'+(r*.75).toFixed(1)+'"/><circle class="fct" cx="'+x+'" cy="'+y+'" r="'+(r*.4).toFixed(1)+'"/></g>';});
  v+='</g>';
  v+='<g transform="translate('+DX+','+HY+')">';
  // house seen at an angle: long side wall (darker) with the roof slope above it, gable end (lighter) on the right;
  // the gable tip sits right under the sun bubble (x 250 + DX) and the inverter hangs on the gable wall
  function rp(u,t){return (30+u*160+t*60).toFixed(1)+','+(180-t*52).toFixed(1);}    // point on the roof slope: u along the eave, t up the slope
  function pan(t0,t1){return 'M'+rp(.08,t0)+'L'+rp(.92,t0)+'L'+rp(.92,t1)+'L'+rp(.08,t1)+'Z';}
  v+='<rect class="hw1" x="40" y="178" width="152" height="67"/>';
  v+='<path class="hw2" d="M190,180L250,128L310,180V245H190Z"/>';
  v+='<path class="hrf" d="M'+rp(0,0)+'L'+rp(1,0)+'L'+rp(1,1)+'L'+rp(0,1)+'Z"/>';
  v+='<path class="pv" d="'+pan(.14,.47)+pan(.55,.88)+'"/>';
  v+='<path class="pvl" d="M'+rp(.36,.14)+'L'+rp(.36,.47)+'M'+rp(.64,.14)+'L'+rp(.64,.47)+'M'+rp(.36,.55)+'L'+rp(.36,.88)+'M'+rp(.64,.55)+'L'+rp(.64,.88)+'"/>';
  v+='<path class="ho" d="M40,180V245H310V180L250,128H90L30,180H190L250,128M190,180V245"/>';
  // sun -> inverter and inverter <-> battery: drawn over the house (roof and bottom wall), under the inverter box
  v+='</g>'+ln(250+DX,122,250+DX,186+HY,sunOn?1:0,'c-sun')+ln(250+DX,226+HY,250+DX,266+BY,on(B)?(B>0?1:-1):0,'c-bat');
  v+='<g transform="translate(0,'+HY+')">'+ln(282+DX,206,494,206,on(G)?(G>0?1:-1):0,G>0?'c-exp':'c-imp')+'</g>';   // grid: + export, - import (pylon stays put)
  v+='<g transform="translate('+DX+','+HY+')">';
  v+=ln(218,206,128,206,on(Hw)&&Hw>0?1:0,'c-home');                                 // inverter -> home
  // window (home), inverter
  var lit=on(Hw)&&Hw>0?' lit':'';                                                 // glass door lit while the house uses power
  v+='<rect class="win'+lit+'" x="58" y="190" width="66" height="55"/>';
  if(lit)v+='<path class="wgl" d="M58,232L96,190H110L58,245ZM76,245L124,203V214L88,245Z"/>';
  v+='<path class="wfr" d="M58,245V190H124V245M80,190V245M102,190V245"/>';
  v+='<rect class="inv" x="218" y="186" width="64" height="40" rx="5"/><path class="wave" d="M236,206q7,-11 14,0t14,0"/>';
  // home
  v+='<text class="fs" x="58" y="278">To home</text><text class="fb" x="58" y="310">'+kwv(Hw)+'<tspan class="fu"> kW</tspan></text>';
  v+='</g><g transform="translate(0,'+HY+')">';
  // pylon
  v+='<path class="py" d="M494,245L510,150L526,245M498,222H522M501,198H519M504,176H516M486,170H534M490,184H530M498,222L519,198L501,198L516,176M522,222L501,198"/>';
  // grid bubble
  var gc=on(G)?(G>0?' exp':' imp'):'';
  v+='<circle class="nd'+gc+'" cx="510" cy="92" r="58"/>';
  v+='<text class="fb" x="510" y="94" text-anchor="middle">'+kwv(G)+'</text><text class="fl" x="510" y="118" text-anchor="middle">'+
     (on(G)?(G>0?'kW to grid':'kW from grid'):'kW grid')+'</text>';
  v+='</g>';
  // battery with charge level
  var lv=soc==null?0:Math.max(0,Math.min(100,soc)),bh=Math.round(52*lv/100);
  v+='<g transform="translate('+DX+','+BY+')">';
  v+='<rect class="btc" x="243" y="263" width="14" height="6" rx="1.5"/><rect class="bt" x="232" y="268" width="36" height="58" rx="5"/>';
  if(bh)v+='<rect class="btf" x="235" y="'+(323-bh)+'" width="30" height="'+bh+'" rx="2.5"/>';
  v+='<text class="fb" x="284" y="300">'+(soc==null?D:f(soc,0))+'<tspan class="fu"> %</tspan></text>';
  v+='<text class="fs" x="284" y="324">'+(on(B)?(B>0?'Charging ':'Discharging ')+kwv(B)+' kW':'Battery idle')+'</text>';
  v+='</g>';
  // sun bubble + small sun, production today
  v+='<circle class="nd'+(sunOn?' sun':'')+'" cx="'+(250+DX)+'" cy="64" r="58"/>';
  v+='<text class="fb" x="'+(250+DX)+'" y="66" text-anchor="middle">'+kwv(P)+'</text><text class="fl" x="'+(250+DX)+'" y="90" text-anchor="middle">kW solar now</text>';
  v+=wxIcon(wx?!wx.day:!sunOn);                                        // weather top right (night: moon)
  v+='<text class="fs" x="20" y="30">Production today</text><text class="fb" x="20" y="64">'+(prodToday==null?D:f(prodToday,1))+'<tspan class="fu"> kWh</tspan></text>';
  return '<section class="card flow">'+v+'</svg></section>';
}
// Usage chart (switched on the setup page): /history = per 5 min house use from solar (s), battery (b), grid (g) and solar production (p), in W; battery charge level (c) in 0.1 %
function loadHist(){histAt=Date.now();fetch('/history',{cache:'no-store'}).then(function(r){return r.json();})
  .then(function(d){hist=d;if(lastData)render(lastData);}).catch(function(){});}
// previous days (hView = yyyymmdd, 0 = today): fetched once, they don't change
function loadOld(day){hOld[day]='…';fetch('/history?day='+day,{cache:'no-store'}).then(function(r){return r.ok?r.json():null;})
  .then(function(d){hOld[day]=d&&d.s?d:'x';if(lastData)render(lastData);}).catch(function(){hOld[day]='x';if(lastData)render(lastData);});}
function hStep(dir){var ds=(hist&&hist.days)||[],i=hView?ds.indexOf(hView):ds.length;
  i+=dir;if(i<0)return;hView=i<ds.length?ds[i]:0;if(lastData)render(lastData);}
function dayName(n){var dt=new Date(Math.floor(n/10000),Math.floor(n/100)%100-1,n%100);
  return ['Sun','Mon','Tue','Wed','Thu','Fri','Sat'][dt.getDay()]+' '+(n%100)+' '+
    ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'][dt.getMonth()];}
// chart card header with the day arrows (both charts show the same day); d = today's data or null
// d = the day's data; while it loads or when there is none, an empty day with a message (msg), so the
// card keeps exactly the same height in every case
function chDay(){var ds=(hist&&hist.days)||[],iv=hView?ds.indexOf(hView):ds.length;
  if(hView&&iv<0){hView=0;iv=ds.length;}                  // that day is gone
  var d=hist,msg='';if(hView){d=hOld[hView];if(d==null)loadOld(hView);if(typeof d!=='object'){msg=d==='x'?'No data for this day':'Loading…';d=null;}}
  if(!d||!d.s){var e=[];for(var k=0;k<288;k++)e.push(null);d={step:5,slot:-1,s:e,b:e,g:e,p:e,c:e};msg=msg||'Loading…';}
  var t=new Date(),day=hView||(hist&&hist.day)||(t.getFullYear()*10000+(t.getMonth()+1)*100+t.getDate());
  return {d:d,iv:iv,msg:msg,day:day};}
function chMsg(m,W,H){return m?'<text class="cm" x="'+(W/2)+'" y="'+(H/2-4)+'" text-anchor="middle">'+m+'</text>':'';}
// card header. With the Day/Week switch (Overview): row 1 = arrow, switch centered, arrow; row 2 = the title,
// centered, so it is never cut off on a phone. Without (Battery): arrow, title, arrow on one row.
// fn = function the arrows call with -1 / 1, what = 'day' or 'week'
function chHead2(t,pOk,nOk,fn,what,seg){return '<section class="card chart" style="margin-bottom:10px"><div class="ch"><button class="hb" onclick="'+fn+'(-1)"'+(pOk?'':' disabled')+
    ' aria-label="Previous '+what+'">&#8249;</button>'+(seg||'<div class="l">'+t+'</div>')+
    '<button class="hb" onclick="'+fn+'(1)"'+(nOk?'':' disabled')+' aria-label="Next '+what+'">&#8250;</button></div>'+(seg?'<div class="l cht">'+t+'</div>':'');}
function chHead(t,iv,seg){return chHead2(t,iv>0,!!hView,'hStep','day',seg);}
// Overview period is remembered in this browser.
var ovPeriod='d',wEnd=0,curW=null,monthView=0,yearView=0,summaryYears={};
try{var savedPeriod=localStorage.getItem('ovMode');if(['d','w','m','y'].indexOf(savedPeriod)>=0)ovPeriod=savedPeriod;}catch(e){}
function ovSeg(){return '<span class="seg" role="group" aria-label="Overview period">'+
    [['d','Day'],['w','Week'],['m','Month'],['y','Year']].map(function(a){
      return '<button type="button" class="'+(ovPeriod===a[0]?'on':'')+'" aria-pressed="'+(ovPeriod===a[0])+'" onclick="ovMode(\''+a[0]+'\')">'+a[1]+'</button>';
    }).join('')+'</span>';}
function ovMode(p){ovPeriod=p;try{localStorage.setItem('ovMode',p);}catch(e){}hSel=null;if(lastData)render(lastData);}
function chWidth(){var app=document.getElementById('app');return Math.max(260,Math.round(app&&app.clientWidth?app.clientWidth-70:600));}
function chHours(g,L,pw,n,step,H){[[6,'6'],[12,'Noon'],[18,'18']].forEach(function(a){var x=(L+a[0]*60/step*pw/n).toFixed(1);
    g.s+='<text x="'+x+'" y="'+(H-6)+'" text-anchor="middle">'+a[1]+'</text>';});}
// Production & consumption today (like the SolarEdge app): from today's 5-minute chart data, so it adds up
// with the Overview chart. Production = solar production; to home = house use from solar; to grid = the
// exported counter; to battery = the rest. Consumption = house use, split into from solar / battery / grid.
var PCI={home:'<path d="M2.5,8L8,3.2L13.5,8V13.5H2.5Z"/><path d="M6.5,13.5V10H9.5V13.5"/>',
  bat:'<rect x="4.5" y="3" width="7" height="11" rx="1.5"/><path d="M6.5,1.8H9.5M8.6,5.5L6.8,8.8H9.2L7.4,11.8"/>',
  grid:'<path d="M5,14.5L8,2L11,14.5M3.5,5H12.5M4,8H12M6.6,9L9.6,12.8M9.4,9L6.4,12.8"/>',
  sun:'<circle cx="8" cy="8" r="2.8"/><path d="M8,1.5V3M8,13V14.5M1.5,8H3M13,8H14.5M3.4,3.4L4.5,4.5M11.5,11.5L12.6,12.6M3.4,12.6L4.5,11.5M11.5,4.5L12.6,3.4"/>'};
function eTxt(k){return k==null?'–':k<1?f(k*1000,k<0.1?1:0)+' Wh':f(k,2)+' kWh';}
function pcPart(title,tot,items,foot){
  var h='<div class="pcp"><div class="pct">'+title+'</div><div class="pcv">'+(tot==null?'–':f(tot,tot>=10?1:2))+'<small> kWh</small></div>';
  var bar='',dn='',at=0;
  items.forEach(function(it){var pc=tot>0&&it[2]!=null?100*it[2]/tot:0;
    if(pc>0){bar+='<i class="'+it[3]+'" style="width:'+pc.toFixed(2)+'%"></i>';
      dn+='<circle class="'+it[3]+'" cx="50" cy="50" r="40" pathLength="100" stroke-dasharray="'+pc.toFixed(2)+' '+(100-pc).toFixed(2)+'" stroke-dashoffset="'+(-at).toFixed(2)+'"/>';at+=pc;}});
  h+='<div class="pcb">'+bar+'</div><div class="pcr"><svg class="dn" viewBox="0 0 100 100"><circle class="dn0" cx="50" cy="50" r="40"/><g transform="rotate(-90 50 50)">'+dn+'</g></svg><div class="pcl">';
  items.forEach(function(it){var pc=tot>0&&it[2]!=null?Math.round(100*it[2]/tot):null;
    h+='<div class="li"><i class="dot '+it[3]+'"></i><svg class="ic" viewBox="0 0 16 16">'+PCI[it[1]]+'</svg><span class="n">'+it[0]+'</span>'+
       '<b>'+eTxt(it[2])+'</b><span class="pp">'+(pc==null?'':'('+pc+'%)')+'</span></div>';});
  return h+'</div></div>'+(foot||'')+'</div>';
}
function chart(){
  var cd=chDay(),d=cd.d,h=chHead('Solar Production '+dayName(cd.day),cd.iv,ovSeg());
  var n=d.s.length,W=chWidth(),H=138,L=34,R=8,T=10,B=24,pw=W-L-R,ph=H-T-B,max=0,e=0,i,v;
  // Solar production chart: shows only solar production (p)
  function P(k){return d.p&&d.p[k]!=null?d.p[k]:0;}
  for(i=0;i<n;i++){if(d.s[i]==null)continue;max=Math.max(max,P(i));e+=P(i);}
  // round kW steps (0.2, 0.4, 0.5, 1, 2, 5) so there are at most 4 lines above 0
  var mk=Math.max(max/1000,0.1),st=[0.2,0.4,0.5,1,2,5].filter(function(x){return Math.ceil(mk/x)<=4;})[0]||5;
  var top=st*Math.ceil(mk/st);
  function X(i){return (L+(i+0.5)*pw/n).toFixed(1);}
  function Y(w){return (T+ph-ph*w/(top*1000)).toFixed(1);}
  function area(c,fn,own){var p='',q='',run=[];
    function used(k){return k>=0&&k<n&&d.s[k]!=null&&own(k)>0;}
    function flush(){if(run.length){var top='',pen=false;
      run.forEach(function(k,j){top+=(j?'L':'M')+X(k)+','+Y(fn(k));
        if(used(k)||used(k-1)||used(k+1)){q+=(pen?'L':'M')+X(k)+','+Y(fn(k));pen=true;}else pen=false;});
      p+='M'+X(run[0])+','+Y(0)+'L'+top.slice(1)+'L'+X(run[run.length-1])+','+Y(0)+'Z';}run=[];}
    for(var k=0;k<n;k++){if(d.s[k]==null)flush();else run.push(k);}flush();
    return p?'<path class="a '+c+'" d="'+p+'"/>'+(q?'<path class="o '+c+'" d="'+q+'"/>':''):'';}
  curD=d;
  var g='<svg viewBox="0 0 '+W+' '+H+'" role="img" aria-label="Solar production" data-cur="o" data-w="'+W+'" data-l="'+L+'" data-pw="'+pw+'" data-n="'+n+'" data-t="'+T+'" data-ph="'+ph+'" data-top="'+top+'">';
  for(v=0;v<=top+1e-9;v+=st)g+='<line class="gl" x1="'+L+'" x2="'+(W-R)+'" y1="'+Y(v*1000)+'" y2="'+Y(v*1000)+'"/>'+
    '<text x="'+(L-6)+'" y="'+(+Y(v*1000)+4)+'" text-anchor="end">'+(v?f(v,st<1?1:0):'kW')+'</text>';
  g+=area('cp',P,P);
  var hg={s:''};chHours(hg,L,pw,n,d.step,H);g+=hg.s;
  g+='<line class="ax" x1="'+L+'" x2="'+(W-R)+'" y1="'+Y(0)+'" y2="'+Y(0)+'"/>';   // bottom axis
  if(d.slot>=0){g+='<line class="now" x1="'+X(d.slot)+'" x2="'+X(d.slot)+'" y1="'+T+'" y2="'+Y(0)+'"/>';
    if(P(d.slot)>0)g+='<circle class="nr" cx="'+X(d.slot)+'" cy="'+Y(P(d.slot))+'" r="3"/>';}
  g+=chMsg(cd.msg,W,H)+'<g class="cur"></g></svg>';
  var total=productionTotal(d);
  var lg='<span><i class="cp"></i>Solar production <b>'+(cd.msg||total==null?'–':f(total,1))+' kWh</b></span>';
  return h+g+'<div class="lgd">'+lg+'</div></section>';
}
// ---- Overview, week: 7 days up to wEnd (today by default), from the same 5-minute day data as the day view.
var MON=['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
function dDate(n){return new Date(Math.floor(n/10000),Math.floor(n/100)%100-1,n%100);}
function dKey(t){return t.getFullYear()*10000+(t.getMonth()+1)*100+t.getDate();}
function addDays(n,k){var t=dDate(n);t.setDate(t.getDate()+k);return dKey(t);}
function todayKey(){return (hist&&hist.day)||dKey(new Date());}
var tariffDays={},tariffClock=new Intl.DateTimeFormat('en-GB',{timeZone:'Europe/London',hour:'2-digit',hourCycle:'h23'});
function tariffWindow(day){
  if(tariffDays[day])return tariffDays[day];
  var t=Date.UTC(Math.floor(day/10000),Math.floor(day/100)%100-1,day%100),before=Number(tariffClock.format(t)),after=Number(tariffClock.format(t+12*3600000));
  return tariffDays[day]={start:after===13?2:1,end:after===13?9:8,repeated:before===1&&after===12};
}
function tariffSplit(day,values,step,mask,energy){
  if(!day||!Array.isArray(values))return null;
  var win=tariffWindow(day),daySum=0,nightSum=0,any=false;
  for(var i=0;i<values.length;i++){
    if(values[i]==null||(mask&&mask[i]==null))continue;
    var hour=i*step/60;
    if(win.repeated&&hour>=1&&hour<2)return null;
    any=true;
    if(hour>=win.start&&hour<win.end)nightSum+=values[i]*(energy?1:step/60000);
    else daySum+=values[i]*(energy?1:step/60000);
  }
  return any?{day:daySum,night:nightSum}:null;
}
function standingDays(first,last,today){
  if(last>today)last=today;
  if(last<first)return 0;
  function utc(n){return Date.UTC(Math.floor(n/10000),Math.floor(n/100)%100-1,n%100);}
  return Math.round((utc(last)-utc(first))/86400000)+1;
}
function tariffDetails(day,night,total,days){
  if(total==null)return '';
  if(day==null||night==null)return ' <span>Tariff breakdown unavailable</span>';
  var pct=function(v){return total>0?f(100*v/total,0)+'%':'\u2013';};
  return ' <span class="rate-breakdown">(<span><i class="day"></i>Day '+pct(day)+' '+f(day,1)+' kWh</span>'+
    '<span><i class="night"></i>Night '+pct(night)+' '+f(night,1)+' kWh</span>)</span>'+
    ' <span>Estimated cost \u00a3'+f(day*.3649+night*.1719+days*.1333,2)+' (2026 rates; recorded import + standing charge)</span>';
}
function wHasBefore(e){var ds=(hist&&hist.days)||[];return ds.length>0&&ds[0]<=addDays(e,-7);}   // any day before this week?
function wStep(dir){var t=todayKey(),e=wEnd||t,ne=addDays(e,7*dir);
  if(dir<0&&!wHasBefore(e))return;if(dir>0&&ne>=t)ne=0;wEnd=ne;hSel=null;if(lastData)render(lastData);}
// one day's totals in kWh {p,c,i,e}; null = no data for that day, false = still loading
function dayTot(n){var d;
  if(n===todayKey())d=hist;
  else{if(!hist||(hist.days||[]).indexOf(n)<0)return null;d=hOld[n];
    if(d==null){loadOld(n);return false;}if(typeof d!=='object')return d==='x'?null:false;}
  if(!d||!d.s)return false;
  var p=0,c=0,im=0,ex=0,k=d.step/60000,any=false,anyI=false,anyE=false;
  for(var i=0;i<d.s.length;i++){if(d.s[i]==null)continue;any=true;
    p+=d.p&&d.p[i]!=null?d.p[i]:0;c+=d.s[i]+d.b[i]+d.g[i];
    if(d.i&&d.i[i]!=null){im+=d.i[i];anyI=true;}if(d.e&&d.e[i]!=null){ex+=d.e[i];anyE=true;}}
  var rates=tariffSplit(n,d.i,d.step,d.s);
  return any?{p:p*k,c:c*k,i:anyI?im*k:null,e:anyE?ex*k:null,
    id:rates?rates.day:null,inight:rates?rates.night:null}:null;}
function overviewBars(items,title,prev,next,step,period,msg,partial,series,chargeDays){
  var h=chHead2(title,prev,next,step,period,ovSeg()),n=items.length,W=chWidth(),
    H=138,L=period==='year'?58:34,R=8,T=10,B=24,pw=W-L-R,ph=H-T-B,max=0,
    sums=series.map(function(){return 0;}),counts=series.map(function(){return 0;}),v,
    rateDay=0,rateNight=0,rateMissing=false,rateAny=false;
  items.forEach(function(item){var x=item.t;if(x)series.forEach(function(s,j){
    if(x[s.key]!=null){max=Math.max(max,x[s.key]);sums[j]+=x[s.key];counts[j]++;
      if(s.key==='i'){rateAny=true;if(x.id==null||x.inight==null)rateMissing=true;
        else{rateDay+=x.id;rateNight+=x.inight;}}}});});
  var some=counts.some(function(count){return count>0;});
  var mk=Math.max(max,1),unit=Math.pow(10,Math.floor(Math.log(Math.max(mk/4,1))/Math.LN10));
  var st=[1,2,5,10].map(function(x){return x*unit;}).filter(function(x){return Math.ceil(mk/x)<=4;})[0],top=st*Math.ceil(mk/st);
  function Y(k){return T+ph-ph*k/top;}
  var gw=pw/n,bw=Math.min(gw*0.68,40);
  function bar(x,k,c,w){if(!(k>0))return '';var y=Y(k);
    return '<rect class="wb '+c+'" x="'+x.toFixed(3)+'" y="'+y.toFixed(1)+'" width="'+(w||bw).toFixed(3)+'" height="'+(T+ph-y).toFixed(1)+'" rx="'+(c==='ci'||c==='ce'?0:1.5)+'"/>';}
  var metric=series.length>1?'grid':series[0].key,
    g='<svg viewBox="0 0 '+W+' '+H+'" role="img" aria-label="'+title+'" data-cur="w" data-series="'+metric+'" data-w="'+W+'" data-l="'+L+'" data-pw="'+pw+'" data-n="'+n+'" data-t="'+T+'" data-ph="'+ph+'">';
  for(v=0;v<=top+1e-9;v+=st)g+='<line class="gl" x1="'+L+'" x2="'+(W-R)+'" y1="'+Y(v).toFixed(1)+'" y2="'+Y(v).toFixed(1)+'"/>'+
    '<text x="'+(L-6)+'" y="'+(Y(v)+4).toFixed(1)+'" text-anchor="end">'+(v?f(v,0):'kWh')+'</text>';
  items.forEach(function(item,k){var cx=L+(k+0.5)*gw,x=item.t;
    if(x)series.forEach(function(s){if(x[s.key]!=null){
      var grid=series.length>1,w=grid?bw*2/3:bw,offset=grid&&s.key==='e'?bw/3:0;
      if(s.key==='i'&&x.id!=null&&x.inight!=null){
        var left=cx-bw/2,yd=Y(x.id),yt=Y(x.id+x.inight);
        if(x.id>0)g+='<rect class="wb cday" x="'+left.toFixed(3)+'" y="'+yd.toFixed(1)+'" width="'+w.toFixed(3)+'" height="'+(Y(0)-yd).toFixed(1)+'"/>';
        if(x.inight>0)g+='<rect class="wb cnight" x="'+left.toFixed(3)+'" y="'+yt.toFixed(1)+'" width="'+w.toFixed(3)+'" height="'+(yd-yt).toFixed(1)+'"/>';
      }else g+=bar(cx-bw/2+offset,x[s.key],s.cls,w);}});
    if(item.axis&&(n<=7||k%(n>12?5:(W<400?2:1))===0))
      g+='<text x="'+cx.toFixed(1)+'" y="'+(H-6)+'" text-anchor="middle">'+item.axis+'</text>';});
  g+='<line class="ax" x1="'+L+'" x2="'+(W-R)+'" y1="'+Y(0).toFixed(1)+'" y2="'+Y(0).toFixed(1)+'"/>';
  g+=chMsg(msg||(some?'':'No data for this '+period),W,H)+'<g class="cur"></g></svg>';
  curW=items;
  var kwh=function(j){return counts[j]?f(sums[j],1):'\u2013';};
  var lg=series.map(function(s,j){return '<span'+(s.key==='i'?' class="import-legend"':'')+'><i class="'+s.cls+'"></i>'+s.label+' <b>'+kwh(j)+' kWh</b>'+
    (s.key==='i'&&rateAny?tariffDetails(rateMissing?null:rateDay,rateMissing?null:rateNight,sums[j],chargeDays):'')+'</span>';}).join('')+
    (some&&partial?'<span>Available data only</span>':'');
  return h+g+'<div class="lgd">'+lg+'</div></section>';
}
var PERIOD_SERIES={
  p:[{key:'p',label:'Solar production',cls:'cp'}],
  c:[{key:'c',label:'Consumption',cls:'cc'}],
  grid:[{key:'i',label:'Grid import',cls:'ci'},{key:'e',label:'Grid export',cls:'ce'}]
};
function weekChart(kind){
  var t=todayKey(),e=wEnd||t,days=[];
  for(var i=6;i>=0;i--)days.push(addDays(e,-i));
  var TT=days.map(dayTot),d0=dDate(days[0]),d1=dDate(e);
  var title=(kind==='p'?'Solar production':kind==='c'?'Consumption':'Grid import & export')+' '+d0.getDate()+(d0.getMonth()!==d1.getMonth()?' '+MON[d0.getMonth()]:'')+' \u2013 '+d1.getDate()+' '+MON[d1.getMonth()];
  var items=days.map(function(n,k){return {n:n,t:TT[k]||null,axis:dDate(n).toLocaleDateString(undefined,{day:'numeric',month:'numeric'})};});
  return overviewBars(items,title,wHasBefore(e),!!wEnd,'wStep','week',
    !TT.some(function(x){return !!x;})&&TT.some(function(x){return x===false;})?'Loading\u2026':'',false,PERIOD_SERIES[kind],standingDays(days[0],e,t));
}
function summaryFor(year){
  var entry=summaryYears[year];
  if(!entry||(!entry.loading&&Date.now()-entry.at>60000)){
    entry=summaryYears[year]={loading:true,at:Date.now()};
    fetch('/history-summary?year='+year,{cache:'no-store'}).then(function(r){
      if(!r.ok)throw new Error('History summary request failed ('+r.status+')');return r.json();
    }).then(function(data){
      if(!Array.isArray(data.days))throw new Error('Invalid history summary');
      entry.days={};data.days.forEach(function(row){
        var rates=tariffSplit(row[0],row[5],60,null,true);
        entry.days[row[0]]={p:row[1],c:row[2],i:row[3],e:row[4],id:rates?rates.day:null,inight:rates?rates.night:null};});
      entry.loading=false;if(lastData)render(lastData);
    }).catch(function(err){entry.loading=false;entry.error=true;console.error('History summary:',err);if(lastData)render(lastData);});
  }
  return entry;
}
function periodStep(dir){
  var today=dDate(todayKey()),d;
  if(ovPeriod==='m'){
    d=monthView?dDate(monthView):new Date(today.getFullYear(),today.getMonth(),1);
    d.setMonth(d.getMonth()+dir);monthView=dKey(d);
  }else{
    d=new Date(yearView||today.getFullYear(),0,1);d.setFullYear(d.getFullYear()+dir);yearView=d.getFullYear();
  }
  hSel=null;if(lastData)render(lastData);
}
function periodChart(kind){
  var today=todayKey(),now=dDate(today),isMonth=ovPeriod==='m',
    date=isMonth?(monthView?dDate(monthView):new Date(now.getFullYear(),now.getMonth(),1)):
      new Date(yearView||now.getFullYear(),0,1),
    year=date.getFullYear(),first=dKey(date),ds=(hist&&hist.days)||[],entry=hist?summaryFor(year):null,
    msg=!entry||entry.loading?'Loading\u2026':entry.error?'Unable to load history':'',items=[],partial=false;
  function value(n){return n===today?dayTot(n):(entry&&entry.days?entry.days[n]:null);}
  if(isMonth){
    var count=new Date(year,date.getMonth()+1,0).getDate();
    for(var i=1;i<=count;i++){var n=dKey(new Date(year,date.getMonth(),i)),v=msg?null:value(n);
      if(n<=today&&!v)partial=true;
      if(kind==='grid'&&v&&(v.i==null||v.e==null))partial=true;
      items.push({n:n,t:v||null,axis:String(i)});}
  }else{
    for(var m=0;m<12;m++){var total=null,rateMissing=false,days=new Date(year,m+1,0).getDate();
      for(var j=1;j<=days;j++){var dn=dKey(new Date(year,m,j)),val=msg?null:value(dn);
        if(dn<=today&&!val)partial=true;
        if(kind==='grid'&&val&&(val.i==null||val.e==null))partial=true;
        if(val){var base=total||{p:0,c:0,i:null,e:null,id:0,inight:0};
          if(val.i!=null&&(val.id==null||val.inight==null))rateMissing=true;
          total={p:base.p+val.p,c:base.c+val.c,
            i:val.i==null?base.i:(base.i==null?0:base.i)+val.i,
            e:val.e==null?base.e:(base.e==null?0:base.e)+val.e,
            id:base.id+(val.id||0),inight:base.inight+(val.inight||0)};}}
      if(total&&rateMissing){total.id=null;total.inight=null;}
      items.push({n:dKey(new Date(year,m,1)),t:total,axis:MON[m],label:MON[m]+' '+year});}
  }
  var title=(kind==='p'?'Solar production':kind==='c'?'Consumption':'Grid import & export')+' '+(isMonth?MON[date.getMonth()]+' '+year:String(year)),
    next=isMonth?year<now.getFullYear()||year===now.getFullYear()&&date.getMonth()<now.getMonth():year<now.getFullYear();
  var last=isMonth?year*10000+(date.getMonth()+1)*100+new Date(year,date.getMonth()+1,0).getDate():year*10000+1231;
  return overviewBars(items,title,ds.length>0&&ds[0]<first,next,'periodStep',isMonth?'month':'year',msg,partial,PERIOD_SERIES[kind],standingDays(first,last,today));
}
// Battery charge level over the day (c = 0.1 %, null = no reading), same day as the chart above
function consChart(){
  var cd=chDay(),d=cd.d,h=chHead('Consumption '+dayName(cd.day),cd.iv);
  var n=d.s.length,W=chWidth(),H=138,L=34,R=8,T=10,B=24,pw=W-L-R,ph=H-T-B,max=0,e=0,i,v;
  function C(k){return d.s[k]+(d.b&&d.b[k]?d.b[k]:0)+(d.g&&d.g[k]?d.g[k]:0);}
  for(i=0;i<n;i++){if(d.s[i]==null)continue;max=Math.max(max,C(i));e+=C(i);}
  var mk=Math.max(max/1000,0.1),st=[0.2,0.4,0.5,1,2,5].filter(function(x){return Math.ceil(mk/x)<=4;})[0]||5;
  var top=st*Math.ceil(mk/st);
  function X(i){return (L+(i+0.5)*pw/n).toFixed(1);}
  function Y(w){return (T+ph-ph*w/(top*1000)).toFixed(1);}
  function area(c,fn,own){var p='',q='',run=[];
    function used(k){return k>=0&&k<n&&d.s[k]!=null&&own(k)>0;}
    function flush(){if(run.length){var top='',pen=false;
      run.forEach(function(k,j){top+=(j?'L':'M')+X(k)+','+Y(fn(k));
        if(used(k)||used(k-1)||used(k+1)){q+=(pen?'L':'M')+X(k)+','+Y(fn(k));pen=true;}else pen=false;});
      p+='M'+X(run[0])+','+Y(0)+'L'+top.slice(1)+'L'+X(run[run.length-1])+','+Y(0)+'Z';}run=[];}
    for(var k=0;k<n;k++){if(d.s[k]==null)flush();else run.push(k);}flush();
    return p?'<path class="a '+c+'" d="'+p+'"/>'+(q?'<path class="o '+c+'" d="'+q+'"/>':''):'';}
  curD=d;
  var g='<svg viewBox="0 0 '+W+' '+H+'" role="img" aria-label="House consumption" data-cur="c" data-w="'+W+'" data-l="'+L+'" data-pw="'+pw+'" data-n="'+n+'" data-t="'+T+'" data-ph="'+ph+'" data-top="'+top+'">';
  for(v=0;v<=top+1e-9;v+=st)g+='<line class="gl" x1="'+L+'" x2="'+(W-R)+'" y1="'+Y(v*1000)+'" y2="'+Y(v*1000)+'"/>'+
    '<text x="'+(L-6)+'" y="'+(+Y(v*1000)+4)+'" text-anchor="end">'+(v?f(v,st<1?1:0):'kW')+'</text>';
  g+=area('cc',C,C);
  var hg={s:''};chHours(hg,L,pw,n,d.step,H);g+=hg.s;
  g+='<line class="ax" x1="'+L+'" x2="'+(W-R)+'" y1="'+Y(0)+'" y2="'+Y(0)+'"/>';
  if(d.slot>=0){g+='<line class="now" x1="'+X(d.slot)+'" x2="'+X(d.slot)+'" y1="'+T+'" y2="'+Y(0)+'"/>';
    if(C(d.slot)>0)g+='<circle class="nr" cx="'+X(d.slot)+'" cy="'+Y(C(d.slot))+'" r="3"/>';}
  g+=chMsg(cd.msg,W,H)+'<g class="cur"></g></svg>';
  var kwh=function(w){return cd.msg?'–':f(w*d.step/60000,1);};
  var lg='<span><i class="cc"></i>Consumption <b>'+kwh(e)+' kWh</b></span>';
  return h+g+'<div class="lgd">'+lg+'</div></section>';
}
function gridChart(){
  if(ovPeriod!=='d')return ovPeriod==='w'?weekChart('grid'):periodChart('grid');
  var cd=chDay(),d=cd.d,h=chHead('Grid import & export '+dayName(cd.day),cd.iv);
  var n=d.s.length,W=chWidth(),H=138,L=34,R=8,T=10,B=24,pw=W-L-R,ph=H-T-B,max=0,im=0,ex=0,i,v;
  function I(k){return d.i&&d.i[k]!=null?d.i[k]:null;}
  function E(k){return d.e&&d.e[k]!=null?d.e[k]:null;}
  var anyI=false,anyE=false,partial=false,win=cd.day?tariffWindow(cd.day):null,
    rates=tariffSplit(cd.day,d.i,d.step);
  for(i=0;i<n;i++){
    if(I(i)!=null){max=Math.max(max,I(i));im+=I(i);anyI=true;}else if(d.s[i]!=null)partial=true;
    if(E(i)!=null){max=Math.max(max,E(i));ex+=E(i);anyE=true;}else if(d.s[i]!=null)partial=true;
  }
  var mk=Math.max(max/1000,0.1),st=[0.2,0.4,0.5,1,2,5].filter(function(x){return Math.ceil(mk/x)<=4;})[0]||5;
  var top=st*Math.ceil(mk/st),gw=pw/n,bw=Math.min(gw*0.68,40);
  function X(k){return L+(k+0.5)*gw;}
  function Y(w){return T+ph-ph*w/(top*1000);}
  function bar(k,w,cls){if(!(w>0))return '';var y=Y(w);
    return '<rect class="wb '+cls+'" x="'+(X(k)-bw/2+(cls==='ce'?bw/3:0)).toFixed(3)+'" y="'+y.toFixed(1)+'" width="'+(bw*2/3).toFixed(3)+'" height="'+(T+ph-y).toFixed(1)+'"/>';}
  curD=d;
  var g='<svg viewBox="0 0 '+W+' '+H+'" role="img" aria-label="Grid import and export" data-cur="ie" data-w="'+W+'" data-l="'+L+'" data-pw="'+pw+'" data-n="'+n+'" data-t="'+T+'" data-ph="'+ph+'" data-top="'+top+'">';
  for(v=0;v<=top+1e-9;v+=st)g+='<line class="gl" x1="'+L+'" x2="'+(W-R)+'" y1="'+Y(v*1000).toFixed(1)+'" y2="'+Y(v*1000).toFixed(1)+'"/>'+
    '<text x="'+(L-6)+'" y="'+(Y(v*1000)+4).toFixed(1)+'" text-anchor="end">'+(v?f(v,st<1?1:0):'kW')+'</text>';
  for(i=0;i<n;i++){
    var hour=i*d.step/60,cls=win&&!(win.repeated&&hour>=1&&hour<2)?(hour>=win.start&&hour<win.end?'cnight':'cday'):'ci';
    g+=bar(i,I(i),cls)+bar(i,E(i),'ce');
  }
  if(win)[win.start,win.end].forEach(function(hour){
    var x=L+pw*hour*60/(n*d.step);
    g+='<line class="rate-cut" x1="'+x.toFixed(1)+'" x2="'+x.toFixed(1)+'" y1="'+T+'" y2="'+Y(0).toFixed(1)+'"/>';
  });
  var hg={s:''};chHours(hg,L,pw,n,d.step,H);g+=hg.s;
  g+='<line class="ax" x1="'+L+'" x2="'+(W-R)+'" y1="'+Y(0).toFixed(1)+'" y2="'+Y(0).toFixed(1)+'"/>';
  if(d.slot>=0)g+='<line class="now" x1="'+X(d.slot).toFixed(1)+'" x2="'+X(d.slot).toFixed(1)+'" y1="'+T+'" y2="'+Y(0).toFixed(1)+'"/>';
  g+=chMsg(cd.msg||(!anyI&&!anyE?'No grid history available':''),W,H)+'<g class="cur"></g></svg>';
  var kwh=function(w,any){return cd.msg||!any?'–':f(w*d.step/60000,1);};
  var lg='<span class="import-legend"><i class="ci"></i>Grid import <b>'+kwh(im,anyI)+' kWh</b>'+
    (anyI&&!cd.msg?tariffDetails(rates?rates.day:null,rates?rates.night:null,im*d.step/60000,1):'')+'</span>'+
    '<span><i class="ce"></i>Grid export <b>'+kwh(ex,anyE)+' kWh</b></span>'+
    ((anyI||anyE)&&partial?'<span>Partial history</span>':'');
  return h+g+'<div class="lgd">'+lg+'</div></section>';
}
function batChart(){
  var cd=chDay(),d=cd.d,h=chHead('Battery '+dayName(cd.day),cd.iv);
  var c=d.c||[],n=d.s.length,W=chWidth(),H=138,L=34,R=8,T=10,B=24,pw=W-L-R,ph=H-T-B,lo=null,hi=null,i,v;
  function X(i){return (L+(i+0.5)*pw/n).toFixed(1);}
  function Y(p){return (T+ph-ph*p/100).toFixed(1);}
  function V(k){return c[k]==null?null:c[k]/10;}
  var p='',q='',run=[];
  function used(k){return k>=0&&k<n&&V(k)!=null&&V(k)>0;}
  function flush(){if(run.length){var top='',pen=false;
    run.forEach(function(k,j){top+=(j?'L':'M')+X(k)+','+Y(V(k));
      if(used(k)||used(k-1)||used(k+1)){q+=(pen?'L':'M')+X(k)+','+Y(V(k));pen=true;}else pen=false;});
    p+='M'+X(run[0])+','+Y(0)+'L'+top.slice(1)+'L'+X(run[run.length-1])+','+Y(0)+'Z';}run=[];}
  for(i=0;i<n;i++){v=V(i);if(v==null){flush();continue;}run.push(i);lo=lo==null?v:Math.min(lo,v);hi=hi==null?v:Math.max(hi,v);}flush();
  curD=d;
  var g='<svg viewBox="0 0 '+W+' '+H+'" role="img" aria-label="Battery charge level" data-cur="b" data-w="'+W+'" data-l="'+L+'" data-pw="'+pw+'" data-n="'+n+'" data-t="'+T+'" data-ph="'+ph+'">';
  for(v=0;v<=100;v+=20)g+='<line class="gl" x1="'+L+'" x2="'+(W-R)+'" y1="'+Y(v)+'" y2="'+Y(v)+'"/>'+
    '<text x="'+(L-6)+'" y="'+(+Y(v)+4)+'" text-anchor="end">'+(v?v:'%')+'</text>';
  if(p)g+='<path class="a cf" d="'+p+'"/>'+(q?'<path class="o cf" d="'+q+'"/>':'');
  var hg={s:''};chHours(hg,L,pw,n,d.step,H);g+=hg.s;
  g+='<line class="ax" x1="'+L+'" x2="'+(W-R)+'" y1="'+Y(0)+'" y2="'+Y(0)+'"/>';
  var now=d.slot>=0?V(d.slot):null,last=null;
  for(i=n-1;i>=0&&last==null;i--)last=V(i);
  if(d.slot>=0){g+='<line class="now" x1="'+X(d.slot)+'" x2="'+X(d.slot)+'" y1="'+T+'" y2="'+Y(0)+'"/>';
    if(now!=null)g+='<circle class="nr bt" cx="'+X(d.slot)+'" cy="'+Y(now)+'" r="3"/>';}
  g+=chMsg(cd.msg,W,H)+'<g class="cur"></g></svg>';
  function pc(x){return x==null?'–':Math.round(x)+' %';}
  var lg='<span><i class="cf"></i>Battery level <b>'+pc(d.slot>=0?now:last)+'</b> '+(d.slot>=0?'now':'at the end of the day')+'</span>'+
    '<span>Lowest <b>'+pc(lo)+'</b></span><span>Highest <b>'+pc(hi)+'</b></span>';
  return h+g+'<div class="lgd">'+lg+'</div></section>';
}
if(typeof window!=='undefined'&&window.addEventListener)window.addEventListener('resize',function(){if(lastData)render(lastData);});
// Cursor on both charts: move the mouse or a finger over a chart to see the time and the values of that
// 5 minutes (both charts show the same moment). Mouse: gone when it leaves the chart; touch: stays until
// you tap somewhere else. hSel = slot (null = no cursor); curD = the day the charts show.
var hSel=null,hSelW=false,curD=null;                // hSelW: hSel is a day of the week chart, not a 5-minute slot
function curDraw(){
  var ss=document.querySelectorAll?document.querySelectorAll('svg[data-cur]'):[];
  for(var j=0;j<ss.length;j++){var svg=ss[j],g=svg.querySelector('g.cur'),ds=svg.dataset;if(!g)continue;
    var wk=ds.cur==='w',d=curD,k=hSel;
    if(k==null||k<0||wk!==hSelW||(wk?!curW||k>=curW.length:!d||k>=d.s.length)){g.innerHTML='';continue;}
    var W=+ds.w,L=+ds.l,pw=+ds.pw,n=+ds.n,T=+ds.t,ph=+ds.ph,x=L+(k+0.5)*pw/n,rows=[],o=ds.cur==='o',v='',tm;
    if(wk)tm=curW[k].label||dayName(curW[k].n);else{var m=k*d.step;tm=('0'+Math.floor(m/60)).slice(-2)+':'+('0'+m%60).slice(-2);}
    function Y(fr){return T+ph-ph*fr;}
    if(wk){var w=curW[k].t,series=PERIOD_SERIES[ds.series]||[];
      if(w)rows=series.filter(function(s){return w[s.key]!=null;}).map(function(s){return [s.label,f(w[s.key],1)+' kWh',s.cls];});}
    else if(o){var top=+ds.top*1000;
      if(d.s[k]!=null){var P=d.p&&d.p[k]!=null?d.p[k]:0;
        rows=[['Production',P,'cp']].map(function(r){
          v+='<circle class="cd '+r[2]+'" cx="'+x.toFixed(1)+'" cy="'+Y(r[1]/top).toFixed(1)+'" r="3"/>';return [r[0],f(r[1]/1000,2)+' kW',r[2]];});}}
    else if(ds.cur==='c'){var top=+ds.top*1000;
      if(d.s[k]!=null){var C=d.s[k]+(d.b&&d.b[k]?d.b[k]:0)+(d.g&&d.g[k]?d.g[k]:0);
        rows=[['Consumption',C,'cc']].map(function(r){
          v+='<circle class="cd '+r[2]+'" cx="'+x.toFixed(1)+'" cy="'+Y(r[1]/top).toFixed(1)+'" r="3"/>';return [r[0],f(r[1]/1000,2)+' kW',r[2]];});}}
    else if(ds.cur==='ie'){var top=+ds.top*1000;
      rows=[];
      if(d.e&&d.e[k]!=null){var EX=d.e[k];v+='<circle class="cd ce" cx="'+x.toFixed(1)+'" cy="'+Y(EX/top).toFixed(1)+'" r="3"/>';rows.push(['Export',f(EX/1000,2)+' kW','ce']);}
      if(d.i&&d.i[k]!=null){var IM=d.i[k];v+='<circle class="cd ci" cx="'+x.toFixed(1)+'" cy="'+Y(IM/top).toFixed(1)+'" r="3"/>';rows.push(['Import',f(IM/1000,2)+' kW','ci']);}}
    else{var c=d.c&&d.c[k]!=null?d.c[k]/10:null;
      if(c!=null){v+='<circle class="cd cf" cx="'+x.toFixed(1)+'" cy="'+Y(c/100).toFixed(1)+'" r="3"/>';rows=[['Battery',Math.round(c)+' %','cf']];}}
    if(!rows.length)rows=[[wk?'No data':'No reading','','']];
    var lh=20,bh=7+lh*(rows.length+1);
    var t='<line x1="'+x.toFixed(1)+'" x2="'+x.toFixed(1)+'" y1="'+T+'" y2="'+(T+ph)+'"/>'+v+
      '<g class="tip"><rect class="bx" x="0" y="'+T+'" width="0" height="'+bh+'" rx="4.5"/>'+
      '<text class="ct th" x="8" y="'+(T+20)+'"><tspan class="tb">'+tm+'</tspan></text>';
    rows.forEach(function(r,i){var y=T+20+lh*(i+1);
      t+=(r[2]?'<rect class="sq '+r[2]+'" x="8" y="'+(y-11)+'" width="10" height="10" rx="1.5"/>':'')+
        '<text class="ct cl" x="'+(r[2]?24:8)+'" y="'+y+'">'+r[0]+'</text>'+
        '<text class="ct tb cv" x="0" y="'+y+'" text-anchor="end">'+r[1]+'</text>';});
    g.innerHTML=t+'</g>';
    var tip=g.querySelector('.tip'),labels=tip.querySelectorAll('.cl'),values=tip.querySelectorAll('.cv'),
      bw=tip.querySelector('.th').getComputedTextLength()+16;
    rows.forEach(function(r,i){bw=Math.max(bw,(r[2]?24:8)+labels[i].getComputedTextLength()+
      (r[1]?10+values[i].getComputedTextLength():0)+8);});
    var bx=x+7+bw<=W-4?x+7:x-7-bw;bx=Math.max(2,Math.min(W-4-bw,bx));
    tip.setAttribute('transform','translate('+bx.toFixed(1)+' 0)');
    tip.querySelector('.bx').setAttribute('width',bw);
    for(var a=0;a<values.length;a++)values[a].setAttribute('x',bw-8);}
}
function curMove(e){
  var svg=e.target&&e.target.closest?e.target.closest('svg[data-cur]'):null;
  if(!svg){if(hSel!=null&&(e.type==='pointerdown'||e.pointerType==='mouse')){hSel=null;curDraw();}return;}
  var r=svg.getBoundingClientRect(),ds=svg.dataset,n=+ds.n;if(!r.width)return;
  var x=(e.clientX-r.left)*ds.w/r.width,k=Math.floor((x-ds.l)/ds.pw*n),wk=ds.cur==='w';
  k=Math.max(0,Math.min(n-1,k));if(k!==hSel||wk!==hSelW){hSel=k;hSelW=wk;curDraw();}
}
if(typeof document!=='undefined'&&document.addEventListener){document.addEventListener('pointermove',curMove);document.addEventListener('pointerdown',curMove);}
function card(t,body){return '<section class="card"><h2>'+t+'</h2>'+body+'</section>';}
function render(d){
  lastData=d;
  var s=d.summary||{},h='';
  if(showFlow){wxCheck(d);h+=flow(s);}
  if(showChart){
    if(ovPeriod==='d')h+=chart()+consChart();
    else if(ovPeriod==='w')h+=weekChart('p')+weekChart('c');
    else h+=periodChart('p')+periodChart('c');
  }
  if(showGridChart)h+=gridChart();
  if(showBatChart)h+=batChart();
  if((showFlow||showChart||showBatChart||showGridChart)&&Date.now()-histAt>60000)loadHist();
  if(showDetails){
  h+='<div class="cards">';
  (d.inverters||[]).forEach(function(o){h+=card('Inverter (unit '+o.unit+')',o.ok===false?'<p class="err">No data</p>':rows(INV,o));});
  (d.meters||[]).forEach(function(o){h+=card('Meter '+o.index,o.ok===false?'<p class="err">No data</p>':rows(MET,o));});
  if(d.battery)h+=card('Battery',d.battery.ok===false?'<p class="err">No data</p>':rows(BAT,d.battery));
  h+='</div>';
  }
  document.getElementById('app').innerHTML=h;
  curDraw();                                                    // keep the chart cursor after a refresh
  if(d.poll_s)period=d.poll_s;
  lastAt=Date.now()-d.age_s*1000;
  if(d.age_s>90)msg('No update for '+(d.age_s<120?d.age_s+' s':Math.round(d.age_s/60)+' min'),1);
  else{msg('');live=true;tick();}
}
function load(){
  lastLoad=Date.now();
  fetch('/json',{cache:'no-store'}).then(function(r){return r.json();}).then(function(d){
    if(d.error){msg('Waiting for first reading\u2026');return;}
    render(d);
  }).catch(function(){msg('Connection lost',1);});
}
load();setInterval(load,5000);
</script></body></html>)rawliteral";

#endif
