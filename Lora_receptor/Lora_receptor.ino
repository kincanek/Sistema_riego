// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SD.h"
#include <SPI.h>

File myFile;


//---------------------------------------------------------------------
const char* ssid = "Canek_server";
const char* password = "123456789";
//---------------------------------------------------------------------
bool ledState = 0;
const int ledPin = 25;
//----------------------------------------------------------------------
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
//**************************************************************************************************
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <script type="text/javascript" src="swfobject.js"></script>
    <script type="text/javascript" src="web_socket.js"></script>
    <meta http-equiv="content-type" content="text/html; charset=windows-1252">
    <title>ESP Web Server</title>
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css"
      integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr"
      crossorigin="anonymous">
    
    <link href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css"
      rel="stylesheet">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"> </script>
    <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">


    <style>
        html {font-family: Arial; display: inline-block; text-align: center;}
        p { font-size: 1.4rem;}
        body {  margin: 0;}
        .topnav { overflow: hidden; background-color: #50B8B4; color: rgb(255, 255, 255); font-size: 1rem; }
        .content { padding: 30px; }
        .card { background-color: rgb(123, 183, 190) box-shadow:; 2px 2px 12px 1px rgba(140,140,15,.5); }
        .cards { max-width: 800px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); }
        intput,
          label{
            margin: .4rem 0;
          
    </style>

  </head>
  <body>
    <div class="topnav">
      <h1>Sistema de control y monitoreo</h1>
    </div>

    <div class="content">
      <div class="cards">

        <div class="card">
          <p><i class="fas fa-tint" style="color:#00add6;"></i> PROGRAMAR RIEGO </p>
        
            <select id="week" name="Dia">
              <option value="0">Domingo</option>
              <option value="1">Lunes</option>
              <option value="2">Martes</option>
              <option value="3">Miercoles</option>
              <option value="4">Jueves</option>
              <option value="5">Viernes</option>
              <option value="6">Sabado</option>
            </select>

          <input id="hora" name="appt" min="09:00" max="18:00" required="" type="time">
          <p><br>
          </p>
          <button id="adicionar" onclick="Registrar()">Agregar</button>

          <div class="card">
            <p><br>
            </p>
        
            <table id="mytable" class="table table-bordered table-hover " style="width: 347px; height: 38px;">
              <caption style="text-align:center">Registro de alarmas</caption>
              <tbody>
                <tr>
                  <th style="text-align: center;">Dia Semana</th>
                  <th style="text-align: center;">Hora y minutos</th>
                  <th style="text-align: center;">Eliminar</th>
                </tr>
              </tbody>
            </table>
            <button id="enviar" onclick="Send_lora()">Enviar</button>
          </div>
        </div>

        <div class="card">
          <button id="encender" onclick="encender_motor()">Encender riego manual</button>
          <p class="state">State: <span id="state">%STATE%</span></p>
        </div>
        
        <div class="card">
          <button id="visualizar" onclick="vew_data()">Mostrar datos</button>
          <div id="table_div"></div>
        </div>
        <div class="card">
          <div id="Grafica"></div>
        </div>

      </div>
     

      <script type="text/javascript">
      WEB_SOCKET_SWF_LOCATION = "WebSocketMain.swf";
      var gateway = `ws://${window.location.hostname}/ws`;
      var websocket;
      google.charts.load('current', {'packages':['table','corechart']});
      var data_ms;
      var data_split;
      

      window.addEventListener('load', onLoad);

        function initWebSocket() {
          console.log('Trying to open a WebSocket connection...');
          websocket = new WebSocket(gateway);
          websocket.onopen    = onOpen;
          websocket.onclose   = onClose;
          websocket.onmessage = onMessage; // <-- add this line
        }
        function onOpen(event) {
          console.log('Connection opened');
        }
        function onClose(event) {
          console.log('Connection closed');
          setTimeout(initWebSocket, 2000);
        }

        function onMessage(event) {
          data_ms = event.data;

        }

        function onLoad(event) {
          initWebSocket();
        }

        function encender_motor(){
    
          websocket.send("Enciende");
          var myVar = setInterval(myTimer2,3000);
        
        }
        function myTimer2(){
          
        if(data_ms == "ON" || data_ms == "OFF" ){
          document.getElementById("state").innerHTML = data_ms;
        }
        }

        function Send_lora(){
          var data_read = document.getElementById("mytable");
          var total_filas = data_read.rows.length;
          var i;
          var day=[];
          var time_ = [];

          if(total_filas != 0){
            for (i=1; i<total_filas;i++){
              day.push(data_read.rows[i].cells[0].innerText);
              time_.push(data_read.rows[i].cells[1].innerText);
            }
            websocket.send(day);
            websocket.send(time_);
            
          }

        }

        function vew_data(){
          websocket.send("Leer");
          var myVar = setInterval(myTimer,3000);
        }
        
        function myTimer(){
          google.charts.setOnLoadCallback(drawDateFormatTable);
        }
        
        function drawDateFormatTable(event){
          data_split = data_ms.split(",");
          var i;
          var len_data = (data_split.length-1)/3;
          if(data_split.length != 1){
            var fecha;
            var hora;
            var data_base = new google.visualization.DataTable();
            data_base.addColumn('date','Fecha inicio');
            data_base.addColumn('number','Litros');
            data_base.addRows(len_data);
            var j = 0;
            for(i=0;i<len_data;i++)
            {
              
             fecha = data_split[j].split("/")
             hora = data_split[1+j].split(":")
             data_base.setCell(i, 0, new Date(fecha[0],fecha[1],fecha[2],hora[0],hora[1],0));
             data_base.setCell(i, 1,parseFloat(data_split[2+j]));
             j = j + 3;
            }
           
            
            var formatter_long = new google.visualization.DateFormat({formatType: 'long'});
            formatter_long.format(data_base, 0);
            var table = new google.visualization.Table(document.getElementById('table_div'));
            table.draw(data_base, {showRowNumber: true, width: 'auto', height: 'auto'});
           
            var options = {
            title: 'GRAFICA',
            width: 800,
            height: 400,
            hAxis: {
              format: 'M/d/yy,hh:mm',
              gridlines: {count: 15}
            },
            vAxis: {
              gridlines: {color: 'none'},
              minValue: 0
            }
          };
          
          var chart = new google.visualization.LineChart(document.getElementById('Grafica'));
          chart.draw(data_base,options);
          
          }

         
        }

        function Registrar(){
          var dia_semana = document.getElementById("week").value;
          var hora_pro = document.getElementById("hora").value;
          var i = 1;
          if(dia_semana==0){
            dia_semana = "Domingo";
          }
          else if(dia_semana==1){
            dia_semana = "Lunes";
          }
          else if(dia_semana==2){
            dia_semana = "Martes";
          }
          else if(dia_semana==3){
            dia_semana = "Miercoles";
          }
          else if(dia_semana==4){
            dia_semana="Jueves";
          }
          else if(dia_semana==5){
            dia_semana = "Viernes";
          }
          else{
            dia_semana = "Sabado";
          }

          $("#mytable").append('<tr id="row' + i + '"><td>' + dia_semana +  '</td>' + '<td>' + hora_pro + "<td>"+ "<button type='button' onclick='productDelete(this);' class='btn btn-default'>" +"<span class='glyphicon glyphicon-remove' />" +"</button>" + "</td>" +"</tr>");
          
          i++;
        }

        function productDelete(ctl) {
          $(ctl).parents("tr").remove();
        }
    </script>
    </div>
  </body>
</html>
)rawliteral";
//************************************************************************************************


void readFile(){
  myFile = SD.open("/data.txt","r");
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
    Serial.println("No open data");
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
      Serial.println((char*)data);
      
      if (strcmp((char*)data, "Enciende") == 0)
      {
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
        readFile();
      }
      
  }
}


void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
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

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  SPI.begin(14,2,15,13);
  if (!SD.begin())
  {
    Serial.println("initialization failed!");
    return;
  }
  else{
    Serial.println("SD montada");
  }
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
 
  WiFi.softAP(ssid, password);

  initWebSocket();
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.begin();
  
}

void loop() {
  ws.cleanupClients();
  digitalWrite(ledPin, ledState);
}
