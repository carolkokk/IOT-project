/* =========================
Preset Highlight
========================= */

function highlightPreset(value){

const items=document.querySelectorAll(".preset-item");

items.forEach(item=>{
item.classList.remove("active");

if(parseInt(item.dataset.value)===parseInt(value)){
item.classList.add("active");
}
});
}

/* =========================
Preset Click
========================= */

function applyPreset(value){

const input=document.getElementById("sp");

input.value=value;

sendSP();

highlightPreset(value);

}

/* =========================
Send Setpoint
========================= */

async function sendSP(){

const input=document.getElementById("sp");

const v=input.value;

if(v===""||isNaN(v)||v<30||v>70){
alert("Humidity must be between 30 and 70");
return;
}

await fetch('/setpoint',{
method:'POST',
headers:{'Content-Type':'application/json'},
body:JSON.stringify({setpoint:v})
});

/* 清空输入框 */

input.value="";

highlightPreset(v);

}

/* =========================
Enter Key Submit
========================= */

document.addEventListener("DOMContentLoaded",function(){

const input=document.getElementById("sp");

if(input){

input.addEventListener("keydown",function(event){

if(event.key==="Enter"){
sendSP();
}

});

}

});

/* =========================
Update Setpoint
========================= */

async function updateSetpoint(){

const res=await fetch('/getsetpoint');

const data=await res.json();

document.getElementById("sp-display").innerText=data.setpoint;

highlightPreset(data.setpoint);

}

/* =========================
Chart Initialization
========================= */

const ctx=document.getElementById('thChart');

if(ctx){

const chart=new Chart(ctx,{
type:'line',
data:{
labels:[],
datasets:[
{label:'Humidity (%)',data:[],borderColor:'#2563eb',tension:0.3},
{label:'Temperature (°C)',data:[],borderColor:'#f59e0b',tension:0.3},
{label:'Upper Bound',data:[],borderColor:'rgba(34,197,94,0.6)',borderDash:[6,6],pointRadius:0},
{label:'Lower Bound',data:[],borderColor:'rgba(34,197,94,0.6)',backgroundColor:'rgba(34,197,94,0.1)',fill:'-1',pointRadius:0}
]
},
options:{animation:false}
});

/* =========================
Update Chart
========================= */

async function updateChart(){

const res=await fetch('/messages');

const data=await res.json();

const last=data.slice(-30);

const labels=last.map(d=>new Date(d.t).toLocaleTimeString());

const humData=last.map(d=>d.hum);

const tempData=last.map(d=>d.temp);

chart.data.labels=labels;

chart.data.datasets[0].data=humData;
chart.data.datasets[1].data=tempData;

const sp=parseFloat(document.getElementById("sp-display").innerText);

if(!isNaN(sp)){

chart.data.datasets[2].data=Array(labels.length).fill(sp+5);
chart.data.datasets[3].data=Array(labels.length).fill(sp-5);

}else{

chart.data.datasets[2].data=[];
chart.data.datasets[3].data=[];
}

chart.update();
}

/* =========================
Summary
========================= */

async function updateSummary(){

const panel=document.getElementById("summary-panel");

if(!panel) return;

const res=await fetch('/messages');

const data=await res.json();

let maxTemp='<span class="placeholder">/</span>';
let minTemp='<span class="placeholder">/</span>';
let maxHum='<span class="placeholder">/</span>';
let minHum='<span class="placeholder">/</span>';
let alarmStatus='<span class="placeholder">/</span>';

if(data.length>0){

const last=data.slice(-30);

const temps=last.map(d=>d.temp);
const hums=last.map(d=>d.hum);
const alarms=last.map(d=>d.alarm);

maxTemp=Math.max(...temps).toFixed(1)+" °C";
minTemp=Math.min(...temps).toFixed(1)+" °C";
maxHum=Math.max(...hums).toFixed(1)+" %";
minHum=Math.min(...hums).toFixed(1)+" %";

const hasAlarm=alarms.some(a=>a==1);

alarmStatus=hasAlarm
?'<span class="warning-text">Warning Detected</span>'
:"No Warning Detected";

}

panel.innerHTML = `
<div class="summary-item">
<strong>Max Temperature</strong>
<div class="summary-value">${maxTemp}</div>
</div>

<div class="summary-item">
<strong>Min Temperature</strong>
<div class="summary-value">${minTemp}</div>
</div>

<div class="summary-item">
<strong>Max Humidity</strong>
<div class="summary-value">${maxHum}</div>
</div>

<div class="summary-item">
<strong>Min Humidity</strong>
<div class="summary-value">${minHum}</div>
</div>

<div class="summary-item">
<strong>Alarm Status</strong>
<div class="summary-value">${alarmStatus}</div>
</div>
`;

}

/* =========================
Init Dashboard
========================= */

updateChart();
updateSummary();
updateSetpoint();

setInterval(updateChart,5000);
setInterval(updateSummary,5000);
setInterval(updateSetpoint,5000);

}