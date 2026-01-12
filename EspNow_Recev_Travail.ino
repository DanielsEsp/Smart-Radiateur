



#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}

// ---- STRUCTURE DATAS ESPNOW ----
typedef struct {
                float temp;
                float batt;
               } espnowData;

                 espnowData rxData;


// -------- CALLBACK ESPNOW --------
void onDataRecv (uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  
     if (len == sizeof(espnowData)) {
         memcpy(&rxData, incomingData, sizeof(rxData));

//       Envoi vers ESP32 (Socket)
         Serial.print("T=");
         Serial.print(rxData.temp, 1);
         Serial.print(";B=");
         Serial.println(rxData.batt, 2);
        }
}


//   -------- SETUP --------

void setup() {
  
     Serial.begin(115200);

     WiFi.mode(WIFI_STA);
     WiFi.disconnect();   // nécessaire pour ESPNOW sur ESP8266

     if (esp_now_init() != 0) {
     //  échec init ESPNOW
         return;
        }

     esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
     esp_now_register_recv_cb(onDataRecv);
}


//   -------- LOOP --------

void loop() {
  
//   Boucle vide
  
}


//
