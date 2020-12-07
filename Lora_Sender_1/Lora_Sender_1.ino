// TTGO LORA OLED ESP32 V.2.1.6
/*
Libreria que permite la comunicacion con dispositivos 
por bus I2C (inter integrated circuit)
SDA: Datos y SCL: Reloj

SDA: Pin GPIO 21
SCL: Pin GPIO 22
*/

#include <Wire.h>

/*
Librerias para el control del RTC
*/
#include <ErriezDS3231.h>
#include "RTClib.h"
#include <LoRa.h>
#include <SPI.h>
#include "FS.h"
#include "SPIFFS.h"

/*
 * With ICACHE_RAM_ATTR you put the function on the RAM.

With ICACHE_FLASH_ATTR you put the function on the FLASH (to save RAM).

Interrupt functions should use the ICACHE_RAM_ATTR. Function that are 
called often, should not use any cache attribute.

Important: NEVER access your flash inside an interrupt! The interrupt 
can occur during a flash access, so if you try to access the flash at 
the same time, you will get a crash (and sometimes this happens after 
1-2 hours after you use your device).
 */

ICACHE_RAM_ATTR
// Clases enlazadas al RTC
ErriezDS3231 Clock; 
RTC_DS3231 rtcc;

// Pin entrada de interrupcion del reloj
#define INT_PIN 13
// Pin GPIO 04 Salida de ecendido 
#define RELE 4 
// Pin GPIO 25 Entrada de interrupcion Sensor de agua
#define Sensor 25 

#define SCK 5 // GPIO5 -- SCK
#define MISO 19 // GPIO19 -- MISO
#define MOSI 27 // GPIO27 -- MOSI
#define SS 18 // GPIO18 -- CS
#define RST 23 // GPIO23 -- RESET
#define DI0 26 // GPIO26 -- IRQ(Interrupt Request)
#define BAND 915E6

//#define BUTTON_PIN_BITMASK 0X8000

// Variables de interrupciones
volatile bool alarmInterrupt = false; // Interrrupcion de la alarma
volatile int count = 0; // Interrupcion del sensor

// Define la entrada y salida del modo sleep
bool sleep_start = true;

////////////////////////////////////////////////////////////
/*
Variables a modificar, en caso de requerirlo
*/
const int T_Muestreo = 3000; // Tiempo de muestreo sensor ms
const int tiempo_riego = 1;  // Tiempo de riego en minutos



//////////// Definicion de alarmas periodicas ///////////

/*
 Dia de la semana 0: domingo 1: Lunes 2: Martes 
 3: Miercoles 4:Jueves 5:Viernes 6:Sabado 
 Horas (0 - 24)
 Minutos (0 - 59)
*/


bool alarm_periodica = true; // Activacion de alarma
#define rows 3 // Numero de alarmas
const int alarms[][3]= {{5,22,56},
                      {5,23,5},
                      {5,23,10}};                           


//////////////////////////////////////////////////////////////

// Constante de conversion del sensor
const float K = 6.997; 
float volumen = 0; // Volumen en litros
long t0 = 0; // Tiempo transcurrido de riego


// Contador de pulsos
void contador()
{
    /*
Esta funcion permite el aumento rapido del contador 
cuando se hacen interrupciones del sensor 
   */
    count++;
}

void onAlarm()
{
  /*
Esta funcion permite hacer un llamado rapido
para la interrupccion del reloj
   */
    alarmInterrupt = true;
}

