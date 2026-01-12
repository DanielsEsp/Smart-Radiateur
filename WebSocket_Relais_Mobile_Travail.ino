



#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Preferences.h>

#define RELAY_PIN 25

const char* ssid = "SSID";
const char* password = "PASS";

// ---------- OBJETS ----------
Preferences prefs;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------- VARIABLES ----------
float temperature = 0.0;
float batteryVoltage = 0.0;
float batteryPercent = 0.0;

float consigne = 20.0;
int fonction = 1;        // 0=OFF 1=AUTO 2=ON

bool relayState = false;

// ------ TEMPO CHAUFFAGE évite rebonds relais ------

unsigned long lastRelayChange = 0;
const unsigned long MIN_ON_TIME  = 10000; // 10 sec
const unsigned long MIN_OFF_TIME = 10000; // 10 sec


// ---------- ANTI-USURE FLASH ----------

unsigned long lastSaveCons = 0;
unsigned long lastSaveFunc = 0;
const unsigned long SAVE_DELAY = 1000;

float consigneToSave;
  int fonctionToSave;


// ---------- RELAIS mode OFF/ON ----------

void setRelay(bool state) {
  
     if (relayState != state){
         relayState = state;
         digitalWrite(RELAY_PIN, state ? HIGH : LOW);
         lastRelayChange = millis();
         ws.textAll("{\"relay\":" + String(state ? 1 : 0) + "}");
        }
     
}

void updateRelay() {
  
     if (fonction == 2) setRelay(true);
     if (fonction == 0) setRelay(false);
  
}


// ---------- RELAIS mode AUTO ----------
void updateRelayAuto() {
  
     unsigned long now = millis();

     if (!relayState && temperature < consigne - 0.5 && now - lastRelayChange >= MIN_OFF_TIME) {
         setRelay(true);
        } else if (relayState && temperature > consigne + 0.5 && now - lastRelayChange >= MIN_ON_TIME) {
         setRelay(false);
        }
        
}


// ---------- HTML ----------

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, height=device-height, initial-scale=1, user-scalable=no">
<title>ESP32 Chauffage</title>

<style>
html,body{
  margin:0;
  padding:0;
  width:100%;
  height:100%;
  background:#111;
  color:#fff;
  font-family:Arial,Helvetica,sans-serif;
  overflow:hidden;
}

/* ------ BLOQUEUR PORTRAIT pour mobile paysage ------ */
#rotateBlock{
  position:fixed;
  inset:0;
  background:#111;
  color:white;
  display:none;
  z-index:9999;
  align-items:center;
  justify-content:center;
  text-align:center;
  font-size:2rem;
}

/* ---------- INTERFACE ---------- */
.container{
  display:flex;
  flex-direction:column;
  justify-content:space-evenly;
  align-items:center;
  height:100%;
  padding:10px;
}

.big{ font-size:2.5rem; font-weight:bold; }
.medium{ font-size:1.8rem; }

.full{
  width:100%;
}

input[type=range]{
  width:100%;
  height:40px;
}

.btn-group{
  display:flex;
  gap:16px;
  width:100%;
}

.btn-group button{
  flex:1;
  padding:20px 0;
  font-size:1.5rem;
  border-radius:18px;
  border:none;
  background:#ccc;
  color:#000;
}

