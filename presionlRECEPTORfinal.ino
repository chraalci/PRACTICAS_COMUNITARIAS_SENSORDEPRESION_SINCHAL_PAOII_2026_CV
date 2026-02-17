#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"

// ========== VARIABLES ==========
double presionPSI = 0.0;

char rxpacket[50];
char idDESEADO[] = "T03";

static RadioEvents_t RadioEvents;
bool lora_idle = true;
int16_t rssi;

// ========== DISPLAY ==========
static SSD1306Wire display(
  0x3c,
  500000,
  SDA_OLED,
  SCL_OLED,
  GEOMETRY_128_64,
  RST_OLED
);

// ========== VEXT ==========
void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(915000000);

  // ====== RX LARGO ALCANCE ======
  Radio.SetRxConfig(
    MODEM_LORA,
    0,      // BW 125 kHz
    9,      // SF9
    1,      // CR 4/5
    0,
    8,
    0,
    false,
    0,
    true,
    0,
    0,
    false,
    true
  );

  VextON();
  delay(100);
  display.init();
  display.setFont(ArialMT_Plain_10);

  Serial.println("Receptor presion listo (SF9)");
}

// ================= LOOP =================
void loop() {

  display.clear();
  display.drawString(0, 0, "Presion:");
  display.drawString(0, 20, String(presionPSI, 2) + " PSI");
  display.drawString(0, 40, "RSSI: " + String(rssi));
  display.display();

  if (lora_idle) {
    lora_idle = false;
    Radio.Rx(0);
  }

  Radio.IrqProcess();
}

// ================= RX CALLBACK =================
void OnRxDone(uint8_t *payload, uint16_t size, int16_t packetRssi, int8_t snr)
{
  rssi = packetRssi;

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Radio.Sleep();
  lora_idle = true;

  Serial.print("RX: ");
  Serial.print(rxpacket);
  Serial.print(" RSSI=");
  Serial.println(rssi);

  if (rxpacket[0] != 'T') return;

  char *sep = strchr(rxpacket, '|');
  if (!sep) return;

  char id[5];
  int len = sep - rxpacket;
  strncpy(id, rxpacket, len);
  id[len] = '\0';

  if (strcmp(id, idDESEADO) != 0) return;

  presionPSI = atof(sep + 1);
}
