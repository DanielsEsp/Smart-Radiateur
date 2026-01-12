



#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}
uint8_t receiverMac[] = { MAC ADDRESS de l'ESP01 cote socket};

// -------- STRUCTURE ESPNOW --------
typedef struct {
                float temp;
                float batt;
               } espnowData;

                 espnowData txData;
                 
// ------ CONFIGURATION DUREE SOMMEIL -------
#define SLEEP_TIME_US 300e6   // 5 minutes
//
#include <Wire.h>
// ------ CONFIGURATION CAPTEUR TEMPERATURE ------
#include <Adafruit_Sensor.h>
#include "Adafruit_Si7021.h"
          Adafruit_Si7021 sensor = Adafruit_Si7021();
// ------ CONFIGURATION CIRCUIT MESURE TENSION ------
#include "Adafruit_ADS1X15.h"
          Adafruit_ADS1115 ads;
          

//   -------- SETUP --------
void setup() {
  
     Serial.begin(115200); Serial.println(); Serial.println();
     
     Wire.begin(); //  I2C = 0x40 capteur température

     ads.begin();  //  I2C = ADS1115 mesure tension
     ads.setGain(GAIN_ONE); // ±4.096V → 0.125mV/bit

     // --- WIFI / ESPNOW ---
     WiFi.mode(WIFI_STA);
     WiFi.disconnect();
     if (esp_now_init() != 0) {
         ESP.restart(); //     ESP RESTART si échec EspNow
        }

     esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
     esp_now_add_peer(receiverMac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

//   ------ MESURES TEMPERATURE ET TENSION BATTERIE ------
     txData.temp = sensor.readTemperature();
     Serial.print("\tTemperature: "); Serial.print(txData.temp);

     float multivoltage = 0; //    Pour moyenne de 5 lectures tension batterie    
     Serial.print("\tVoltage  : "); 
     for (int boucle = 0; boucle < 5; boucle ++) {
          int16_t raw = ads.readADC_SingleEnded(2);
          float voltage = raw * 0.125 / 1000.0;   // ADS1115 → volts
          Serial.print(voltage); Serial.print("-"); delay(100);
          multivoltage = multivoltage + voltage;
         }
     multivoltage = multivoltage / 5; //  Tension de la batterie
     Serial.print("\tMulti Voltage  : "); Serial.print(multivoltage);
     multivoltage *= 1.028350515; //      Compense écart lecture et réel

     txData.batt = multivoltage;
     Serial.print("\tBatterie  : "); Serial.print(txData.batt);
     Serial.println(); Serial.println();
     
//   ------ ENVOI ESPNOW ------
     esp_now_send(receiverMac, (uint8_t*)&txData, sizeof(txData));
     delay(50);  // temps radio

//   ----- MISE EN SOMMEIL -----
     ESP.deepSleep(SLEEP_TIME_US);
}


// -------- LOOP --------

void loop() {
  
  // boucle vide
  
}


//
