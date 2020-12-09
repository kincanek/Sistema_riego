#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SD.h"
#include <SPI.h>
#include <LoRa.h>
#include <SPIFFS.h>
#include <Separador.h>

SPIClass LoRaSPI;

Separador s;

String mensajes = "";
volatile bool on_of = false;

//"https://canvasjs.com/assets/script/canvasjs.min.js"
File myFile;


#define SPF 12
#define LORA_SCK     5    // GPIO5  -- SX1278's SCK
#define LOAR_MISO    19   // GPIO19 -- SX1278's MISO
#define LORA_MOSI    27   // GPIO27 -- SX1278's MOSI
#define LORA_CS      18   // GPIO18 -- SX1278's CS
#define LORA_RST     23   // GPIO14 -- SX1278's RESET
#define LORA_IRQ     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

//---------------------------------------------------------------------
const char* ssid = "Canek_server";
const char* password = "123456789";
//---------------------------------------------------------------------
bool ledState = 0;
const int ledPin = 25;
volatile bool estado_lora = true;
long t0= 0;
char* decifre_msg = "";
String msgString = "";
//----------------------------------------------------------------------
AsyncWebServer server(80);

AsyncWebSocket ws("/ws"); 


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
     
      if (strcmp((char*)data,"Enciende") == 0)
      {
        Serial.println((char*)data);
        ledState = !ledState;
        
        if(ledState){
          //enviar_msg("ON");
          mensajes = "ON";
          on_of = true;
         }
         else{
          //enviar_msg("OFF");
          mensajes = "OFF";
          on_of = true;
          }
        
      }
      if (strcmp((char*)data, "Leer") == 0)
      {
        Serial.println((char*)data);
        readFile("/data.txt");
        
      }
      if (strcmp((char*)data, "LeerAlarm") == 0)
      {
        Serial.println((char*)data);
        readFile("/alarma.txt");
        
      }
      if(strcmp((char*)data, "Leer") != 0 && strcmp((char*)data, "Enciende") != 0){
        mensajes =  String((char*)data);
        on_of = true;
        //enviar_msg(msgString);
       //Serial.println(msgString);
 
      }
      
  }
  
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
      AwsEventType type,void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", 
              client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void readFile(String paht){
  myFile = SD.open(paht,"r");
  if(myFile){
    Serial.println("Data: ");
    String inString;
    while(myFile.available()){
      inString += myFile.readString();
    }
    myFile.close();
    Serial.println(inString);
    ws.textAll(inString);
  }else{
    Serial.println("Error al leer el archivo");
  }

}
void appendFile(fs::FS &fs, const char * path, String message){
    Serial.printf("Appending to file: %s\n", path);
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Error al abrir el archivo");
        return;
    }
    if(file.println(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


String processor(const String& var){
  Serial.println(var);
  
  if(ledState){
    return "ON";
  }
  else{
    return "OFF";
  }

}

void enviar_msg(String msg){
  LoRa.beginPacket();                   // start packet
  LoRa.print(msg);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
}

void recibe_msg(int packetSize){
  if (packetSize == 0) return;
  String packet = "";
  while (LoRa.available()) {
    packet += (char)LoRa.read();
  }
  Serial.println(packet);
  if(packet == "ON"){
    ws.textAll("ON");
  }
  else if(packet == "OFF"){
    ws.textAll("OFF");
  }
  else if(packet == "Alarma"){
    ws.textAll("Alarmas activadas correctamente");
  }else if(packet == "AlarmaE"){
    ws.textAll("Error al activar alarmas");
  }
  else{
    appendFile(SD,"/data.txt",packet);
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
void setup(){
// inicializacion del puerto serial
  Serial.begin(115200);
  
  if(!SPIFFS.begin())
  {
    Serial.println("Error spiff");
    while(1);
  }
  
  Init_SD();
  Init_Lora();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  WiFi.softAP(ssid, password);

  initWebSocket();
    server.on("/canvasjs.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/canvasjs.min.js","text/javascript");
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html",String(),false, processor);
  });


  server.begin();

  
}

void loop() {
  ws.cleanupClients();
  digitalWrite(ledPin, ledState);
  if(on_of){
    enviar_msg(mensajes);
    on_of = false;
    Serial.print(mensajes);
    delay(100);

  }else{
    recibe_msg(LoRa.parsePacket());
  }
  

}
