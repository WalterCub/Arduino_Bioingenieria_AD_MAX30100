#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configuración OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET 4
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// MAX30100
PulseOximeter pox;
#define REPORTING_PERIOD_MS 2000
uint32_t tsLastReport = 0;

// AD8232
#define ECG_SENSOR_PIN A0
int ecgThreshold = 400;
unsigned long lastBeatTimeECG = 0;
int bpmECG = 0;

// Callback del MAX30100
void onBeatDetected() {
  Serial.println("Latido detectado por MAX30100");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando MAX30100 + AD8232 + OLED...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Error al iniciar OLED"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1.2);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("Iniciando...");
  display.display();
  delay(2000);

  if (!pox.begin()) {
    Serial.println("MAX30100 no detectado");
    while (true);
  }

  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
  pox.update();

  // Lectura del AD8232
  int ecgSignal = analogRead(ECG_SENSOR_PIN);
  unsigned long currentTime = millis();
  if (ecgSignal > ecgThreshold) {
    if (currentTime - lastBeatTimeECG > 400) {
      unsigned long timeBetweenBeats = currentTime - lastBeatTimeECG;
      lastBeatTimeECG = currentTime;
      bpmECG = 60000 / timeBetweenBeats;
    }
  }

  // Mostrar cada cierto tiempo
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    tsLastReport = millis();

    float bpmMAX = pox.getHeartRate();
    float spo2 = pox.getSpO2();

    // Monitor Serial
    Serial.print("MAX30100 BPM: "); Serial.print(bpmMAX, 1);
    Serial.print(" | SpO2: "); Serial.print(spo2, 1);
    Serial.print(" % | AD8232 BPM: "); Serial.println(bpmECG);

    // OLED Display
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("MAX BPM: ");
    display.print(bpmMAX, 1);
    display.setCursor(0, 10);
    display.print("SpO2: ");
    display.print(spo2, 1); display.println(" %");
    display.setCursor(0, 20);
    display.print("ECG BPM: ");
    display.print(bpmECG);
    display.display();
  }
}
