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
var st=null, manual=null, busy=false, editOn=false;
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
    hf:u.getUTCHours()+u.getUTCMinutes()/60+u.getUTCSeconds()/3600};
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
  if(v.ly) document.getElementById("dt").value=iso(v.ly,v.lm,v.ld,v.lh,v.lmi,v.ls);
  if(v.lat!==null&&v.lat!==undefined) document.getElementById("lat").value=v.lat;
  if(v.lonE!==null&&v.lonE!==undefined) document.getElementById("lon").value=(-v.lonE).toFixed(5);
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
function pt(cx,cy,r,ha){
  var ang=(ha*15+90)*Math.PI/180;
  return [cx-r*Math.cos(ang), cy+r*Math.sin(ang)];
}
function render(){
  var cv=document.getElementById("cv");
  var dpr=window.devicePixelRatio||1;
  var w=cv.clientWidth||600, h=cv.clientHeight||480;
  cv.width=Math.round(w*dpr); cv.height=Math.round(h*dpr);
  var ctx=cv.getContext("2d");
  ctx.setTransform(dpr,0,0,dpr,0,0);
  ctx.fillStyle="#12141a"; ctx.fillRect(0,0,w,h);
  var v=view();
  var lt=document.getElementById("lt");
  var meta=document.getElementById("meta");
  var have=!!(v&&v.lat!==null&&v.lat!==undefined&&v.lonE!==null&&v.lonE!==undefined);
  if(v) lt.textContent=pad(v.lh)+":"+pad(v.lmi)+":"+pad(v.ls);
  else lt.textContent="--:--:--";
  var south=have&&v.lat<0;
  var arc=40, ha=0, ut=null, jd=0, lst=0;
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
  var minS=Math.min(w,h), ppm=0.42*minS/Math.max(arc,1);
  var cx=w/2, cy=h/2, Ro=arc*ppm, step;
  ctx.strokeStyle="#ff3b30"; ctx.lineWidth=1; ctx.fillStyle="#ff3b30";
  ctx.beginPath();
  ctx.moveTo(cx,cy-Ro-24); ctx.lineTo(cx,cy+Ro+24);
  ctx.moveTo(cx-Ro-24,cy); ctx.lineTo(cx+Ro+24,cy);
  ctx.stroke();
  ctx.beginPath(); ctx.arc(cx,cy,Ro,0,6.3); ctx.stroke();
  ctx.font="14px sans-serif"; ctx.textAlign="center"; ctx.textBaseline="middle";
  var labs=["12","6","0","18"], has=[12,6,0,18];
  for(step=0;step<24;step++){
    var a=pt(cx,cy,Ro,step), b=pt(cx,cy,Ro+(step%6===0?11:5),step);
    ctx.beginPath(); ctx.moveTo(a[0],a[1]); ctx.lineTo(b[0],b[1]); ctx.stroke();
  }
  for(step=0;step<4;step++){
    var q=pt(cx,cy,Ro+22,has[step]);
    ctx.fillText(labs[step], q[0], q[1]);
  }
  if(have){
    var sp=pt(cx,cy,Ro,ha);
    ctx.fillStyle="#fff"; ctx.beginPath(); ctx.arc(sp[0],sp[1],3.4,0,6.3); ctx.fill();
  }
  var lines;
  if(!v) lines="Waiting for the mount clock.";
  else if(!have) lines=v.ly+"-"+pad(v.lm)+"-"+pad(v.ld)+"<br>The site is not set, so the pole star stays off the glass.";
  else {
    lines=v.ly+"-"+pad(v.lm)+"-"+pad(v.ld);
    lines+="<br>UT "+hms(ut.hf);
    lines+="<br>"+(south?"Sigma Octantis":"Polaris")+" "+arc.toFixed(1)+"\u2032 from the pole";
    lines+="<br>Hour angle "+hms(ha)+"<br>Put 12 at the top.";
  }
  meta.innerHTML=lines;
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
