#include <Arduino.h>
#include <QTRSensors.h>
#include <WiFi.h>
#include <WebServer.h>

// ===================== WIFI AP =====================

const char* AP_SSID = "Robot-Calibrador";
const char* AP_PASS = "12345678";
WebServer server(80);

// ===================== SENSORES =====================

#define NUM_SENSORS 8
QTRSensors qtr;
uint16_t sensorValues[NUM_SENSORS];
#define PIN_EMISOR_IR 2

// ===================== MOTORES =====================

#define ENA  23
#define IN1  22
#define IN2  21
#define ENB  19
#define IN3  18
#define IN4   5
#define VEL_MAX 255

#define LEDC_FREQ      5000
#define LEDC_RES       8
#define LEDC_CANAL_IZQ 0
#define LEDC_CANAL_DER 1

// ===================== ESTADO =====================

bool     calibrado = false;
bool     motorOn   = false;
int      velocidad = 200;
int      offsetIzq = 0;
int      offsetDer = 0;
uint16_t posActual = 3500;
int      pasoMs    = 300;   // duración de cada paso (ms)

// ===================== MOTORES =====================

void moverMotores(int velIzq, int velDer)
{
  velIzq = constrain(velIzq + offsetIzq, -VEL_MAX, VEL_MAX);
  velDer = constrain(velDer + offsetDer, -VEL_MAX, VEL_MAX);

  digitalWrite(IN1, velIzq >= 0 ? HIGH : LOW);
  digitalWrite(IN2, velIzq >= 0 ? LOW  : HIGH);
  ledcWrite(LEDC_CANAL_IZQ, abs(velIzq));

  digitalWrite(IN3, velDer >= 0 ? HIGH : LOW);
  digitalWrite(IN4, velDer >= 0 ? LOW  : HIGH);
  ledcWrite(LEDC_CANAL_DER, abs(velDer));

  motorOn = true;
}

void detener()
{
  ledcWrite(LEDC_CANAL_IZQ, 0);
  ledcWrite(LEDC_CANAL_DER, 0);
  motorOn = false;
}

// ===================== CALIBRACIÓN =====================

void calibrarSensores()
{
  bool motoresEstaban = motorOn;
  if (motorOn) detener();
  calibrado = false;

  Serial.println(F(">> Calibrando (3s)..."));
  for (int i = 0; i < 300; i++)
  {
    qtr.calibrate();
    delay(10);
    if (i % 10 == 0) server.handleClient(); // mantener web activa
  }
  calibrado = true;
  posActual = qtr.readLineBlack(sensorValues);
  Serial.println(F(">> Calibración completada"));

  if (motoresEstaban) moverMotores(velocidad, velocidad);
}

// ===================== TEST MOTORES =====================

void testMotores()
{
  Serial.println(F(">> TEST MOTORES"));
  moverMotores(velocidad, velocidad);  delay(2000); server.handleClient();
  detener();                           delay(1000); server.handleClient();
  moverMotores(-velocidad, -velocidad);delay(2000); server.handleClient();
  detener();                           delay(1000); server.handleClient();
  moverMotores(velocidad / 2, velocidad); delay(1000); server.handleClient();
  moverMotores(velocidad, velocidad / 2); delay(1000); server.handleClient();
  detener();
  Serial.println(F(">> Test completado"));
}

// ===================== COMANDO =====================

