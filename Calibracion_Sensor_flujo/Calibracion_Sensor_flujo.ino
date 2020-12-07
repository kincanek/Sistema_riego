/*
  ==================================================================
    FICHERO PARA OBTENER LA CONSTANTE K DEL SENSOR DE FLUJO

  ==================================================================
*/

#include <Wire.h>   // Libreria para comunicacion I2C
#include <RTClib.h> // Libreria para modulos RTC

// El tipo de variable tendra que ser volatile
// para evitar perder los datos generados por
// la interrupcion
volatile long NumP; // Contador de pulsos

int PinS = 2;    // Pin para muestrear los pulsos generados el sensor.

RTC_DS3231 rtc;

//Funcion que ejecuta las interrupciones
// Rutina del Servicio de interrupciones
void ContarPulsos ()
{
  NumP++;  //incrementamos la variable de pulsos
}


// Funcion que imprime la hora:minutos:segundos y el numero de pulsos
void lectura()
{
  DateTime fecha = rtc.now();

  Serial.print(fecha.hour());
  Serial.print(":");
  Serial.print(fecha.minute());
  Serial.print(":");
  Serial.println(fecha.second());
  Serial.print("Numeros de pulsos = ");
  Serial.println(NumP);

}

// Inicializacion de variables
void setup() {

  Serial.begin(9600); // Inicializacion de la comunicacion serial

  pinMode(PinS, INPUT); // Se declara el pin 2 como entrada

  /*
    Modos:
    LOW: Dispara la interrupcion cuando el pinesta a nivel bajo
    CHANGE dispara la interrupcion cuando el pin cambia su valor
    RISING Dispara la interrupcion cuando el pin pasa de nivel bajo a alto
    FALLING Dispara la interrupcion cuando el pin pasa de nivel alto a bajo
  */
  // (Interrupción 0(Pin2),función,Flanco de subida)
  attachInterrupt(digitalPinToInterrupt(0), ContarPulsos, RISING);
  interrupts();   //Habilita las interrupciones del arduino

  // Verifica si el reloj se encuentra conectado
  if (!rtc.begin())
  {
    Serial.println("Modulo RTC no encontrado!");
    Serial.flush();
    abort();
  }

}

void loop()
{
  // Verificamos si el monitor serie se encuetra encendido
  if (Serial.available())
  {
    char data = Serial.read(); // Leemos la entrada en el puerto Serie

    // Si la entrada es 1 leemos la hora y el numero de pulsos de inicio
    if (data == '1')
    {
      Serial.println("Inicio");
      lectura();
    }
    // Si la entrada es 0 leemos la hora y el numero de pulsos final
    if (data == '0')
    {
      lectura();
      NumP = 0; // Limpia la variable

    }
  }
  delay(10);    // Delay de 10 milisegundos
}
