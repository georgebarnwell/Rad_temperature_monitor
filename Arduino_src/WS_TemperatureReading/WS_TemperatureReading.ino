// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>   // Include the WebServer library
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 2; // D4 

const int sizePerJsonObject = 128;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Temp sensor devices
int nDevices = 0;
DeviceAddress tempDeviceAddress;
char deviceAddressBuffer[17];

// Replace with your network credentials
const char* ssid     = "ENTER_SSID_IN_THE_QUOTES";
const char* password = "ENTER_PASSWORD_IN_THE_QUOTES";

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

void handleRoot();              // function prototypes for HTTP handlers
void handleNotFound();

void setup() {
  Serial.begin(9600);

  delay(2000);

  sensors.begin();

  // Connect to Wi-Fi network with SSID and password
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

  server.begin();                           // Start the server
  Serial.println("HTTP server started");
}

void loop(){
  server.handleClient();
}

String getAddressString(DeviceAddress deviceAddress, boolean serialPrint) {
  String addressStr = "";
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) {
      addressStr = addressStr + "0";
      if (serialPrint) Serial.print("0");
    }
    addressStr = addressStr + String(deviceAddress[i], HEX);
    if (serialPrint) Serial.print(deviceAddress[i], HEX);
  }
  return addressStr;
}

void handleRoot() {
  nDevices = sensors.getDeviceCount();

  Serial.print("Number of sensors is ");
  Serial.print(nDevices);

  sensors.requestTemperatures(); 

  DynamicJsonDocument arrayDoc(sizePerJsonObject * (nDevices));
  JsonArray sensorsJsonArray = arrayDoc.to<JsonArray>();

  for (int i = 0; i < nDevices; i++) {
    if(sensors.getAddress(tempDeviceAddress, i)) {
      Serial.print("Temperature for device with address: ");
      String deviceAddress = getAddressString(tempDeviceAddress, true);
      Serial.println("");
      float temperatureCByAddress = sensors.getTempC(tempDeviceAddress);
      Serial.print(temperatureCByAddress);
      Serial.println("ºC");
      Serial.println("");

      DynamicJsonDocument objDoc(sizePerJsonObject * nDevices);

      JsonObject sensorInfo = objDoc.to<JsonObject>();

      sensorInfo["SensorID"] = deviceAddress;
      sensorInfo["TempDegC"] = temperatureCByAddress;

      serializeJson(sensorInfo,Serial);

      sensorsJsonArray.add(sensorInfo);
    }
  }

  String serialisedJson;
  serializeJson(arrayDoc,serialisedJson);
  serializeJson(arrayDoc,Serial);
  
  server.send(200, "text/json", serialisedJson);   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found. Go to root to get temperatures"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}