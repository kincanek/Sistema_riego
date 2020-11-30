/*
 * ============================================================
 *              PUESTA A PUNTO DEL MODULO RTC3231
 * ============================================================ 
 */
#include <Wire.h>   // Permite al arduino comunicarse por I2C
#include "RTClib.h"  // Permite obtener datos a partir del RTC

RTC_DS3231 rtc;

void setup () {
  Serial.begin(9600);
  rtc.begin();
  // Cargamos la fecha correspondiente al sistema 
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //Carga el tiempo en el cual se compila
  Serial.print("Fecha = ");
  Serial.print(__DATE__);
  Serial.print("  Hora = ");
  Serial.println(__TIME__);
}

void loop () {
}