.active-off{ background:#c0392b!important; color:white; }
.active-auto{ background:#f39c12!important; color:black; }
.active-on{ background:#27ae60!important; color:white; }

#btnRelay{
  width:100%;
  padding:22px 0;
  font-size:1.6rem;
  border-radius:18px;
  border:none;
  color:white;
}
</style>
</head>

<body>

<div id="rotateBlock">
  🔄<br><br>
  Tournez le téléphone<br>
  en mode paysage
</div>

<div class="container" id="app">

  <div class="big" id="line1">--.- °C · -- %</div>

<div style="width:100%">
  <div class="medium" style="text-align:center">
    Consigne : <span id="consVal">--</span> °C
  </div><br>
  <input type="range" min="15" max="25" step="0.5" id="slider" class="full">
</div>

  <div style="width:100%">
    <div class="btn-group">
      <button id="btnOff"  onclick="setFunc(0)">OFF</button>
      <button id="btnAuto" onclick="setFunc(1)">AUTO</button>
      <button id="btnOn"   onclick="setFunc(2)">ON</button>
    </div><br>
    <button id="btnRelay" class="full">RELAIS</button>
  </div>

</div>

<script>
let ws = new WebSocket(`ws://${location.host}/ws`);
let slider = document.getElementById("slider");

function checkOrientation() {  
  if (window.innerHeight > window.innerWidth) {
      rotateBlock.style.display="flex";
      app.style.display="none";
     } else {
      rotateBlock.style.display="none";
      app.style.display="flex";
     }
}

window.addEventListener("resize", checkOrientation);
window.addEventListener("orientationchange", checkOrientation);
checkOrientation();

function updateFunc(v) {
  btnOff.className="";
  btnAuto.className="";
  btnOn.className="";
  if (v==0) btnOff.classList.add("active-off");
  if (v==1) btnAuto.classList.add("active-auto");
  if (v==2) btnOn.classList.add("active-on");
}

ws.onmessage = e => {
  let d = JSON.parse(e.data);

  if (d.temp!==undefined)
      line1.innerHTML = `Température : ${d.temp.toFixed(1)} °C · Batterie : ${d.batt}%`;
  if (d.cons!==undefined) {
      slider.value = d.cons;
      consVal.innerText = d.cons.toFixed(1);
      }

  if (d.func!==undefined) updateFunc(d.func);

  if (d.relay!==undefined) {
      btnRelay.style.background = d.relay ? "#27ae60" : "#c0392b";
      btnRelay.innerText = d.relay ? "CHAUFFAGE ACTIF" : "CHAUFFAGE INACTIF";
     }
};

slider.oninput = () => {
  let v = parseFloat(slider.value);
  consVal.innerText = v.toFixed(1);
  ws.send(JSON.stringify({cons:v}));
};

function setFunc(v){
  ws.send(JSON.stringify({func:v}));
  updateFunc(v);
}
</script>

</body>
</html>
)rawliteral";


// ---------- WEBSOCKET ----------

void onWsEvent (AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *, uint8_t *data, size_t len) {

     if (type == WS_EVT_CONNECT) {
         client->text("{\"temp\":"+String(temperature,1)+
                      ",\"batt\":"+String((int)batteryPercent)+
                      ",\"cons\":"+String(consigne,1)+
                      ",\"func\":"+String(fonction)+
                      ",\"relay\":"+String(relayState?1:0)+"}");
        }

     if (type == WS_EVT_DATA) {
         String msg;
         for (size_t i=0;i<len;i++) msg += (char)data[i];

              if (msg.indexOf("cons")>=0) {
                  float nc = msg.substring(msg.indexOf(":")+1).toFloat();
                  if (round(nc*10) != round(consigne*10)) {
                      consigne = nc;
                      consigneToSave = consigne;
                      lastSaveCons = millis();
                     }
                  updateRelayAuto();      
                 }

              if (msg.indexOf("func")>=0) {
                  int nf = constrain(msg.substring(msg.indexOf(":")+1).toInt(),0,2);
                  if (nf != fonction) {
                      fonction = nf;
                      fonctionToSave = fonction;
                      lastSaveFunc = millis();
                      updateRelay();
                      ws.textAll("{\"func\":"+String(fonction)+"}");
                     }
                  }
        }
        
}


// ---------- SETUP ----------
void setup() {
  
     Serial.begin(115200);
     Serial2.begin(115200, SERIAL_8N1, 16, 17);

     pinMode(RELAY_PIN, OUTPUT);
     digitalWrite(RELAY_PIN, LOW);

     prefs.begin("reglages", false);
     consigne = prefs.getFloat("consigne", 20.0);
     fonction = prefs.getInt("fonction", 1);

     updateRelay();

     WiFi.begin(ssid, password);
     while (WiFi.status()!=WL_CONNECTED) delay(500);
     Serial.println("");
     Serial.println("WiFi connected..!");
     Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
  
     server.on ("/", HTTP_GET, [](AsyncWebServerRequest *r) {
                r->send_P(200, "text/html", index_html);
               });

     ws.onEvent(onWsEvent);
     server.addHandler(&ws);
     server.begin();
     
}


// ---------- LOOP ----------

void loop() {
  
     if (Serial2.available()) {
         String l = Serial2.readStringUntil('\n');
         int t = l.indexOf("T=");
         int b = l.indexOf("B=");
         if (t>=0 && b>=0) {
             temperature = l.substring(t+2, l.indexOf(';')).toFloat();
             batteryVoltage = l.substring(b+2).toFloat();
             batteryPercent = constrain((batteryVoltage-3.4)*100/0.8,0,100);

             if (fonction == 1) updateRelayAuto();

             ws.textAll("{\"temp\":"+String(temperature,1)+
                        ",\"batt\":"+String((int)batteryPercent)+
                        ",\"relay\":"+String(relayState?1:0)+"}");
            }
        }

  if (lastSaveCons && millis()-lastSaveCons > SAVE_DELAY) {
      prefs.putFloat("consigne", consigneToSave);
      lastSaveCons = 0;
     }

  if (lastSaveFunc && millis()-lastSaveFunc > SAVE_DELAY) {
      prefs.putInt("fonction", fonctionToSave);
      lastSaveFunc = 0;
     }
     
}


//
