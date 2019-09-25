// By @neoxelox https://github.com/Neoxelox/BProject

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "FS.h"
#include "SPIFFS.h"
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include "DHTesp.h"

//Credenciales de la RED WIFI
const char* ssid = "<ssid>";
const char* password = "<password>";

//Instancias del Servidor WEB
WebServer server(3553);
WebSocketsServer webSocket = WebSocketsServer(3554);

//Credenciales para conectarse a la Dashboard
const char* www_username = "<username>";
const char* www_password = "<password>";

// Static IP 
IPAddress ip(192, 168, 1, 21);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

//Ticker
Ticker timer;
#define rateSeconds 60

//RTC 3231
RtcDS3231<TwoWire> Rtc(Wire);
RtcDateTime now;
RtcTemperature RTCtemp;
char diaSemana[7][12] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado"};

//DHT11
DHTesp dht;
TempAndHumidity dhtValues;
float heatIndex;
float dewPoint;
#define dhtPin 23

//Alarms
//---DEPRECATED#define espRestart 8 //[h]

//RF RAWS
#define SHORT_0 390
#define SHORT_1 395
#define LONG_HEADER 3900
#define SPACE_COMMAND 16000
#define SEND_PIN 13
#define durCommands 1
 //Persianas - Habitacion xxxx
 String phaupCommand   = "";
 String phadownCommand = "";
 String phastopCommand = "";
 //Persianas - Habitacion xxxx
 String phpupCommand   = "";  
 String phpdownCommand = "";
 String phpstopCommand = "":
 //Persianas - Habitacion xxxxx
 String phcupCommand =   "";
 String phcdownCommand = "";
 String phcstopCommand = "";
 //Persianas - Habitacion xxxxx
 String phjupCommand =   "";
 String phjdownCommand = "";
 String phjstopCommand = "";
 
//DEBUG VARS
#define inbuiltLED 2

//Funciones WEB
//--Sirve blank page con Error
void handleNotFound() {
  String message = "Ruta Incorrecta\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//--Sirve index.html en SPIFFS
void serveMainPage() {
  File file = SPIFFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}
//Aqui se acaban las Funciones WEB

//Eventos SOCKETS
//Buffers JSON Estaticos
const size_t bufferSize = JSON_OBJECT_SIZE(5) + 131;
DynamicJsonBuffer jsonBuffer(bufferSize);
//--Evento Principal -- RECIBE TODOS LOS SOCKETS
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  //Si no compruebas la String CRASHEA
  if (type == WStype_TEXT) {
    Serial.printf("[WSc] get text: %s\n", payload);
    JsonObject& root = jsonBuffer.parseObject(payload);
    String type = root["type"];
    if (type == "command") handleUpCommand(root);
    else if (type == "actuator") handleUpActuator(root);
    else handleUpSensor(root);
  }
}

void handleUpCommand(JsonObject& root) {
  String command = root["command"];
  //-- Comando para Encender in-built LED
  if (command == "ledOn") {
    digitalWrite(inbuiltLED, 1);
    logDashboard("[Comando]: " + command + " | Encendiendo in-built LED...");
    Serial.println("[Comando]: " + command + " | Encendiendo in-built LED...");
  }
  //-- Comando para Apagar in-built LED
  else if (command == "ledOff") {
    digitalWrite(inbuiltLED, 0);
    logDashboard("[Comando]: " + command + " | Apagando in-built LED...");
    Serial.println("[Comando]: " + command + " | Apagando in-built LED...");
  }
  //-- Comando para cambiar hora RTC
  else if (command.substring(0,3) == "RTC") {
    now = (command.substring(3,7).toInt(),command.substring(7,9).toInt(),command.substring(9,11).toInt(),command.substring(11,13).toInt(),command.substring(13,15).toInt(),command.substring(15).toInt());
    Rtc.SetDateTime(now);
    now = Rtc.GetDateTime();
    String horacam = String(diaSemana[now.DayOfWeek( )]) + " " + String(now.Day()) + "/" + String(now.Month()) + "/" + String(now.Year()) + " " + String(now.Hour()) + ":" + String(now.Minute()) + ":" + String(now.Second());
    logDashboard("[Comando]: " + command + " | Hora cambiada a: " + horacam);
    Serial.println("[Comando]: " + command + " | Hora cambiada a: " + horacam);
  }
  //-- Comando para ver hora RTC
  else if (command == "inTime") {
    String horacam;
    if (!Rtc.IsDateTimeValid()) horacam = "[RTC lost confidence in the DateTime!]: ";
    now = Rtc.GetDateTime();
    horacam += String(diaSemana[now.DayOfWeek( )]) + " " + String(now.Day()) + "/" + String(now.Month()) + "/" + String(now.Year()) + " " + String(now.Hour()) + ":" + String(now.Minute()) + ":" + String(now.Second());
    RTCtemp = Rtc.GetTemperature();
    horacam += " [RTC TEMPERATURE]: " + String(RTCtemp.AsFloatDegC());
    logDashboard("[Comando]: " + command + " | Hora interna: " + horacam);
    Serial.println("[Comando]: " + command + " | Hora interna: " + horacam);
  }
  else if (command == "restart") {
    ESP.restart();
  }
}

