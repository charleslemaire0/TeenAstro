#include "TeenAstroWifi.h"
#include "HtmlCommon.h"

// Polar-finder page. The browser draws the plate; this file only serves the
// script and a small JSON snapshot of the mount clock and site.

const char html_reticule_style[] PROGMEM = R"raw(
<style>
.ret-wrap{position:relative;background:#12141a;border-radius:8px}
#cv{width:100%;height:78vh;display:block;background:#12141a;touch-action:none}
.ret-panel{position:absolute;z-index:3;top:8px;left:8px;max-width:320px;max-height:calc(78vh - 16px);overflow:auto;
background:rgba(8,10,14,.92);color:#e8eaed;padding:10px 12px;border-radius:8px;font-size:13px;line-height:1.4}
.ret-panel .lt{font-size:1.5em;font-weight:700;margin:2px 0 4px;letter-spacing:.02em}
.ret-panel select,.ret-panel input{width:100%;max-width:100%;box-sizing:border-box;margin:3px 0}
.ret-hide{display:none}
.ret-panel button{margin:4px 4px 0 0}
.ret-note{color:#f0c040;margin-top:4px}
</style>
<div class='card'><div class='ret-wrap'>
<canvas id='cv'></canvas>
<div class='ret-panel'>
<select id='plate'></select>
<div id='customBox' class='ret-hide'>Circle <input id='arcmin' type='number' min='20' max='80' step='0.1'> arcmin</div>
<div id='disp'>
<div>Local time</div>
<div class='lt' id='lt'>--:--:--</div>
<div id='meta'></div>
<button type='button' id='bEdit'>Edit</button>
<button type='button' id='bPlus'>Brighter</button>
<button type='button' id='bMinus'>Less Bright</button>
</div>
<div id='edit' class='ret-hide'>
<label>Date and time<input id='dt' type='datetime-local' step='1'></label>
<label>Latitude (N + / S -)<input id='lat' type='number' step='0.0001'></label>
<label>Longitude (E - / W +)<input id='lon' type='number' step='0.0001'></label>
<button type='button' id='bNow'>Now</button>
<button type='button' id='bSub'>Submit</button>
<button type='button' id='bBack'>Back to realtime</button>
</div>
</div></div></div>
)raw";

const char html_reticule_js[] PROGMEM = R"raw(
var POL={ra:2.5301944,dec:89.2641667};
var OCT={ra:21.146111,dec:-88.956389};
var OCTC=[
{ra:23.46772,dec:-87.48222},
{ra:18.91306,dec:-87.60611},
{ra:22.52709,dec:-85.96725}
];
var DIP=[
{ra:11.0621,dec:61.751},{ra:11.0307,dec:56.3824},{ra:11.8972,dec:53.6947},
{ra:12.2571,dec:57.0326},{ra:12.9005,dec:55.9598},{ra:13.3988,dec:54.9254},
{ra:13.7923,dec:49.3133}
];
var DIPL=[[0,1],[1,2],[2,3],[3,0],[3,4],[4,5],[5,6]];
var CAS=[
{ra:0.6751,dec:56.5373},{ra:0.153,dec:59.15},{ra:0.9451,dec:60.7168},
{ra:1.4303,dec:60.2353},{ra:1.9066,dec:63.6701}
];
var CASL=[[1,0],[0,2],[2,3],[3,4]];
var PL=[
{n:"Takahashi 2010-2050",d:0,y:[2010,2030,2050],dash:[2030],south:1},
{n:"Takahashi 2000-2030",d:0,y:[2000,2010,2020,2030]},
{n:"Konus 2000-2020",d:0,y:[2000,2010,2020]},
{n:"Astro-Physics RAPAS",d:0,y:[2010,2020,2030,2040],south:1},
{n:"Sky-Watcher / Orion 2012-2032",d:1,y:[2012,2020,2028],tick:[2016,2024,2032],north:1,oct:1},
{n:"iOptron",d:2,a:[36,40,44],as:[60,65,70]},
{n:"iOptron, older dial",d:3,a:[36,40,44],as:[60,65,70]},
{n:"Arcminute scale 40-60",d:0,a:[40,45,50,55,60]},
{n:"EXOS-2 / Explore Scientific",d:0,line:[40,45,50,55,60]},
{n:"Classic, epoch 2000",d:0,a:[44],fig:1,oct:1},
{n:"Celestron #94221 / Vixen Polaris, 1995",d:0,a:[45.8],fig:1,oct:1},
{n:"PASILL / Carton, 1995",d:0,a:[46]},
{n:"Explore Scientific 8x50, epoch 2005",d:0,ey:2005},
{n:"Custom circle",d:0,custom:1}
];
var idx=0, custom=44, st=null, manual=null, busy=false, editOn=false;
try{idx=parseInt(localStorage.getItem("taReticle2")||"0",10)||0;}catch(e){}
try{custom=parseFloat(localStorage.getItem("taRetArc")||"44");}catch(e){}
if(!(idx>=0&&idx<PL.length)) idx=0;
if(!(custom>=20&&custom<=80)) custom=44;

function has(a,v){if(!a)return false;var i;for(i=0;i<a.length;i++)if(a[i]===v)return true;return false;}
function pad(n){n=Math.floor(Math.abs(n));return (n<10?"0":"")+n;}
function norm24(h){h=h%24;if(h<0)h+=24;return h;}
function frac(x){x=x-Math.floor(x);if(x<0)x+=1;return x;}
function jdOf(y,m,d,hf){
  var a=Math.floor((14-m)/12), yy=y+4800-a, mm=m+12*a-3;
  var jdn=d+Math.floor((153*mm+2)/5)+365*yy+Math.floor(yy/4)-Math.floor(yy/100)+Math.floor(yy/400)-32045;
  return jdn-0.5+hf/24;
}
function precess(raH,decD,jd){
  var s=(jd-2451545)/36525, as=Math.PI/648000;
  var z=(2306.2181*s+0.30188*s*s+0.017998*s*s*s)*as;
  var ze=(2306.2181*s+1.09468*s*s+0.018203*s*s*s)*as;
  var th=(2004.3109*s-0.42665*s*s-0.041833*s*s*s)*as;
  var ra=raH*Math.PI/12, dec=decD*Math.PI/180;
  var A=Math.cos(dec)*Math.sin(ra+z);
  var B=Math.cos(th)*Math.cos(dec)*Math.cos(ra+z)-Math.sin(th)*Math.sin(dec);
  var C=Math.sin(th)*Math.cos(dec)*Math.cos(ra+z)+Math.cos(th)*Math.sin(dec);
  if(C>1)C=1; if(C<-1)C=-1;
  return {ra:norm24((Math.atan2(A,B)+ze)*12/Math.PI), dec:Math.asin(C)*180/Math.PI};
}
function polMin(dec){return (90-Math.abs(dec))*60;}
function gmst(jd){
  var d=jd-2400000.5, n=Math.floor(d), f=d-n, t=(n-51544.5)/36525;
  return 6.697374558+24*f*1.0027379093+(8640184.812866+(0.093104-6.2e-6*t)*t)*t/3600;
}
function lstOf(jd,lonEast){return 24*frac((gmst(jd)+lonEast/15)/24);}
function hms(h){
  h=norm24(h);
  var H=Math.floor(h+1e-9), m=Math.floor((h-H)*60+1e-9), s=Math.round(((h-H)*60-m)*60);
  if(s>=60){s=0;m++;} if(m>=60){m=0;H=(H+1)%24;}
  return pad(H)+":"+pad(m)+":"+pad(s);
}
function prime(v){
  var r=Math.round(v*10)/10;
  return ((r===Math.round(r))?String(Math.round(r)):String(r))+"\u2032";
}
function splitIso(s){
  if(!s||s.length<19) return null;
  return {y:+s.substr(0,4), m:+s.substr(5,2), d:+s.substr(8,2),
    h:+s.substr(11,2), mi:+s.substr(14,2), s:+s.substr(17,2)};
}
function iso(y,m,d,h,mi,s){
  return y+"-"+pad(m)+"-"+pad(d)+"T"+pad(h)+":"+pad(mi)+":"+pad(s);
}
function toUt(y,m,d,h,mi,s,tz){
  var ms=Date.UTC(y,m-1,d,h,mi,s)-tz*3600000, u=new Date(ms);
  return {y:u.getUTCFullYear(), m:u.getUTCMonth()+1, d:u.getUTCDate(),
    hf:u.getUTCHours()+u.getUTCMinutes()/60+u.getUTCSeconds()/3600,
    h:u.getUTCHours(), mi:u.getUTCMinutes(), s:u.getUTCSeconds()};
}
function yearMark(y,southStar,kind,dashList){
  var jd=jdOf(y,1,1,0), cat=southStar?OCT:POL, p=precess(cat.ra,cat.dec,jd);
  return {arc:polMin(p.dec), name:String(y), dash:has(dashList,y)?1:0, kind:kind, hemi:southStar?1:0};
}
function marksOf(plate, arcCustom, south){
  var m=[], i;
  if(plate.y){
    for(i=0;i<plate.y.length;i++){
      m.push(yearMark(plate.y[i], false, "c", plate.dash));
      if(plate.south) m.push(yearMark(plate.y[i], true, "c", plate.dash));
    }
  }
  if(plate.tick){
    for(i=0;i<plate.tick.length;i++) m.push(yearMark(plate.tick[i], false, "t", null));
  }
  if(plate.a){
    for(i=0;i<plate.a.length;i++) m.push({arc:plate.a[i], name:prime(plate.a[i]), dash:0, kind:"c", hemi:2});
  }
  if(plate.as){
    for(i=0;i<plate.as.length;i++) m.push({arc:plate.as[i], name:prime(plate.as[i]), dash:0, kind:"c", hemi:1});
  }
  if(plate.line){
    for(i=0;i<plate.line.length;i++) m.push({arc:plate.line[i], name:prime(plate.line[i]), dash:0, kind:"l", hemi:2});
  }
  if(plate.ey) m.push(yearMark(plate.ey, south, "c", null));
  if(plate.custom) m.push({arc:arcCustom, name:prime(arcCustom), dash:0, kind:"c", hemi:2});
  return m;
}
function pt(cx,cy,rpx,ha){
  var ang=(ha*15+90)*Math.PI/180;
  return [cx-rpx*Math.cos(ang), cy+rpx*Math.sin(ang)];
}
function between(arc, marks, south){
  var s=[], i;
  for(i=0;i<marks.length;i++){
    var h=marks[i].hemi;
    if(h===2||h===(south?1:0)) s.push(marks[i]);
  }
  s.sort(function(a,b){return a.arc-b.arc;});
  for(i=0;i<s.length;i++) if(Math.abs(s[i].arc-arc)<0.35) return "on "+s[i].name;
  var lo=null, hi=null;
  for(i=0;i<s.length;i++){
    if(s[i].arc<arc) lo=s[i];
    if(s[i].arc>arc && hi===null) hi=s[i];
  }
  if(lo&&hi) return "between "+hi.name+" and "+lo.name;
  if(hi) return "inside "+hi.name;
  if(lo) return "outside "+lo.name;
  return "";
}
function savePlate(){
  try{localStorage.setItem("taReticle2", String(idx)); localStorage.setItem("taRetArc", String(custom));}catch(e){}
}
function view(){
  if(manual) return manual;
  if(!st||!st.ok) return null;
  var L=splitIso(st.local), U=splitIso(st.utc);
  if(!L||!U) return null;
  return {lat:st.lat, lonE:st.lon, tz:st.tz, lst:st.lst,
    ly:L.y, lm:L.m, ld:L.d, lh:L.h, lmi:L.mi, ls:L.s,
    uy:U.y, um:U.m, ud:U.d, uhf:U.h+U.mi/60+U.s/3600};
}
function fillForm(){
  var v=view(); if(!v) return;
  var dt=document.getElementById("dt");
  var lat=document.getElementById("lat");
  var lon=document.getElementById("lon");
  if(v.ly) dt.value=iso(v.ly,v.lm,v.ld,v.lh,v.lmi,v.ls);
  if(v.lat!==null&&v.lat!==undefined) lat.value=v.lat;
  if(v.lonE!==null&&v.lonE!==undefined) lon.value=(-v.lonE).toFixed(5);
}
function render(){
  var cv=document.getElementById("cv");
  var dpr=window.devicePixelRatio||1;
  var w=cv.clientWidth||600, h=cv.clientHeight||480;
  cv.width=Math.round(w*dpr); cv.height=Math.round(h*dpr);
  var ctx=cv.getContext("2d");
  ctx.setTransform(dpr,0,0,dpr,0,0);
  ctx.fillStyle="#12141a"; ctx.fillRect(0,0,w,h);
  var seed=1;
  function rnd(){seed=(seed*16807)%2147483647; return (seed-1)/2147483646;}
  var i;
  ctx.fillStyle="#c5d0dc";
  for(i=0;i<80;i++){
    ctx.globalAlpha=0.12+rnd()*0.4;
    ctx.fillRect(rnd()*w, rnd()*h, 1.3, 1.3);
  }
  ctx.globalAlpha=1;
  var v=view();
  var meta=document.getElementById("meta");
  var lt=document.getElementById("lt");
  var plate=PL[idx]||PL[0];
  var have=!!(v&&v.lat!==null&&v.lat!==undefined&&v.lonE!==null&&v.lonE!==undefined);
  if(v) lt.textContent=pad(v.lh)+":"+pad(v.lmi)+":"+pad(v.ls);
  else lt.textContent="--:--:--";
  var south=have&&v.lat<0;
  var noStar=!have||(south&&!plate.oct&&(plate.north||(plate.y&&!plate.south)));
  var ut=null, jd=2451545, lst=0, arc=0, ha=0;
  if(v){
    ut=manual?toUt(v.ly,v.lm,v.ld,v.lh,v.lmi,v.ls,v.tz):{y:v.uy,m:v.um,d:v.ud,hf:v.uhf};
    jd=jdOf(ut.y,ut.m,ut.d,ut.hf);
  }
  if(have){
    lst=(manual||v.lst===null||v.lst===undefined)?lstOf(jd,v.lonE):v.lst;
    var cat=south?OCT:POL;
    var star=precess(cat.ra,cat.dec,jd);
    arc=polMin(star.dec);
    ha=norm24(lst-star.ra);
  }
  var marks=marksOf(plate, custom, south);
  var fit=1, k;
  for(k=0;k<marks.length;k++) if(marks[k].arc>fit) fit=marks[k].arc;
  if(have&&!noStar&&arc>fit) fit=arc;
  if(have&&south&&plate.oct){
    for(k=0;k<OCTC.length;k++){
      var oa=polMin(precess(OCTC[k].ra,OCTC[k].dec,jd).dec);
      if(oa>fit) fit=oa;
    }
  }
  var minS=Math.min(w,h);
  var ppm=0.42*minS/fit;
  var cx=w/2, cy=h/2;
  ctx.strokeStyle="#ff3b30"; ctx.lineWidth=1; ctx.fillStyle="#ff3b30";
  ctx.beginPath();
  ctx.moveTo(cx, cy-0.46*minS); ctx.lineTo(cx, cy+0.46*minS);
  ctx.moveTo(cx-0.46*minS, cy); ctx.lineTo(cx+0.46*minS, cy);
  ctx.stroke();
  ctx.beginPath(); ctx.arc(cx,cy,3,0,6.3); ctx.stroke();
  function strokeCircle(rpx, dash){
    ctx.beginPath(); ctx.setLineDash(dash?[5,4]:[]);
    ctx.arc(cx,cy,rpx,0,6.3); ctx.stroke(); ctx.setLineDash([]);
  }
  for(k=0;k<marks.length;k++){
    var mk=marks[k], rpx=mk.arc*ppm;
    if(mk.kind==="c"){
      strokeCircle(rpx, mk.dash);
      var lab=pt(cx,cy,rpx,6);
      ctx.font="12px sans-serif"; ctx.textAlign="left"; ctx.textBaseline="middle";
      ctx.fillText(mk.name, lab[0]+4, lab[1]);
    } else if(mk.kind==="t"){
      var hrs=[12,6,0,18], t;
      for(t=0;t<hrs.length;t++){
        var a1=pt(cx,cy,rpx-5,hrs[t]), a2=pt(cx,cy,rpx+5,hrs[t]);
        ctx.beginPath(); ctx.moveTo(a1[0],a1[1]); ctx.lineTo(a2[0],a2[1]); ctx.stroke();
      }
      var up=pt(cx,cy,rpx,12);
      ctx.font="11px sans-serif"; ctx.textAlign="left"; ctx.textBaseline="middle";
      ctx.fillText(mk.name, up[0]+6, up[1]);
    } else if(mk.kind==="l"){
      var lp=pt(cx,cy,rpx,12);
      ctx.beginPath(); ctx.moveTo(lp[0]-7,lp[1]); ctx.lineTo(lp[0]+7,lp[1]); ctx.stroke();
      ctx.font="12px sans-serif"; ctx.textAlign="left"; ctx.textBaseline="middle";
      ctx.fillText(mk.name, lp[0]+8, lp[1]);
    }
  }
  var outer=0;
  for(k=0;k<marks.length;k++) if(marks[k].kind==="c" && marks[k].arc>outer) outer=marks[k].arc;
  if(outer<1) outer=fit;
  var Ro=outer*ppm;
  function tick(ha0,len){
    var p1=pt(cx,cy,Ro,ha0), p2=pt(cx,cy,Ro+len,ha0);
    ctx.beginPath(); ctx.moveTo(p1[0],p1[1]); ctx.lineTo(p2[0],p2[1]); ctx.stroke();
  }
  ctx.font="14px sans-serif"; ctx.textAlign="center"; ctx.textBaseline="middle";
  var dial=plate.d, step, labelHa;
  if(dial===1){
    for(step=0;step<12;step++) tick(12-step*2, step%3===0?10:5);
    var L12=["0","3","6","9"], H12=[12,6,0,18];
    for(step=0;step<4;step++){
      var q=pt(cx,cy,Ro+20,H12[step]);
      ctx.fillText(L12[step], q[0], q[1]);
    }
  } else if(dial===2||dial===3){
    var ntick=dial===2?72:48;
    var dha=dial===2?(1/3):0.5;
    for(step=0;step<ntick;step++) tick(norm24(12-step*dha), (step%(dial===2?6:4)===0)?9:4);
    for(step=1;step<=12;step++){
      labelHa=norm24(12-2*(step%12));
      var cq=pt(cx,cy,Ro+20,labelHa);
      ctx.fillText(String(step===12?12:step), cq[0], cq[1]);
    }
  } else {
    for(step=0;step<24;step++) tick(step, step%6===0?11:5);
    var L24=["12","6","0","18"], H24=[12,6,0,18];
    for(step=0;step<4;step++){
      var q2=pt(cx,cy,Ro+22,H24[step]);
      ctx.fillText(L24[step], q2[0], q2[1]);
    }
  }
  if(have&&!noStar){
    var sp=pt(cx,cy,arc*ppm,ha);
    ctx.strokeStyle="#3dcc6e"; ctx.setLineDash([5,3]);
    ctx.beginPath(); ctx.moveTo(cx,cy); ctx.lineTo(sp[0],sp[1]); ctx.stroke();
    ctx.setLineDash([]);
    ctx.fillStyle="#fff"; ctx.beginPath(); ctx.arc(sp[0],sp[1],3.4,0,6.3); ctx.fill();
  }
  function drawFig(stars, links){
    var spf=[], i, ra0=0, dec0=0;
    for(i=0;i<stars.length;i++){
      var p=precess(stars[i].ra, stars[i].dec, jd);
      spf.push(p); ra0+=p.ra; dec0+=p.dec;
    }
    ra0/=spf.length; dec0/=spf.length;
    var ang=(norm24(lst-ra0)*15+90)*Math.PI/180;
    var rx=-Math.cos(ang), ry=Math.sin(ang);
    var ex=-ry, ey=rx;
    var pts=[], max=0.2;
    for(i=0;i<spf.length;i++){
      var dRa=(spf[i].ra-ra0)*15*Math.cos(dec0*Math.PI/180);
      var dDec=spf[i].dec-dec0;
      pts.push([dRa, dDec]);
      if(Math.abs(dRa)>max) max=Math.abs(dRa);
      if(Math.abs(dDec)>max) max=Math.abs(dDec);
    }
    var box=Math.max(18, Math.min(34, 0.07*minS));
    var sc=box/max;
    var rad=Math.min(Ro+box+12, Math.min(w,h)*0.48-box);
    if(rad<Ro+6) rad=Ro+6;
    var ox=cx+rx*rad, oy=cy+ry*rad;
    function xy(u,vv){return [ox+ex*u*sc+rx*vv*sc, oy+ey*u*sc+ry*vv*sc];}
    ctx.strokeStyle="#9aa3ad"; ctx.fillStyle="#f4f7fb"; ctx.lineWidth=1;
    ctx.beginPath();
    for(i=0;i<links.length;i++){
      var A=xy(pts[links[i][0]][0], pts[links[i][0]][1]);
      var B=xy(pts[links[i][1]][0], pts[links[i][1]][1]);
      ctx.moveTo(A[0], A[1]); ctx.lineTo(B[0], B[1]);
    }
    ctx.stroke();
    for(i=0;i<pts.length;i++){
      var Q=xy(pts[i][0], pts[i][1]);
      ctx.beginPath(); ctx.arc(Q[0], Q[1], 2.2, 0, 6.3); ctx.fill();
    }
  }
  if(have&&!south&&plate.fig){drawFig(DIP, DIPL); drawFig(CAS, CASL);}
  if(have&&south&&plate.oct){
    var oi, ops=[];
    ctx.fillStyle="#fff";
    for(oi=0;oi<OCTC.length;oi++){
      var os=precess(OCTC[oi].ra, OCTC[oi].dec, jd);
      var op=pt(cx,cy,polMin(os.dec)*ppm, norm24(lst-os.ra));
      ops.push(op);
      ctx.beginPath(); ctx.arc(op[0], op[1], 2.4, 0, 6.3); ctx.fill();
    }
    ctx.strokeStyle="#9aa3ad"; ctx.lineWidth=1; ctx.beginPath();
    ctx.moveTo(ops[0][0], ops[0][1]);
    ctx.lineTo(ops[1][0], ops[1][1]);
    ctx.lineTo(ops[2][0], ops[2][1]);
    ctx.closePath(); ctx.stroke();
  }
  var lines;
  if(!v){
    lines="Waiting for the mount clock.";
  } else if(!have){
    lines=v.ly+"-"+pad(v.lm)+"-"+pad(v.ld);
    lines+="<br>The site is not set, so the pole star stays off the glass.";
  } else {
    var who=south?"Sigma Octantis":"Polaris";
    var place=between(arc, marks, south);
    lines=v.ly+"-"+pad(v.lm)+"-"+pad(v.ld);
    lines+="<br>UT "+hms(ut.hf);
    lines+="<br>JD "+jd.toFixed(5);
    lines+="<br>Latitude "+(v.lat>=0?"+":"")+v.lat.toFixed(4);
    lines+="<br>Longitude (E - / W +) "+(-v.lonE>=0?"+":"")+(-v.lonE).toFixed(4);
    lines+="<br>Hour angle "+hms(ha);
    if(!noStar) lines+="<br>"+who+" "+arc.toFixed(1)+"\u2032 from the pole"+(place?", "+place:"");
    lines+="<br>"+(dial===1?"Put 0 at the top.":"Put 12 at the top.");
    if(!south&&plate.fig) lines+="<br>Big Dipper and Cassiopeia, as on the glass.";
    if(south&&plate.oct) lines+="<br>Octans stars at their true distance from the pole.";
    if(noStar) lines+="<br><span class='ret-note'>This plate has no southern circles.</span>";
  }
  meta.innerHTML=lines;
}
function applyManual(){
  if(!st) return false;
  var dt=document.getElementById("dt").value;
  var lat=parseFloat(document.getElementById("lat").value);
  var lonW=parseFloat(document.getElementById("lon").value);
  var L=splitIso(dt.length===16?dt+":00":dt);
  if(!L||!(lat<=90&&lat>=-90)||!(lonW<=180&&lonW>=-180)) return false;
  var tz=st.tz;
  manual={lat:lat, lonE:-lonW, tz:tz, lst:null,
    ly:L.y, lm:L.m, ld:L.d, lh:L.h, lmi:L.mi, ls:L.s};
  render();
  return true;
}
function getJson(url, cb, onDone){
  var x=new XMLHttpRequest();
  x.open("GET", url, true);
  x.timeout=4000;
  function finish(){ if(onDone) onDone(); }
  x.onload=function(){ finish(); if(x.status>=200&&x.status<300){try{cb(JSON.parse(x.responseText));}catch(e){}} };
  x.onerror=finish;
  x.ontimeout=finish;
  x.send();
}
function poll(){
  if(busy) return;
  busy=true;
  getJson("/reticule.txt?x="+Date.now(), function(j){
    if(!j||!j.ok) return;
    st=j;
    if(!manual){ if(!editOn) fillForm(); render(); }
  }, function(){ busy=false; });
}
function bright(v){
  getJson("/reticule.txt?b="+v+"&x="+Date.now(), function(j){
    if(!j||!j.ok) return;
    st=j;
    if(!manual){fillForm(); render();}
  });
}
function boot(){
  var sel=document.getElementById("plate");
  var i;
  for(i=0;i<PL.length;i++){
    var o=document.createElement("option");
    o.value=String(i); o.textContent=PL[i].n;
    if(i===idx) o.selected=true;
    sel.appendChild(o);
  }
  document.getElementById("arcmin").value=String(custom);
  function showCustom(){
    document.getElementById("customBox").className=PL[idx].custom?"":"ret-hide";
  }
  showCustom();
  sel.onchange=function(){
    var n=parseInt(sel.value,10);
    idx=(n>=0&&n<PL.length)?n:0;
    savePlate(); showCustom(); render();
  };
  document.getElementById("arcmin").onchange=function(){
    var v=parseFloat(this.value);
    if(v>=20&&v<=80){custom=v; savePlate(); render();}
  };
  document.getElementById("bEdit").onclick=function(){
    editOn=true; fillForm();
    document.getElementById("disp").className="ret-hide";
    document.getElementById("edit").className="";
  };
  document.getElementById("bNow").onclick=function(){
    manual=null; editOn=false;
    document.getElementById("edit").className="ret-hide";
    document.getElementById("disp").className="";
    fillForm(); render();
  };
  document.getElementById("bBack").onclick=document.getElementById("bNow").onclick;
  document.getElementById("bSub").onclick=function(){
    if(!applyManual()) return;
    editOn=false;
    document.getElementById("edit").className="ret-hide";
    document.getElementById("disp").className="";
  };
  document.getElementById("bPlus").onclick=function(){bright(1);};
  document.getElementById("bMinus").onclick=function(){bright(0);};
  window.addEventListener("resize", render);
  render();
  poll();
  setInterval(poll, 1000);
}
boot();
)raw";

static double s_retLat = 0;
static double s_retLonWest = 0;
static bool s_retSiteOk = false;
static unsigned long s_retSiteMs = 0;

static void refreshReticuleSite()
{
  unsigned long now = millis();
  if (s_retSiteOk && (now - s_retSiteMs) < 30000UL)
    return;
  double lat = 0, lon = 0;
  if (TeenAstroWifi::client().getLatitude(lat) == LX200_VALUEGET &&
      TeenAstroWifi::client().getLongitude(lon) == LX200_VALUEGET)
  {
    s_retLat = lat;
    s_retLonWest = lon;
    s_retSiteOk = true;
    s_retSiteMs = now;
  }
}

void TeenAstroWifi::reticuleAjax()
{
  s_client->setTimeout(WebTimeout);
  String b = server.arg("b");
  if (b == "1" || b == "0")
    s_client->reticuleBrightnessAdjust(b == "1");

  ta_MountStatus.updateAllState();
  if (!ta_MountStatus.hasInfoUTC())
  {
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, "application/json", "{\"ok\":0}");
    return;
  }
  refreshReticuleSite();

  uint8_t lh = 0, lm = 0, ls = 0, lmo = 1, ld = 1, ly = 0;
  bool hasLocal = ta_MountStatus.getLocalTimeCached(lh, lm, ls) &&
                  ta_MountStatus.getLocalDateCached(lmo, ld, ly);
  if (!hasLocal)
  {
    lh = ta_MountStatus.getUtcHour();
    lm = ta_MountStatus.getUtcMin();
    ls = ta_MountStatus.getUtcSec();
    lmo = ta_MountStatus.getUtcMonth();
    ld = ta_MountStatus.getUtcDay();
    ly = ta_MountStatus.getUtcYear();
  }
  const float tz = ta_MountStatus.hasTimezone() ? -ta_MountStatus.getTimezoneOffset() : 0.f;
  const unsigned utcY = 2000u + ta_MountStatus.getUtcYear();
  const unsigned locY = 2000u + ly;
  const bool hasLst = ta_MountStatus.hasInfoSidereal();
  const float lst = hasLst ? ta_MountStatus.getLstHoursCached() : 0.f;

  char buf[420];
  int n = snprintf(buf, sizeof(buf), "{\"ok\":1,");
  auto room = [&]() { return n > 0 && n < (int)sizeof(buf); };
  if (room())
  {
    if (s_retSiteOk)
      n += snprintf(buf + n, sizeof(buf) - (size_t)n, "\"lat\":%.5f,\"lon\":%.5f,", s_retLat, -s_retLonWest);
    else
      n += snprintf(buf + n, sizeof(buf) - (size_t)n, "\"lat\":null,\"lon\":null,");
  }
  if (room())
    n += snprintf(buf + n, sizeof(buf) - (size_t)n,
      "\"utc\":\"%04u-%02u-%02uT%02u:%02u:%02u\",\"tz\":%.1f,"
      "\"local\":\"%04u-%02u-%02uT%02u:%02u:%02u\",",
      utcY, (unsigned)ta_MountStatus.getUtcMonth(), (unsigned)ta_MountStatus.getUtcDay(),
      (unsigned)ta_MountStatus.getUtcHour(), (unsigned)ta_MountStatus.getUtcMin(),
      (unsigned)ta_MountStatus.getUtcSec(),
      tz,
      locY, (unsigned)lmo, (unsigned)ld, (unsigned)lh, (unsigned)lm, (unsigned)ls);
  if (room())
  {
    if (hasLst)
      n += snprintf(buf + n, sizeof(buf) - (size_t)n, "\"lst\":%.5f}", lst);
    else
      n += snprintf(buf + n, sizeof(buf) - (size_t)n, "\"lst\":null}");
  }

  server.sendHeader("Cache-Control", "no-cache");
  if (n > 0 && n < (int)sizeof(buf))
    server.send(200, "application/json", buf);
  else
    server.send(200, "application/json", "{\"ok\":0}");
}

