#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>

const char* ssid = "Nikita-Pipitka";
const char* password = "Nikshadow92";

ESP8266WebServer server(80);
HTTPClient httpClient; 

const int led = 13;

const int RELAY_1_IN = 2; // (D4)
const int RELAY_2_IN = 4; // (D2)
int relay1state = 0;
int relay2state = 0;

int requestsCount = 0;

const String moduleName = "Smart power supply";
const String devices = "relay1,relay2";
const int relaysCount = 2;

const String SERVER_IP = "192.168.0.192";

String mainPage = "<!DOCTYPE html> <html> <head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <script> function renderControls() { var devices = document.getElementById(\"devices\").innerHTML.split(\",\"); var switchHtml = document.getElementById('switch-template').outerHTML; /* Render */ var root = document.getElementById('controls'); var controlsToAdd = \"\"; var deviceIndex = 0; devices.forEach(function(device) { deviceIndex++; var controlHtml = String(switchHtml).replaceAll(\"#switch-name\", device).replace('#switch-num', deviceIndex); controlsToAdd = controlsToAdd + controlHtml; }); root.innerHTML = controlsToAdd; /* Hide template */ document.getElementById('switch-template').style.display = 'none'; } </script> <script> function handleCheck(elId, switchIndex) { var checkbox = document.getElementById(elId); if (checkbox.checked) { var xhr = new XMLHttpRequest(); xhr.open('GET', window.location.protocol + \"//\" + window.location.host + '/' + switchIndex + '_1', true); xhr.send(null); } else { var xhr = new XMLHttpRequest(); xhr.open('GET', window.location.protocol + \"//\" + window.location.host + '/' + switchIndex + '_0', true); xhr.send(null); } } </script> <style> .switch { position: absolute; display: inline-block; width: 60px; height: 34px; right: 10px; } .switch input { opacity: 0; width: 0; height: 0; } .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; -webkit-transition: .4s; transition: .4s; } .slider:before { position: absolute; content: \"\"; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; -webkit-transition: .4s; transition: .4s; } input:checked + .slider { background-color: #2196F3; } input:focus + .slider { box-shadow: 0 0 1px #2196F3; } input:checked + .slider:before { -webkit-transform: translateX(26px); -ms-transform: translateX(26px); transform: translateX(26px); } /* Rounded sliders */ .slider.round { border-radius: 34px; } .slider.round:before { border-radius: 50%; } .panel { height: 50px; position: relative; border: thick double #32a1ce; background-color: rgb(148, 212, 231); /*padding: 9px 9px 0px 9px;*/ width: 92%; display: flex; align-items: center; box-shadow: 0 0 2px #2196F3; } .panel-label { position: absolute; left: 10px; width: 65%; font-family: cursive; overflow: hidden; } </style> </head> <body onload=\"renderControls()\"> <!-- Settings provided here. --> <p id=\"devices\" hidden>#devices</p> <h2>#title</h2> <!-- Template for switch control. Invisible.  --> <div id=\"switch-template\" class=\"panel\"> <div class=\"panel-label\"> <label>#switch-name</label> </div> <label class=\"switch\"> <input id=\"#switch-name\" type=\"checkbox\" onchange=\"handleCheck('#switch-name', '#switch-num')\"> <span class=\"slider round\"></span> </label> </div> <div id=\"controls\"> <!-- All switches will be here. --> </div> </body> </html>";

String getIndexPage() {
  return mainPage;
}

void handleRoot() {
  server.send(200, "text/html", getIndexPage());
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void setup(void){
  // UI
  mainPage.replace("#devices", devices);
  mainPage.replace("#title", moduleName);
  
  pinMode(led, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.hostname("relays");
  WiFi.begin(ssid, password);
  
  Serial.println("");

  // Setup relay
  pinMode(RELAY_1_IN, OUTPUT);
  pinMode(RELAY_2_IN, OUTPUT);
  digitalWrite(RELAY_1_IN, LOW);
  digitalWrite(RELAY_2_IN, LOW);

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

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/1_1", [](){
    digitalWrite(RELAY_1_IN, HIGH);
    relay1state = 1;
    anyRequestHandled();
    server.send(200, "text/plain", "relay 1 on");
  });

  server.on("/1_0", [](){
    digitalWrite(RELAY_1_IN, LOW);
    relay1state = 0;
    anyRequestHandled();
    server.send(200, "text/plain", "relay 1 off");
  });

  server.on("/2_1", [](){
    digitalWrite(RELAY_2_IN, HIGH);
    relay2state = 1;
    anyRequestHandled();
    server.send(200, "text/plain", "relay 2 on");
  });

  server.on("/2_0", [](){
    digitalWrite(RELAY_2_IN, LOW);
    relay2state = 0;
    anyRequestHandled();
    server.send(200, "text/plain", "relay 2 off");
  });

  server.on("/state", []() {
    server.send(200, "text/plain", "relay 1 state " + String(relay1state) + "\nrelay 2 state " + String(relay2state));
  });

  server.on("/rc", []() {
    server.send(200, "text/plain", "Requests count: " + String(requestsCount));
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP client started");
}

boolean registerOnServer() {
   httpClient.begin("http://" + SERVER_IP + "/register");
   httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");
   int httpResponseCode = httpClient.POST("devices=" + devices + "&client-ip=" + WiFi.localIP().toString() + "&client-name=" + moduleName);
   httpClient.end();
   if (httpResponseCode == 200) {
      Serial.println("Client successfully registered on server " + SERVER_IP);
      return true;
   }
   Serial.println("Client cannot register on server " + SERVER_IP);
   Serial.println("Error code " + String(httpResponseCode));
   return false;
};

void anyRequestHandled() {
    requestsCount++;
}

boolean registeredOnServer = false;

void loop(void){
  server.handleClient();
  if (!registeredOnServer) {
    delay(10000);
    if (registerOnServer()) {
      registeredOnServer = true;
    }
  }
}