void setAlarm2(bool std)
{
    /*
Esta funcion permite fijar y actualizar las alarmas
periodicas
     */
DateTime hoy = rtcc.now(); // Lectura del RTC

int index_alarm = 0;

/////////////////// Buscamos que alarma activar ////////////////
for(int i=0;i < rows;i++){
  //Busca la alarma siguiente
  if(alarms[i][0]>=hoy.dayOfTheWeek()){
    // Si la alarma es el dia actual, busca 
    // que la hora y minutos sean mayores al actual
    if(alarms[i][0]==hoy.dayOfTheWeek()){
      // Compara la hora y los minutos
      if(alarms[i][1] == hoy.hour() && alarms[i][2] > hoy.minute() ){
        index_alarm = i;
        break;
      }
      if(alarms[i][1] > hoy.hour()){
        index_alarm = i;
        break;
      }
    }
    // En caso de que el dia de la semana sea diferente 
    // a la actual
    if(alarms[i][0] != hoy.dayOfTheWeek()){
      index_alarm = i;
      break;
      }
  }

}
// Si es verdadero se activa la alarma para encender el ESP32
// Un minuto antes de que se encienda el motor
    if(std)
    {
      Serial.println("Encendido de ESP programado");
      if(alarms[index_alarm][2] == 0)
      {
        Serial.println(index_alarm);
       Clock.setAlarm2(Alarm2MatchDay, //Modo de alarma
                          alarms[index_alarm][0], // Dia de la semana
                          alarms[index_alarm][1]-1, // Hora
                          alarms[index_alarm][2]+59);     
      }
      if(alarms[index_alarm][2] != 0){
        Serial.println(index_alarm);
       Clock.setAlarm2(Alarm2MatchDay, //Modo de alarma
                          alarms[index_alarm][0], // Dia de la semana
                          alarms[index_alarm][1], // Hora
                          alarms[index_alarm][2]-1);     
      }
    }
    // Activamos la alarma para encender el motor
    else{
    Serial.println("Alarma para riego programada");
    Serial.println(index_alarm);
    // Fijado de alarma
    Clock.setAlarm2(Alarm2MatchDay, //Modo de alarma
                          alarms[index_alarm][0], // Dia de la semana
                          alarms[index_alarm][1], // Hora
                          alarms[index_alarm][2]); // minutos
    // Enciende la alarma
    }
    Clock.alarmInterruptEnable(Alarm2, true);
}


void writeFile(fs::FS &fs, const char *path, char *mensaje)
{
  Serial.println("Mensaje escrito en archivo");

  File file = fs.open(path,FILE_WRITE);
  if(!file)
  {
    Serial.println("Fallo en abrir el archivo para escribir");
    
  }
  if(file.println(mensaje))
  {
    Serial.println("Mensaje guardado");
  }
  else
  {
    Serial.println("Error de escritura");
  }
  file.close();
}

void readFile(fs::FS &fs, const char * path)
{
  char data[127];
  Serial.println("Leyendo mensaje");

  File file = fs.open(path,"r");

  if(!file || file.isDirectory()){
        Serial.println("- invalid file");
        return;
    }

        while(file.available()) { 
        //data = file.readStringUntil('\0');
        sprintf(data,"%s",file.readStringUntil('\n'));
        Serial.println(data);        
    }
      file.close();  //Close file
      Serial.println("File Closed");
    
    
}

void Lora_sender(float mensaje,DateTime start,DateTime Stop)
{
  Serial.print("Mensaje enviado");
  LoRa.beginPacket();
  LoRa.print(start.year());
  LoRa.print("/");
  LoRa.print(start.month());
  LoRa.print("/");
  LoRa.print(start.day());
  LoRa.print(",");
  LoRa.print(start.hour());
  LoRa.print(":");
  LoRa.println(start.minute());
  LoRa.print(",");
  LoRa.print(mensaje);
  LoRa.endPacket();
}
// Calculo del volumen en l/m
void In_volumen(float dV)
{
  // Recibe como entrada el caudal
    volumen += dV / 60 * (millis() - t0) / 1000.0;
    /* millis() devuelve el numero de milisegundos desde 
     *  que la placa se empezo a ejecutar luego del encendido
     *  se desborda despues de 50 dias y vuelve a comenzar.
     *  millis() no interfiere en la ejecucion del programa
     *  como delay()
     */
    t0 = millis();
}

// Obtenemos la frecuencia
float Frecuencia()
{
/*
 * Esta funcion permite contar el numero de pulsos
 * en un tiempo determinado. Se hace de esta manera
 * para evitar mediciones falsas al principio y el
 * incremento de la variables count es menor.
 */
    count = 0;
      delay(T_Muestreo); // Detenemos la ejecucion del programa
    return (float)count * 1000 / T_Muestreo;
}

