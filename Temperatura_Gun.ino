#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Dimensiones de pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// Crear instancia para la pantalla OLED (dirección 0x3C por defecto)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// Crear instancia para el sensor MLX90614
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
// Pines
const int ledRojo = 5;
const int ledVerde = 6;
const int buzzer = 4;
const int boton = 2;
const int laser = 3;
bool medicionActiva = false;
void setup() {
  Serial.begin(9600);
  // Inicializar pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Error al iniciar la pantalla OLED"));
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Cargando...");
  display.display();
  // Inicializar sensor MLX90614
  mlx.begin();
  // Configurar pines
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(laser, OUTPUT);
  pinMode(boton, INPUT_PULLUP); // El botón va a GND
  digitalWrite(laser, LOW); // Inicialmente apagado
  digitalWrite(ledRojo, LOW);
  digitalWrite(ledVerde, LOW);
  digitalWrite(buzzer, LOW);
  delay(1500);
  display.clearDisplay();
}
void loop() {
  if (digitalRead(boton) == LOW) {
    medicionActiva = true;
    digitalWrite(laser, HIGH);
    delay(300); // Pequeño rebote y espera
  }
  if (medicionActiva) {
    float temp = mlx.readObjectTempC();
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("TEMP:");
    display.setCursor(0, 20);
    display.setTextSize(3);
    display.print(temp, 1); // Un decimal
    display.print(" C");
    display.display();
    if (temp > 37.5) {
      digitalWrite(ledRojo, HIGH);
      digitalWrite(ledVerde, LOW);
      tone(buzzer, 1000, 300); // Beep
    } else {
      digitalWrite(ledRojo, LOW);
      digitalWrite(ledVerde, HIGH);
      noTone(buzzer);
    }
    delay(2000); // Espera entre lecturas
    digitalWrite(ledRojo, LOW);
    digitalWrite(ledVerde, LOW);
    digitalWrite(buzzer, LOW);
    digitalWrite(laser, LOW);
    medicionActiva = false; 
  }
}
