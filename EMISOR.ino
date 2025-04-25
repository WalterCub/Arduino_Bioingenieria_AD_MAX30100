/*
  Capitulo 46 de Arduino desde cero en Español.
  Programa que mediante dos pulsadores en un modulo emisor RF de 433 Mhz
  permite enviar el caracter 1 o 2.
  Requiere instalar libreria RadioHead.h

  Autor: bitwiseAr  

*/


/// Programa 2 lado Emisor ////

#include <RH_ASK.h>   // incluye libreria RadioHead.h
#include <SPI.h>    // incluye libreria SPI necesaria por RadioHead.h
 
RH_ASK rf_driver;   // crea objeto para modulacion por ASK

#define PULSADOR1 2   //reemplaza ocurrencia de PULSADOR1 por el numero 2
#define PULSADOR2 3   //reemplaza ocurrencia de PULSADOR2 por el numero 3
#define PULSADOR3 4   //PIN 4
#define PULSADOR4 5   //PIN 5
 
void setup(){
     pinMode(PULSADOR1, INPUT_PULLUP);  // pin 2 como entrada con resistencia de pull-up
     pinMode(PULSADOR2, INPUT_PULLUP);  // pin 3 como entrada con resistencia de pull-up
     pinMode(PULSADOR3, INPUT_PULLUP);  // pin 4 como entrada con resistencia
     pinMode(PULSADOR4, INPUT_PULLUP);  // pin 5 como entrada con resistencia
     rf_driver.init();      // inicializa objeto con valores por defecto
}
 
void loop(){
    if (digitalRead(PULSADOR1) == LOW){   // si se presiona PULSADOR1
    const char *msg = "1";      // carga numero 1 en mensaje a enviar
    rf_driver.send((uint8_t *)msg, strlen(msg));  // envia el mensaje
    rf_driver.waitPacketSent();     // espera al envio correcto del mensaje
    
    }
    
    else if (digitalRead(PULSADOR2) == LOW){  // si se presiona PULSADOR2
    const char *msg = "2";      // carga numero 2 en mensaje a enviar
    rf_driver.send((uint8_t *)msg, strlen(msg));  // envia el mensaje
    rf_driver.waitPacketSent();     // espera al envio correcto del mensaje
    }
    if (digitalRead(PULSADOR3) == LOW){  // si se presiona PULSADOR3
    const char *msg = "3";      // carga numero 3 en mensaje a enviar
    rf_driver.send((uint8_t *)msg, strlen(msg));  // envia el mensaje
    rf_driver.waitPacketSent();     // espera al envio correcto del mensaje
    }
    else if (digitalRead(PULSADOR4) == LOW){  // si se presiona PULSADOR4
    const char *msg = "4";      // carga numero 4 en mensaje a enviar
    rf_driver.send((uint8_t *)msg, strlen(msg));  // envia el mensaje
    rf_driver.waitPacketSent();     // espera al envio correcto del mensaje
    }
    delay(100);         // demora de 200 mseg.
}