void ejecutarComando(String cmd)
{
  cmd.trim();
  cmd.toUpperCase();

  if      (cmd == "CAL")   { calibrarSensores(); }
  else if (cmd == "GO")    { moverMotores(velocidad, velocidad); }
  else if (cmd == "BACK")  { moverMotores(-velocidad, -velocidad); }
  else if (cmd == "MSTOP") { detener(); }
  else if (cmd == "STOP")  { detener(); }
  else if (cmd == "TEST")  { testMotores(); }
  else if (cmd == "L+") { offsetIzq = constrain(offsetIzq + 5, -127, 127); if (motorOn) moverMotores(velocidad, velocidad); }
  else if (cmd == "L-") { offsetIzq = constrain(offsetIzq - 5, -127, 127); if (motorOn) moverMotores(velocidad, velocidad); }
  else if (cmd == "R+") { offsetDer = constrain(offsetDer + 5, -127, 127); if (motorOn) moverMotores(velocidad, velocidad); }
  else if (cmd == "R-") { offsetDer = constrain(offsetDer - 5, -127, 127); if (motorOn) moverMotores(velocidad, velocidad); }
  else if (cmd == "RESET_OFF") { offsetIzq = 0; offsetDer = 0; if (motorOn) moverMotores(velocidad, velocidad); }
  else if (cmd == "STEP_FWD")  { moverMotores( velocidad,  velocidad); delay(pasoMs); detener(); }
  else if (cmd == "STEP_BACK") { moverMotores(-velocidad, -velocidad); delay(pasoMs); detener(); }
  else if (cmd == "STEP_LEFT") { moverMotores(-velocidad,  velocidad); delay(pasoMs); detener(); }
  else if (cmd == "STEP_RIGHT"){ moverMotores( velocidad, -velocidad); delay(pasoMs); detener(); }
  else if (cmd.startsWith("V "))
  {
    velocidad = constrain(cmd.substring(2).toInt(), 0, 255);
    if (motorOn) moverMotores(velocidad, velocidad);
  }
  else if (cmd.startsWith("PASO "))
  {
    pasoMs = constrain(cmd.substring(5).toInt(), 50, 2000);
  }
}

// ===================== WEB PAGE =====================

