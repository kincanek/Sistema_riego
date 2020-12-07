#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SD.h"
#include <SPI.h>
#include <LoRa.h>
#include <SPIFFS.h>

SPIClass LoRaSPI;

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
//----------------------------------------------------------------------
AsyncWebServer server(80);

AsyncWebSocket ws("/ws"); // crea un objeto llamado ws para manejar las conexiones en el camino 


void readFile(){
  myFile = SD.open("/data.txt","r");
  if(myFile){
    Serial.println("Data: ");
    String inString;
    while(myFile.available()){
      inString += myFile.readString();
    }
    myFile.close();
    //Imprime mensaje 
    Serial.println(inString);
    // Envia mensaje
    ws.textAll(inString);
  }else{
    Serial.println("Error al leer el archivo");
  }

}
void appendFile(fs::FS &fs, const char * path, const char * message){
  
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
/*
 * Un WebSocket es una conexión persistente entre un cliente 
 * y un servidor que permite la comunicación bidireccional entre 
 * ambas partes mediante una conexión TCP. Esto significa que puede 
 * enviar datos del cliente al servidor y del servidor al cliente en cualquier momento. 


 */
 /*
  * La funcion es una funcion de devolucion de llamada que se ejecutar siempre que se reciba 
  * nuevos datos de los clientes a travez del protocolo websocket
  */
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
     
      if (strcmp((char*)data, "Enciende") == 0)
      {
        Serial.println((char*)data);
        ledState = !ledState;
        
        if(ledState){
          ws.textAll("ON");
         }
         else{
          ws.textAll("OFF");
          }
        
      }
      if (strcmp((char*)data, "Leer") == 0)
      {
        Serial.println((char*)data);
        readFile();
      }
      if(strcmp((char*)data, "Leer") != 0 && strcmp((char*)data, "Enciende") != 0){
        Serial.println((char*)data);
        enviar_msg((char*)data);
        
      }
  }
}

/*
 * Detector de eventos para manejar los diferentes pasos asincronicos del protocolo websocket
 */
 /*
  * WS_EVT_CONNECT cuando un cliente ha iniciado sesión;
WS_EVT_DISCONNECT cuando un cliente se ha desconectado;
WS_EVT_DATA cuando se recibe un paquete de datos del cliente;
WS_EVT_PONG en respuesta a una solicitud de ping;
WS_EVT_ERROR cuando se recibe un error del cliente.
  */
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

void enviar_msg(char* msg){
  LoRa.beginPacket();                   // start packet
  LoRa.print(msg);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
}

void recibe_msg(int packetSize){
  if (packetSize == 0) return;          // if there's no packet, return
  // read packet header bytes:
  const char* packet = "";
 
  while (LoRa.available()) {
    packet += (char)LoRa.read();
  }
  Serial.println(packet);
  //appendFile(SD,"/data.txt",packet);
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
}
