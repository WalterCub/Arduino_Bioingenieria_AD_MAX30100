#include <Wire.h> // Librería para comunicación I2C (usada por OLED y MAX30100)
#include "MAX30100_PulseOximeter.h" // Librería para manejar el sensor MAX30100
#include <Adafruit_GFX.h> // Librería gráfica base para pantallas
#include <Adafruit_SSD1306.h> // Librería específica para pantallas OLED SSD1306

// Configuración de la pantalla OLED
#define SCREEN_WIDTH 128 // Ancho de la pantalla OLED en píxeles
#define SCREEN_HEIGHT 32 // Alto de la pantalla OLED en píxeles
#define OLED_RESET 4 // Pin de reset de la pantalla OLED (no siempre se usa)
#define SCREEN_ADDRESS 0x3C // Dirección I2C común para pantallas OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Objeto display para controlar la pantalla

// Sensor MAX30100
PulseOximeter pox; // Objeto para manejar el sensor MAX30100
#define REPORTING_PERIOD_MS 2000 // Intervalo de tiempo para mostrar datos (en milisegundos)
uint32_t tsLastReport = 0; // Marca de tiempo para controlar la frecuencia de actualización

// Sensor AD8232 (ECG)
#define ECG_SENSOR_PIN A0 // Pin analógico donde se conecta la salida del AD8232
int ecgThreshold = 400; // Umbral para detectar un latido en la señal analógica
unsigned long lastBeatTimeECG = 0; // Marca de tiempo del último latido detectado (AD8232)
int bpmECG = 0; // Valor calculado de BPM desde el AD8232

// Función que se ejecuta cada vez que el MAX30100 detecta un latido
void onBeatDetected() {
  Serial.println("Latido detectado por MAX30100");
}

void setup() {
  Serial.begin(115200); // Inicia comunicación serial para depuración
  Serial.println("Iniciando MAX30100 + AD8232 + OLED...");

  // Inicializar la pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Error al iniciar OLED"));
    while (true); // Si falla, el programa se detiene
  }

  // Mensaje inicial en pantalla
  display.clearDisplay();
  display.setTextSize(1.2);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("Iniciando...");
  display.display();
  delay(2000);

  // Inicializar el sensor MAX30100
  if (!pox.begin()) {
    Serial.println("MAX30100 no detectado");
    while (true); // Si falla, se detiene el programa
  }

  // Ajustar intensidad del LED IR del MAX30100
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  // Asignar la función que se ejecuta al detectar un latido
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
  pox.update(); // Llamada obligatoria para actualizar internamente el MAX30100

  // Leer señal analógica del AD8232
  int ecgSignal = analogRead(ECG_SENSOR_PIN);
  unsigned long currentTime = millis(); // Tiempo actual

  // Si la señal supera el umbral y ha pasado tiempo suficiente desde el último latido
  if (ecgSignal > ecgThreshold) {
    if (currentTime - lastBeatTimeECG > 400) { // 400 ms para evitar contar el mismo latido varias veces
      unsigned long timeBetweenBeats = currentTime - lastBeatTimeECG;
      lastBeatTimeECG = currentTime;
      bpmECG = 60000 / timeBetweenBeats; // Cálculo de BPM
    }
  }

  // Mostrar información cada cierto tiempo
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    tsLastReport = millis(); // Actualizar la marca de tiempo

    float bpmMAX = pox.getHeartRate(); // Obtener BPM desde el MAX30100
    float spo2 = pox.getSpO2(); // Obtener SpO2 desde el MAX30100

    // Mostrar datos en monitor serial
    Serial.print("MAX30100 BPM: "); Serial.print(bpmMAX, 1);
    Serial.print(" | SpO2: "); Serial.print(spo2, 1);
    Serial.print(" % | AD8232 BPM: "); Serial.println(bpmECG);

    // Mostrar datos en la pantalla OLED
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