void TeenAstroWifi::handleReticule()
{
  if (busyGuard()) return;
  s_client->setTimeout(WebTimeout);
  ta_MountStatus.updateAllState();

  sendHtmlStart();
  String data;
  preparePage(data, ServerPage::Reticule);
  sendHtml(data);

  const TeenAstroMountStatus::Mount mt = ta_MountStatus.getMount();
  const bool equatorial = (mt == TeenAstroMountStatus::MOUNT_TYPE_GEM ||
                           mt == TeenAstroMountStatus::MOUNT_TYPE_FORK);
  if (!equatorial)
  {
    data += "<div class='card'><p>The polar finder is for an equatorial mount.</p></div>";
    data += FPSTR(html_pageFooter);
    sendHtml(data);
    sendHtmlDone(data);
    s_handlerBusy = false;
    return;
  }

  data += FPSTR(html_reticule_style);
  sendHtml(data);
  data += "<script>";
  sendHtml(data);
  {
    char buf[768];
    size_t n = strlen_P(html_reticule_js);
    for (size_t i = 0; i < n; )
    {
      size_t c = n - i;
      if (c > sizeof(buf) - 1) c = sizeof(buf) - 1;
      memcpy_P(buf, html_reticule_js + i, c);
      buf[c] = 0;
      server.sendContent(buf);
      i += c;
      yield();
    }
  }
  data += "</script>";
  data += FPSTR(html_pageFooter);
  sendHtml(data);
  sendHtmlDone(data);
  s_handlerBusy = false;
}
