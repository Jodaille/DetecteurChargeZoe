# DetecteurChargeZoe
Détecteur de charge de Renault Zoé. Envoie le statut via MQTT à HomeAssistant (à la sauce Tasmota)

## Modèle 3D

Fichier Freecad: 3D/VoyantsZoe.FCStd


Le fichier VoyantsZoe-PlaqueESP.stl permet d'imprimer une plaque de test afin de vérifier que la largeur de la plaque correspond bien à celle du chargeur, que les trous accueillant les LDR (TEPT4400) soient bien en face des voyants du chargeur.
Dans le spreadsheet du fichier Freecad, les valeurs de hauteur_plaque et epaisseur_plaque sont réduites (32,20mm hauteur_plaque).
Cf: 3D/Freecad-spreadsheet-print-test-values.png

## Code/Hardware

Un esp32-C3-mini (Waveshare ? ~ 3-4€)
3 LDR TEPT4400 (~3€ les 10)

Les 3 LDR ont une masse commune (collecteur)
Chaque émetteur est relié à son GPIO (Pullup dans le code)
#define PIN_READY   6
#define PIN_CHARGE  5
#define PIN_FAULT   4

Nb:
Patte plus longue = collecteur (C).
Patte plus courte = émetteur (E).

Le code arduino surveille les 3 voyants du chargeur.
Il modifie la couleur de la led RGB de l'esp32-mini-C3 en fonction.
Publie les états/le changement sur les topics MQTT.

exemple publication:
tele/zoe/STATE {"Ready":"ON","Charge":"OFF","Fault":"OFF"}

dans sketchbook/

script principal: sketchbook/DetecteurChargeESP32-C3/DetecteurChargeESP32-C3.ino

config wifi + MQTT: sketchbook/DetecteurChargeESP32-C3/config.h