void handleUpActuator(JsonObject& root) {
  String actuator_name = root["name"];
  //-- Handle BUTTON
  if (actuator_name == "pha") {
    String value = root["value"];
    if (value == "1") sendSig(phaupCommand,durCommands);
    else if (value == "2") sendSig(phastopCommand,durCommands);
    else if (value == "3") sendSig(phadownCommand,durCommands);
    else {Serial.println(actuator_name + " : " + map(value.toInt(),0,100,0,14)+1);}
    Serial.println(actuator_name + " : " + value);
  } else if (actuator_name == "php") {
    String value = root["value"];
    if (value == "1") sendSig(phpupCommand,durCommands);
    else if (value == "2") sendSig(phpstopCommand,durCommands);
    else if (value == "3") sendSig(phpdownCommand,durCommands);
    else {Serial.println(actuator_name + " : " + map(value.toInt(),0,100,0,14)+1);}
    Serial.println(actuator_name + " : " + value);
  } else if (actuator_name == "phc") {
    String value = root["value"];
    if (value == "1") sendSig(phcupCommand,durCommands);
    else if (value == "2") sendSig(phcstopCommand,durCommands);
    else if (value == "3") sendSig(phcdownCommand,durCommands);
    else {Serial.println(actuator_name + " : " + map(value.toInt(),0,100,0,14)+1);}
    Serial.println(actuator_name + " : " + value);
  } else if (actuator_name == "phj") {
    String value = root["value"];
    if (value == "1") sendSig(phjupCommand,durCommands);
    else if (value == "2") sendSig(phjstopCommand,durCommands);
    else if (value == "3") sendSig(phjdownCommand,durCommands);
    else {Serial.println(actuator_name + " : " + map(value.toInt(),0,100,0,14)+1);}
    Serial.println(actuator_name + " : " + value);
  }
}

void handleUpSensor(JsonObject& root) {
  String sname = root["name"];
  String value = root["value"];
  Serial.println(sname + " : " + value);
}

//--Evento para ENVIAR SOCKETS
void logDashboard(String json) {
  json = "{\"type\":\"log\",\"value\":\"" + json + "\"}";
  webSocket.broadcastTXT(json.c_str(), json.length());
}
void sendToDashboard(String& json) {
  webSocket.broadcastTXT(json.c_str(), json.length());
}
//Aqui se acaban los Eventos SOCKETS

void setup(void) {
  
  SPIFFS.begin();
  Serial.begin(115200);
  
  pinMode(inbuiltLED, OUTPUT);
  digitalWrite(inbuiltLED, 0);
   pinMode(SEND_PIN, OUTPUT);
  digitalWrite(SEND_PIN, 0);
  
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  Rtc.Begin();
  //RtcDateTime compiledTime = RtcDateTime(__DATE__, __TIME__);
  //Rtc.SetDateTime(compiledTime);

  dht.setup(dhtPin, DHTesp::DHT11);
  
  Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("IP Public: xxxxx");

  //Respondedor FDNS
  if (MDNS.begin("dashboard")) {
    Serial.println("MDNS responder started");
  }

  //Eventos WEB
  //--Evento MAIN
  server.on("/", []() {
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    serveMainPage();
  });
  
  //--Evento CERRAR SESION
  server.on("/cerrar", []() {
    server.requestAuthentication();
    if (!server.authenticate(www_username, www_password)) {
      server.send(200, "text/plain", "Has cerrado Sesión correctamente.");
    }
  });

  //--Evento NOT FOUND
  server.onNotFound(handleNotFound);
  //Aqui se acaban los Eventos WEB
  
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  Serial.println("HTTP server started");
  
  timer.attach(rateSeconds, looper);
}

void loop(void) {
  webSocket.loop();
  server.handleClient();
}

void looper() {
    if (!Rtc.IsDateTimeValid()) Serial.println("RTC lost confidence in the DateTime!");
    now = Rtc.GetDateTime();

    // Check ALARMS
    
    // Reconnect WiFi if Down
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.begin(ssid,password);
    }

    // ------------
        
    Serial.println(String(diaSemana[now.DayOfWeek( )]) + " " + String(now.Day()) + "/" + String(now.Month()) + "/" + String(now.Year()) + " " + String(now.Hour()) + ":" + String(now.Minute()) + ":" + String(now.Second()));
    RTCtemp = Rtc.GetTemperature();
    Serial.print(RTCtemp.AsFloatDegC());
    Serial.println("C");

    // Send SENSOR Data
    
    // DHT-11
      dhtValues = dht.getTempAndHumidity();
      if (dht.getStatus() != 0) {Serial.println("DHT11 error status: " + String(dht.getStatusString()));}
      heatIndex = dht.computeHeatIndex(dhtValues.temperature, dhtValues.humidity);
      dewPoint = dht.computeDewPoint(dhtValues.temperature, dhtValues.humidity);
      String dJson = "{\"type\":\"sensor\",\"name\":\"dht11\",\"temp\":" + String(dhtValues.temperature) + ",\"hum\":" + String(dhtValues.humidity) + ",\"hinx\":" + String(heatIndex) + ",\"dwpo\":" + String(dewPoint) + "}";
      sendToDashboard(dJson);
    // ----------------
}

void sendSig(String sig, int duration) {
  if (duration < 1)duration = 1;
  if (duration > 90)duration = 90;
  unsigned long entry = millis();
  while (millis() < entry + duration * 1000) {
    for (int i = 0; i < sig.length(); i++) {
      switch (sig[i]) {
        case '1':  // short pulse
          sigPuls(SHORT_1);
          break;
        case '0':  //short space
          delayMicroseconds(SHORT_0);
          break;
        case 'H':  // long space [header] (pause)
          delayMicroseconds(LONG_HEADER);
          break;
        case 'N':  // new command
          delayMicroseconds(SPACE_COMMAND);
          break;
        default:
          Serial.println("Error");
          break;
      }
    }
  }
}

void sigPuls(int duration) {
  digitalWrite(SEND_PIN, HIGH);
  delayMicroseconds(duration);
  digitalWrite(SEND_PIN, LOW);
}