// Medimos el caudal y el volumen 
void Medir( )
{
    float caudal = Frecuencia() / K ; // Calculo el caudal 
    In_volumen(caudal); // Manda a llamar la funcion que suma el volumen 

}
void Stop_alarm(){
              ///////////////////////////////////////////////////////
             // Desactivamos las alarmas para evitar interferencias
            if(Clock.getAlarmFlag(Alarm2))
            {
              Clock.clearAlarmFlag(Alarm2);
            }
            
            if(Clock.getAlarmFlag(Alarm1))
            {
              Clock.clearAlarmFlag(Alarm1);
            }
            ////////////////////////////////////////////////////////////
}
void Activate_alarm(bool stado = false){
      /////////////////// Activacion de alarmas /////////////////////////
     
    if(alarm_periodica){
      setAlarm2(stado);
    }
    else{
      Clock.alarmInterruptEnable(Alarm2, false);
    }   
    Clock.alarmInterruptEnable(Alarm1, false);
    ///////////////////////////////////////////////////////////////////
}
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :{
      Serial.println("Wakeup caused by external signal using RTC_IO"); 
      if(clock.getAlarmFlag(Alarm2)){
      Activate_alarm(false);
      sleep_start = false;
      break;
      }

    }
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup()
{
    // Inicializacion 
    Serial.begin(9600);
    SPI.begin(SCK,MISO,MOSI,SS);
    LoRa.setPins(SS,RST,DI0);
    
    if(!LoRa.begin(915E6))
    {
      Serial.println("Starting Lora failed");
      while(1);
    }
    LoRa.setSpreadingFactor(12);
    LoRa.setTxPower(17, PA_OUTPUT_PA_BOOST_PIN);

    if(!SPIFFS.begin(true))
    {
      Serial.println("Fallo modulo SPIFFS");
      return;
    }
    
    Wire.begin();
    Wire.setClock(400000);
    Clock.setSquareWave(SquareWaveDisable);
    
    // Entrada del sensor
    pinMode(Sensor,INPUT);
    attachInterrupt(digitalPinToInterrupt(Sensor), contador, RISING);
    interrupts();
    
    // Entrada del RTC
    pinMode(INT_PIN, INPUT_PULLUP); 
    attachInterrupt(digitalPinToInterrupt(INT_PIN), onAlarm, FALLING);
    
     pinMode(RELE,OUTPUT); // Activacion del relevador
     digitalWrite(RELE, HIGH); // Apagamos el relevador

     esp_sleep_enable_ext0_wakeup(GPIO_NUM_15,0);
     print_wakeup_reason();
     
     
}

void loop()
{
 
    
    if (alarmInterrupt) {
            Stop_alarm();
            alarmInterrupt = false; // Reinicializamos la variable de interrupcion 
            
            Serial.print("Alarma activada");
            // Calculo de la hora de apagado de la bomba
            DateTime hoyy = rtcc.now(); // Fecha actual
            DateTime pause (hoyy.unixtime()+ tiempo_riego * 60); // Fecha futura
            // Encendido del motor
            digitalWrite(RELE,LOW);
            int count = 0;
            while(true){
              DateTime hoy = rtcc.now();
              if(hoy.hour() == pause.hour() && hoy.minute()==pause.minute()){
                break;
              }
              Medir();// Medicion del sensor

              if(volumen == 0 && count == 3){
                Serial.println("Paro de emergencia, Sin circulacion de agua");
                break;
              }
              count++;
            }
            Serial.println("El volumen total dezplazado es: ");
            Serial.println(volumen);
            Serial.println("Motor apagado");
            digitalWrite(RELE,HIGH);

            Lora_sender(volumen,hoyy,pause);
            volumen = 0;  
            sleep_start = true;
            
    }
    if(sleep_start){
      Serial.println("Sueño iniciado");
      Activate_alarm(true);
      esp_deep_sleep_start();
    }
}
