#pragma once

// Javascript for Ajax
// be sure to define "var ajaxPage='control.txt';" etc.

#define DEFAULT_AJAX_RATE "5"        // normally 5 seconds between updates
#define DEFAULT_FAST_AJAX_RATE "1"   // fast update is 1 second/update
#define DEFAULT_AJAX_SHED_TIME "15"  // time before return to normal update rate

#define html_ajax_active "<script>\n\
var auto1Tick=0;\n\
var auto2Tick=0;\n\
var auto2Rate=" DEFAULT_AJAX_RATE ";\n\
var auto1=setInterval(autoRun1s,1000);\n\
function autoFastRun() {\n\
auto2Rate=" DEFAULT_FAST_AJAX_RATE "\n\
auto2Tick=" DEFAULT_AJAX_SHED_TIME ";\n\
}\n\
function autoRun1s() {\n\
auto1Tick++;\n\
var i;\n\
if (auto2Tick>=0) auto2Tick--;\n\
if (auto2Tick==0) auto2Rate=" DEFAULT_AJAX_RATE ";\n\
if (auto1Tick%auto2Rate==0) {\n\
nocache='?nocache='+Math.random()*1000000;\n\
var request = new XMLHttpRequest();\n\
request.onreadystatechange = pageReady(ajaxPage);\n\
request.open('GET',ajaxPage.toLowerCase()+nocache,true); request.send(null);\n\
}\
}\n\
function pageReady(aPage) {\n\
return function() {\n\
if ((this.readyState==4)&&(this.status==200)) {\n\
lines=this.responseText.split('\\n');\n\
for (var i=0; i<lines.length; i++) {\n\
v=lines[i].slice(lines[i].indexOf('|')+1);\n\
k=lines[i].slice(0,lines[i].indexOf('|'));\n\
if (k!='') document.getElementById(k).innerHTML=v;\n\
}\n\
}\
}\
}\n\
</script>\n"
