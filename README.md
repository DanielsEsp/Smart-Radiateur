# Smart-Radiateur
WebSocket et Thermostat deporte pour radiateur 
<img width="800" height="596" alt="WebSocketMobileChauff" src="https://github.com/user-attachments/assets/a31dc19a-9e6e-4d9b-a243-544fc59ccedc" />

L'idee est de moderniser un radiateur 
IMPERATIF sans thermostat electronique pour assurer un redemarrage apres coupure de l'alimentation
Description du projet:
- un ESP32 sert de WebSocket pour l'interface de commande qui affiche la temperature et niveau de batterie du thermostat distant recu par un ESP01 relie en Serial.
Via cette interface on peux regler la consigne de temperature, le mode de fonctionnement en OFF, AUTO ou ON pour marche forcee.
- un ESP01 qui receptionne par EspNow la temperature et niveau de batterie provenant du thermostat distant et les envoie an Serial a l'ESP32 .
- un ESP12 pour thermostat deporte qui envoie temperature et niveau batterie par EspNow a l'ESP01 cote Socket toute les cinq minutes et se met en Time Sleep pour 5 minutes.
j'utilise un module ADS1115 pour la lecture de la tension batterie.

Note: j'ai eu recours a ChatGPT pour la partie interface web du socket.
