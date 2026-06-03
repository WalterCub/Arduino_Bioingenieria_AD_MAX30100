#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <DHT.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <time.h>

// ======================================================
// CREDENCIALES ARDUINO CLOUD
// ======================================================
const char DEVICE_LOGIN_NAME[] = "13d9f858-9956-471b-8dc0-f2a10bd374de";
const char DEVICE_KEY[]        = "GnDn9YKYNQt7f885qt796#SnK";

// ======================================================
// WIFI
// ======================================================
const char SSID[] = "BLUE_KITTYSDINOCAVE";
const char PASS[] = "05Kitty14*";

// ======================================================
// TELEGRAM
// ======================================================
const char BOT_TOKEN[] = "8879397221:AAHq9jwUC7psJg1xV4471RHC05awDTO-MVI";
const char CHAT_ID[]   = "7946584551";


WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// ======================================================
// HORA NTP
// Colombia / Ecuador / Perú: UTC-5
// Si estás en otro país, ajusta GMT_OFFSET_SEC.
// ======================================================
const char* TZ_INFO = "COT5";
bool horaConfigurada = false;

// ======================================================
// SENSORES
// ======================================================
#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ135_PIN 34

DHT dht(DHTPIN, DHTTYPE);

// ======================================================
// VARIABLES PARA ARDUINO CLOUD
// ======================================================
float temperatura;
float humedad;
int mq135Raw;
int calidadAire;
String estadoAire;

// Hora exacta de la última medición usada
String horaUltimaMedicion = "Sin sincronizar";

// ======================================================
// CONEXION ARDUINO CLOUD
// ======================================================
WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

// ======================================================
// CALIBRACION MQ135
// ======================================================
const int MQ_LIMPIO = 1350;
const int MQ_MALO   = 2100;

// ======================================================
// CONTROL DE TIEMPO
// ======================================================
unsigned long tiempoMQ135 = 0;
unsigned long tiempoDHT = 0;
unsigned long tiempoSerial = 0;

const unsigned long intervaloMQ135 = 200;
const unsigned long intervaloDHT = 2000;
const unsigned long intervaloSerial = 500;

// Mutex para que Telegram lea datos sin interrumpir el muestreo
SemaphoreHandle_t datosMutex;

// ======================================================
// DECLARACION DE FUNCIONES
// ======================================================
void leerDHT11();
void leerMQ135();
int leerMQ135Promedio();
void mostrarDatosSerial();
String generarMensajeEstado();
void revisarTelegram();
bool enviarTelegramSeguro(String mensaje);
String obtenerHoraActual();
void tareaTelegram(void * parameter);

// ======================================================
// CONFIGURACION DE PROPIEDADES CLOUD
// ======================================================
void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);

  ArduinoCloud.addProperty(temperatura, READ, 1 * SECONDS, NULL);
  ArduinoCloud.addProperty(humedad, READ, 1 * SECONDS, NULL);
  ArduinoCloud.addProperty(mq135Raw, READ, 1 * SECONDS, NULL);
  ArduinoCloud.addProperty(calidadAire, READ, 1 * SECONDS, NULL);
  ArduinoCloud.addProperty(estadoAire, READ, ON_CHANGE, NULL);
}

void setup() {

  Serial.begin(115200);
  delay(1500);

  dht.begin();

  analogReadResolution(12);
  analogSetPinAttenuation(MQ135_PIN, ADC_11db);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  datosMutex = xSemaphoreCreateMutex();

  secured_client.setInsecure();
  secured_client.setTimeout(1000);
  bot.longPoll = 0;

  initProperties();

  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(0);
  // ArduinoCloud.printDebugInfo();

  // Configurar hora por internet

  Serial.println("Sistema iniciado.");
  Serial.println("Arduino Cloud activo.");
  Serial.println("Telegram se ejecuta en tarea separada.");

  // Tarea separada para Telegram.
  // Si Telegram tarda, no detiene el loop principal.
  xTaskCreatePinnedToCore(
    tareaTelegram,
    "TareaTelegram",
    12288,
    NULL,
    1,
    NULL,
    0
  );
}

void loop() {
  ArduinoCloud.update();

  if (!horaConfigurada && WiFi.status() == WL_CONNECTED) {

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 3000)) {
    horaConfigurada = true;
    Serial.println("Hora local de Colombia sincronizada.");
  } else {
    Serial.println("Esperando sincronizacion de hora...");
  }
}

  unsigned long tiempoActual = millis();

  // MQ135 rápido
  if (tiempoActual - tiempoMQ135 >= intervaloMQ135) {
    tiempoMQ135 = tiempoActual;
    leerMQ135();
  }

  // DHT11 cada 2 segundos
  if (tiempoActual - tiempoDHT >= intervaloDHT) {
    tiempoDHT = tiempoActual;
    leerDHT11();
  }

  // Monitor serial
  if (tiempoActual - tiempoSerial >= intervaloSerial) {
    tiempoSerial = tiempoActual;
    mostrarDatosSerial();
  }
  
}

void leerDHT11() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) {
    if (xSemaphoreTake(datosMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
      humedad = h;
      temperatura = t;
      xSemaphoreGive(datosMutex);
    }
  } else {
    Serial.println("Error al leer el DHT11");
  }
}

