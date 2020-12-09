
#include <Wire.h>
#include <ErriezDS3231.h>
#include "RTClib.h"
#include <LoRa.h>
#include <SPI.h>
#include <Separador.h>
#include "SD.h"

Separador s;
SPIClass LoRaSPI;

ICACHE_RAM_ATTR

ErriezDS3231 Clock; 
RTC_DS3231 rtcc;

// Pin entrada de interrupcion del reloj
#define INT_PIN 35
// Pin GPIO 04 Salida de ecendido 
#define RELE 4 
// Pin GPIO 25 Entrada de interrupcion Sensor de agua
#define Sensor 25 

#define SPF 12
#define LORA_SCK     5    // GPIO5  -- SX1278's SCK
#define LOAR_MISO    19   // GPIO19 -- SX1278's MISO
#define LORA_MOSI    27   // GPIO27 -- SX1278's MOSI
#define LORA_CS      18   // GPIO18 -- SX1278's CS
#define LORA_RST     23   // GPIO14 -- SX1278's RESET
#define LORA_IRQ     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

volatile bool alarmInterrupt = false; 
volatile int count = 0; 

bool sleep_start = true;

const int T_Muestreo = 3000; 
const int tiempo_riego = 1;  
volatile int packetSize;

bool alarm_especifica = false; 
const int alarma1[] = {11, 
                        6, 
                        2, 
                        0}; 

bool alarm_periodica = true; 
int rows; 
int alarms[50][3];

const float K = 6.997; 
float volumen = 0; 
long t0 = 0; 

void contador()
{
    count++;
}

void onAlarm()
{
    alarmInterrupt = true;
}

void setAlarm2()
{
  DateTime hoy = rtcc.now();
  int index_alarm = 0;

  for(int i=0;i < rows;i++){
    if(alarms[i][0]>=hoy.dayOfTheWeek()){
      if(alarms[i][0]==hoy.dayOfTheWeek()){
        if(alarms[i][1] == hoy.hour() && alarms[i][2] > hoy.minute() ){
          index_alarm = i;
          break;
        }
        if(alarms[i][1] > hoy.hour()){
          index_alarm = i;
          break;
        }
      }
      if(alarms[i][0] != hoy.dayOfTheWeek()){
        index_alarm = i;
        break;
      }
    }
  }
  
  Serial.println("Alarma para riego programada");
  Clock.setAlarm2(Alarm2MatchDay,
              alarms[index_alarm][0], 
              alarms[index_alarm][1], 
              alarms[index_alarm][2]); 

  Clock.alarmInterruptEnable(Alarm2, true);
    
}

void setAlarm1()
{
  Serial.println("Alarma para riego programada");
  
  Clock.setAlarm1(Alarm1MatchDate,
                alarma1[0], 
                alarma1[1],
                alarma1[2], 
                alarma1[3]); 
    
  Clock.alarmInterruptEnable(Alarm1, true);
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
  LoRa.print(start.minute());
  LoRa.print(",");
  LoRa.print(mensaje);
  LoRa.print(",");
  LoRa.endPacket();
}

void In_volumen(float dV)
{
  volumen += dV / 60 * (millis() - t0) / 1000.0;
  t0 = millis();
}

float Frecuencia()
{
  count = 0;
  delay(T_Muestreo); 
  return (float)count * 1000 / T_Muestreo;
}


void Medir( )
{
  float caudal = Frecuencia() / K ; 
  In_volumen(caudal); 

}
void Stop_alarm(){
  if(Clock.getAlarmFlag(Alarm2))
  {
    Clock.clearAlarmFlag(Alarm2);
  }
  if(Clock.getAlarmFlag(Alarm1))
  {
    Clock.clearAlarmFlag(Alarm1);
  }
}

void Activate_alarm(){

    if (alarm_especifica){
      setAlarm1();
    }
    else{
      Clock.alarmInterruptEnable(Alarm1, false);
    }

    if(alarm_periodica){
      setAlarm2();
    }
    else{
      Clock.alarmInterruptEnable(Alarm2, false);
    }   

}

void enviar_msg(String msg){
  LoRa.beginPacket(); 
    LoRa.print(msg);   
  LoRa.endPacket();  
}

bool prog = true;
int check = 0;


void recibe_msg(int packetSize){
  if (packetSize == 0) return;
  String packet = "";
  String last_caracter = "";
  while (LoRa.available()) {
    last_caracter = (char)LoRa.read();
    packet += last_caracter;
  }
  Serial.println(packet);
  if(packet == "LeerAlarm"){
    enviar_msg("En proceso");
  }
  else if(packet != "ON" || packet != "OFF"){
    if(last_caracter == "A"){
      String var_tem = s.separa(packet,',',0);
      rows = var_tem.toInt();
  
      for(int i = 1; i <= rows; i++){
        String var_tem = s.separa(packet,',',i);
        String tiem_tem = s.separa(packet,',',i + rows);
        String H = s.separa(tiem_tem,':',0);
        String M = s.separa(tiem_tem,':',1);

        alarms[check][0] = var_tem.toInt();
        alarms[check][1] = H.toInt();
        alarms[check][2] = M.toInt();
        Serial.print(alarms[check][0]);
        Serial.print(alarms[check][1]);
        Serial.print(alarms[check][2]);
        check++;
      }
    
      check = 0;
      Stop_alarm();
      Activate_alarm();
      enviar_msg("Alarma");
    }else{
      enviar_msg("AlarmaE");
    }
  }
}
void Init_SD(){
  SPI.begin(14,2,15,13);
   if (!SD.begin(13))
  {
    Serial.println("initialization failed!");
    return;
  }
  else{
    Serial.println("SD montada");
  }
}
void Init_Lora(){

  LoRaSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setSPI(LoRaSPI);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  if (!LoRa.begin(915E6)) { 
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSpreadingFactor(SPF);  
  LoRa.setTxPower(17, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSyncWord(0xF3);
}
void setup()
{
    Serial.begin(115200);
    //Init_SD();
    Init_Lora();
    
    Wire.begin();
    Wire.setClock(400000);
    Clock.setSquareWave(SquareWaveDisable);
    

    pinMode(Sensor,INPUT);
    attachInterrupt(digitalPinToInterrupt(Sensor), contador, RISING);
    interrupts();
    
    pinMode(INT_PIN, INPUT_PULLUP); 
    attachInterrupt(digitalPinToInterrupt(INT_PIN), onAlarm, FALLING);
    
    pinMode(RELE,OUTPUT); 
    digitalWrite(RELE, HIGH); 
    Serial.println("Tarjeta encendida");
}

void loop()
{
  if (alarmInterrupt) {
    Stop_alarm();
    alarmInterrupt = false; 
    enviar_msg("ON");
    Serial.print("Alarma activada");
            
    DateTime hoyy = rtcc.now(); 
    DateTime pause (hoyy.unixtime()+ tiempo_riego * 60); 

    digitalWrite(RELE,LOW);
    int count = 0;
  
    while(true){
      DateTime hoy = rtcc.now();
      if(hoy.hour() == pause.hour() && hoy.minute()==pause.minute()){
        break;
      }
      Medir();

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
    Activate_alarm();
    delay(30);
    enviar_msg("OFF");
            
    }
  packetSize = LoRa.parsePacket();
  if (packetSize) { recibe_msg(packetSize);  }

}
