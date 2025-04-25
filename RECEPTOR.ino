/*
  Programa que enciende LED rojo y apaga el mismo al recibir el número 1,
  y enciende LED verde por 5 segundos y lo apaga al recibir el número 2.
*/

#include <RH_ASK.h>
#include <SPI.h>

RH_ASK rf_driver;

#define LED_UNO 2
#define LED_DOS 3
#define LED_TRES 4
#define LED_CUATRO 5

void setup() {
  pinMode(LED_UNO, OUTPUT);
  pinMode(LED_DOS, OUTPUT);
  pinMode(LED_TRES, OUTPUT);
  pinMode(LED_CUATRO, OUTPUT);
  rf_driver.init();
}

void loop() {
  uint8_t buf[1];
  uint8_t buflen = sizeof(buf);

  if (rf_driver.recv(buf, &buflen)) {
    char receivedChar = (char)buf[0];

    if (receivedChar == '1') {
      digitalWrite(LED_UNO, !digitalRead(LED_UNO));
    }
    else if (receivedChar == '2') {
      digitalWrite(LED_DOS, !digitalRead(LED_DOS));
    }
    else if (receivedChar == '3') {
      digitalWrite(LED_TRES, !digitalRead(LED_TRES));
    }
     else if (receivedChar == '4') {
      digitalWrite(LED_CUATRO, !digitalRead(LED_CUATRO));
    }
  }
}