void leerMQ135() {
  int raw = leerMQ135Promedio();

  int calidad = map(raw, MQ_LIMPIO, MQ_MALO, 100, 0);
  calidad = constrain(calidad, 0, 100);

  String estado;

  if (calidad >= 70) {
    estado = "Buena";
  } else if (calidad >= 40) {
    estado = "Regular";
  } else {
    estado = "Mala";
  }

  String hora = obtenerHoraActual();

  if (xSemaphoreTake(datosMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
    mq135Raw = raw;
    calidadAire = calidad;
    estadoAire = estado;
    horaUltimaMedicion = hora;
    xSemaphoreGive(datosMutex);
  }
}

int leerMQ135Promedio() {
  long suma = 0;
  const int muestras = 4;

  for (int i = 0; i < muestras; i++) {
    suma += analogRead(MQ135_PIN);
    delay(1);
  }

  return suma / muestras;
}

void mostrarDatosSerial() {
  float tempLocal;
  float humLocal;
  int mqLocal;
  int calidadLocal;
  String estadoLocal;
  String horaLocal;

  if (xSemaphoreTake(datosMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
    tempLocal = temperatura;
    humLocal = humedad;
    mqLocal = mq135Raw;
    calidadLocal = calidadAire;
    estadoLocal = estadoAire;
    horaLocal = horaUltimaMedicion;
    xSemaphoreGive(datosMutex);
  } else {
    return;
  }

  Serial.print("Hora: ");
  Serial.print(horaLocal);
  Serial.print(" | Temp: ");
  Serial.print(tempLocal);
  Serial.print(" C | Hum: ");
  Serial.print(humLocal);
  Serial.print(" % | MQ135: ");
  Serial.print(mqLocal);
  Serial.print(" | Calidad: ");
  Serial.print(calidadLocal);
  Serial.print(" % | Estado: ");
  Serial.println(estadoLocal);
}

String obtenerHoraActual() {
  time_t now = time(nullptr);

  // Si la hora aún no se ha sincronizado
  if (now < 100000) {
    return "Hora no sincronizada";
  }

  // Colombia = UTC - 5 horas
  now -= 5 * 3600;

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

  return String(buffer);
}


String generarMensajeEstado() {
  float tempLocal;
  float humLocal;
  int mqLocal;
  int calidadLocal;
  String estadoLocal;
  String horaLocal;

  if (xSemaphoreTake(datosMutex, 100 / portTICK_PERIOD_MS) == pdTRUE) {
    tempLocal = temperatura;
    humLocal = humedad;
    mqLocal = mq135Raw;
    calidadLocal = calidadAire;
    estadoLocal = estadoAire;
    horaLocal = horaUltimaMedicion;
    xSemaphoreGive(datosMutex);
  } else {
    return "No se pudo leer la medicion actual.";
  }

  String mensaje = "Estado ambiental actual\n\n";

  mensaje += "Hora de la medicion:\n";
  mensaje += horaLocal;
  mensaje += "\n\n";

  mensaje += "Temperatura: ";
  mensaje += String(tempLocal, 1);
  mensaje += " C\n";

  mensaje += "Humedad: ";
  mensaje += String(humLocal, 1);
  mensaje += " %\n";

  mensaje += "MQ135 RAW: ";
  mensaje += String(mqLocal);
  mensaje += "\n";

  mensaje += "Calidad de aire: ";
  mensaje += String(calidadLocal);
  mensaje += " %\n";

  mensaje += "Estado del aire: ";
  mensaje += estadoLocal;

  return mensaje;
}

bool enviarTelegramSeguro(String mensaje) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Telegram: WiFi no conectado.");
    return false;
  }

  bool enviado = bot.sendMessage(CHAT_ID, mensaje, "");

  if (enviado) {
    Serial.println("Telegram: mensaje enviado.");
    return true;
  } else {
    Serial.println("Telegram: fallo al enviar mensaje.");
    return false;
  }
}

void revisarTelegram() {
  int mensajesNuevos = bot.getUpdates(bot.last_message_received + 1);

  if (mensajesNuevos <= 0) {
    return;
  }

  for (int i = 0; i < mensajesNuevos; i++) {
    String chat_id = bot.messages[i].chat_id;
    String texto = bot.messages[i].text;

    texto.trim();

    Serial.print("Telegram recibido: ");
    Serial.println(texto);

    if (chat_id != CHAT_ID) {
      Serial.println("Telegram: usuario no autorizado.");
      continue;
    }

    if (texto == "/start") {
      String bienvenida = "Sistema de monitoreo ambiental activo.\n\n";
      bienvenida += "Comandos disponibles:\n";
      bienvenida += "/estado - Consultar temperatura, humedad y calidad del aire\n";
      bienvenida += "/ayuda - Ver comandos disponibles";

      enviarTelegramSeguro(bienvenida);
    }

    else if (texto == "/estado") {
      enviarTelegramSeguro(generarMensajeEstado());
    }

    else if (texto == "/ayuda") {
      String ayuda = "Comandos disponibles:\n\n";
      ayuda += "/estado - Consultar datos actuales\n";
      ayuda += "/start - Iniciar el bot\n";
      ayuda += "/ayuda - Ver ayuda";

      enviarTelegramSeguro(ayuda);
    }

    else {
      enviarTelegramSeguro("Comando no reconocido. Usa /ayuda");
    }
  }
}

void tareaTelegram(void * parameter) {
  delay(8000);

  Serial.println("Tarea Telegram iniciada.");

  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      revisarTelegram();
    }

    // Telegram se revisa cada 3 segundos,
    // pero si tarda, solo se bloquea esta tarea, no el muestreo.
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}