static const char HTML_PAGE[] = R"rawhtml(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Robot Calibrador</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:monospace;background:#111;color:#0f0;padding:12px;max-width:480px;margin:auto}
h2{color:#0f0;border-bottom:1px solid #333;padding:6px 0;margin:14px 0 8px;font-size:15px}
.status{background:#1a1a1a;border:1px solid #333;padding:8px;border-radius:4px;margin:8px 0;font-size:13px;line-height:1.8}
.row{display:flex;flex-wrap:wrap;gap:6px;margin:6px 0}
button{background:#1a1a1a;color:#0f0;border:1px solid #0f0;padding:13px 14px;border-radius:4px;font-size:15px;cursor:pointer;font-family:monospace;flex:1}
button:active{background:#0f0;color:#000}
.red{border-color:#f44;color:#f44;margin-top:8px;width:100%}
.red:active{background:#f44;color:#000}
.sensors{display:flex;gap:3px;height:80px;align-items:flex-end;margin:6px 0;background:#0a0a0a;padding:4px;border-radius:4px}
.s{flex:1;border-radius:2px 2px 0 0;min-height:2px;transition:height .15s;background:#333}
.s.on{background:#0f0}
.lbls{display:flex;gap:3px}
.lbl{flex:1;text-align:center;font-size:10px;color:#555}
.track{height:14px;background:#1a1a1a;border-radius:7px;margin:6px 0;position:relative;border:1px solid #333}
.dot{width:14px;height:14px;background:#0f0;border-radius:50%;position:absolute;top:0;transition:left .15s;margin-left:-7px}
.pos{font-size:13px;margin:4px 0}
.err{color:#f80}
.vel-row{display:flex;align-items:center;gap:10px;margin:6px 0}
input[type=range]{flex:1;accent-color:#0f0;height:6px}
.vv{min-width:28px;text-align:right}
.dpad{display:grid;grid-template-columns:1fr 1fr 1fr;grid-template-rows:1fr 1fr 1fr;gap:6px;width:180px;margin:8px auto}
.dpad button{padding:16px 0;font-size:20px}
.dpad .mid{display:flex;align-items:center;justify-content:center;color:#333;font-size:12px}
</style>
</head>
<body>
<h2>&#9881; Robot Calibrador WiFi</h2>

<div class="status" id="st">Conectando...</div>

<h2>Sensores</h2>
<div class="lbls" id="lbls"></div>
<div class="sensors" id="bars"></div>
<div class="pos" id="pos">POS: -- &nbsp; ERR: --</div>
<div class="track"><div class="dot" id="dot" style="left:50%"></div></div>

<h2>Calibracion</h2>
<div class="row">
  <button onclick="cmd('CAL')" style="flex:2">&#9889; CAL (3s)</button>
  <button onclick="cmd('READ')" style="flex:1">READ</button>
</div>

<h2>Motores</h2>
<div class="row">
  <button onclick="cmd('GO')">&#9654; GO</button>
  <button onclick="cmd('BACK')">&#9664; BACK</button>
  <button onclick="cmd('MSTOP')">&#9646;&#9646; STOP</button>
</div>
<div class="row"><button onclick="cmd('TEST')">TEST secuencia</button></div>

<h2>Paso a paso</h2>
<div class="dpad">
  <div></div>
  <button onclick="cmd('STEP_FWD')">&#9650;</button>
  <div></div>
  <button onclick="cmd('STEP_LEFT')">&#9664;</button>
  <div class="mid">PASO</div>
  <button onclick="cmd('STEP_RIGHT')">&#9654;</button>
  <div></div>
  <button onclick="cmd('STEP_BACK')">&#9660;</button>
  <div></div>
</div>
<div class="vel-row" style="margin-top:6px">
  <span style="font-size:12px;color:#888">Duracion:</span>
  <input type="range" id="paso" min="50" max="2000" value="300" oninput="onPaso(this.value)">
  <span class="vv" id="pv">300ms</span>
</div>

<h2>Velocidad</h2>
<div class="vel-row">
  <input type="range" id="vel" min="0" max="255" value="150" oninput="onVel(this.value)">
  <span class="vv" id="vv">150</span>
</div>

<h2>Offset IZQ / DER</h2>
<div class="status" id="offst">IZQ: 0 &rarr; 0 &nbsp; DER: 0 &rarr; 0</div>
<div class="row">
  <button onclick="cmd('L+')">IZQ +5</button>
  <button onclick="cmd('L-')">IZQ -5</button>
  <button onclick="cmd('R+')">DER +5</button>
  <button onclick="cmd('R-')">DER -5</button>
</div>
<div class="row"><button onclick="cmd('RESET_OFF')" style="border-color:#f80;color:#f80">Reset offsets</button></div>

<button class="red" onclick="cmd('STOP')">&#9646;&#9646; STOP TODO</button>

<script>
let vt;
(function(){
  let l='',b='';
  for(let i=1;i<=8;i++){
    l+='<div class="lbl">S'+i+'</div>';
    b+='<div class="s" id="s'+i+'" style="height:2px"></div>';
  }
  document.getElementById('lbls').innerHTML=l;
  document.getElementById('bars').innerHTML=b;
})();

function cmd(c){fetch('/cmd?c='+encodeURIComponent(c)).catch(()=>{});}

function onVel(v){
  document.getElementById('vv').textContent=v;
  clearTimeout(vt);
  vt=setTimeout(()=>cmd('V '+v),400);
}

let pt;
function onPaso(v){
  document.getElementById('pv').textContent=v+'ms';
  clearTimeout(pt);
  pt=setTimeout(()=>cmd('PASO '+v),400);
}

function tick(){
  fetch('/status').then(r=>r.json()).then(d=>{
    document.getElementById('st').innerHTML=
      '<b>Cal:</b> '+(d.cal?'<span style="color:#0f0">SI</span>':'<span style="color:#f44">NO</span>')+
      ' &nbsp;<b>Motor:</b> '+(d.mot?'<span style="color:#0f0">ON</span>':'OFF')+
      ' &nbsp;<b>Vel:</b> '+d.vel+
      '<br><b>IZQ:</b> '+d.izq+' &rarr; '+d.vi+
      ' &nbsp;<b>DER:</b> '+d.der+' &rarr; '+d.vd;

    for(let i=0;i<8;i++){
      let el=document.getElementById('s'+(i+1));
      let h=Math.max(2,Math.round(d.s[i]/25));
      el.style.height=h+'px';
      el.className='s'+(d.s[i]>400?' on':'');
    }

    if(d.cal){
      let err=d.pos-3500;
      document.getElementById('pos').innerHTML=
        'POS: '+d.pos+' &nbsp; ERR: <span class="err">'+(err>=0?'+':'')+err+'</span>';
      let pct=Math.max(2,Math.min(96,d.pos/7000*100));
      document.getElementById('dot').style.left=pct+'%';
    }

    document.getElementById('vel').value=d.vel;
    document.getElementById('vv').textContent=d.vel;
    document.getElementById('paso').value=d.paso;
    document.getElementById('pv').textContent=d.paso+'ms';
    document.getElementById('offst').innerHTML=
      'IZQ: <b>'+(d.izq>=0?'+':'')+d.izq+'</b> &rarr; '+d.vi+
      ' &nbsp;&nbsp; DER: <b>'+(d.der>=0?'+':'')+d.der+'</b> &rarr; '+d.vd;
  }).catch(()=>{});
}
setInterval(tick,300);
tick();
</script>
</body>
</html>
)rawhtml";

// ===================== HANDLERS =====================

void handleRoot()
{
  server.send(200, "text/html", HTML_PAGE);
}

void handleStatus()
{
  if (calibrado)
    posActual = qtr.readLineBlack(sensorValues);
  else
    qtr.read(sensorValues);

  int vi = constrain(velocidad + offsetIzq, 0, VEL_MAX);
  int vd = constrain(velocidad + offsetDer, 0, VEL_MAX);

  String json = "{";
  json += "\"cal\":"  + String(calibrado ? "true" : "false") + ",";
  json += "\"mot\":"  + String(motorOn   ? "true" : "false") + ",";
  json += "\"vel\":"  + String(velocidad) + ",";
  json += "\"izq\":"  + String(offsetIzq) + ",";
  json += "\"der\":"  + String(offsetDer) + ",";
  json += "\"vi\":"   + String(vi) + ",";
  json += "\"vd\":"   + String(vd) + ",";
  json += "\"paso\":"  + String(pasoMs) + ",";
  json += "\"pos\":"  + String(posActual) + ",";
  json += "\"s\":[";
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    json += String(sensorValues[i]);
    if (i < NUM_SENSORS - 1) json += ",";
  }
  json += "]}";

  server.send(200, "application/json", json);
}

void handleCmd()
{
  if (server.hasArg("c"))
  {
    String c = server.arg("c");
    server.send(200, "text/plain", "OK");
    ejecutarComando(c);
  }
  else
  {
    server.send(400, "text/plain", "falta param c");
  }
}

// ===================== SETUP =====================

void setup()
{
  Serial.begin(9600);
  delay(500);

  // Motores
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcSetup(LEDC_CANAL_IZQ, LEDC_FREQ, LEDC_RES);
  ledcAttachPin(ENA, LEDC_CANAL_IZQ);
  ledcSetup(LEDC_CANAL_DER, LEDC_FREQ, LEDC_RES);
  ledcAttachPin(ENB, LEDC_CANAL_DER);
  detener();

  // Sensores
  pinMode(PIN_EMISOR_IR, OUTPUT);
  digitalWrite(PIN_EMISOR_IR, HIGH);
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){16, 17, 25, 26, 32, 14, 4, 13}, NUM_SENSORS);
  qtr.setEmitterPin(PIN_EMISOR_IR);

  // WiFi AP
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  Serial.print(F("\nWiFi AP: ")); Serial.println(AP_SSID);
  Serial.print(F("Pass:    ")); Serial.println(AP_PASS);
  Serial.print(F("URL:     http://")); Serial.println(ip);

  // Rutas web
  server.on("/",       handleRoot);
  server.on("/status", handleStatus);
  server.on("/cmd",    handleCmd);
  server.begin();

  Serial.println(F("Servidor web listo.\n"));
}

// ===================== LOOP =====================

void loop()
{
  server.handleClient();

  // Fallback Serial
  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    ejecutarComando(cmd);
  }
}
