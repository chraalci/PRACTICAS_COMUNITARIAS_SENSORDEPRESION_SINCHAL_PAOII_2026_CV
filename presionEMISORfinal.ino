#include "LoRaWan_APP.h"
#include "Arduino.h"

// ================= CONFIGURACION ADC =================
#define ADC_PIN 4            // GPIO4 (ADC1)
#define VREF 3.3
#define ADC_MAX 4095.0

// ====== CALIBRACION REAL ======
#define V0_CAL   0.575       // Voltaje ADC a 0 PSI
#define GAIN_CAL 56.12       // PSI por voltio (calibrado)

// ================= LORA =================
int IDsender = 3;            // T03
bool lora_idle = true;
char txpacket[50];

static RadioEvents_t RadioEvents;

void OnTxDone(void);
void OnTxTimeout(void);

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // ---------- ADC ----------
  analogReadResolution(12);
  analogSetPinAttenuation(ADC_PIN, ADC_11db);

  // ---------- MCU ----------
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  // ---------- VEXT (OBLIGATORIO) ----------
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  delay(100);

  // ---------- LORA ----------
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);

  // MISMA CONFIG QUE EL RECEPTOR
  Radio.SetChannel(915000000);

  Radio.SetTxConfig(
    MODEM_LORA,
    20,     // 🔥 20 dBm
    0,
    0,      // BW 125 kHz
    9,      // 🔥 SF9
    1,      // 🔥 CR 4/5
    8,
    false,
    true,
    0,
    0,
    false,
    5000
  );

  Serial.println("Emisor de presion REAL listo (SF9)");
}

// ================= LOOP =================
void loop() {

  // ---------- LECTURA ADC PROMEDIADA ----------
  long suma = 0;
  for (int i = 0; i < 20; i++) {
    suma += analogRead(ADC_PIN);
    delay(2);
  }

  float adcRaw = suma / 20.0;
  float voltaje = adcRaw * (VREF / ADC_MAX);

  // ---------- CALCULO PSI ----------
  float psi = (voltaje - V0_CAL) * GAIN_CAL;
  if (psi < 0) psi = 0;

  // ---------- MONITOR SERIAL ----------
  Serial.print("ADC=");
  Serial.print(adcRaw, 0);
  Serial.print("  V=");
  Serial.print(voltaje, 3);
  Serial.print(" V  PSI=");
  Serial.println(psi, 2);

  // ---------- ENVIO LORA ----------
  if (lora_idle) {
    sprintf(txpacket, "T%02d|%0.2f", IDsender, psi);

    Serial.print("TX -> ");
    Serial.println(txpacket);

    Radio.Send((uint8_t *)txpacket, strlen(txpacket));
    lora_idle = false;
  }

  Radio.IrqProcess();
  delay(1000);
}

// ================= CALLBACKS =================
void OnTxDone(void) {
  Serial.println("TX DONE");
  lora_idle = true;
}

void OnTxTimeout(void) {
  Serial.println("TX TIMEOUT");
  Radio.Sleep();
  lora_idle = true;
}
