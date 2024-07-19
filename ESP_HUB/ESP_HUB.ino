//thư viện cần sài
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
String message ;
//-----------------------------------------------connect ESP room-------------------------------------------------

//Khai báo wifi local
const char *ssid_local = "MinhPhuoc";
const char *pass_loacl = "11223344";

//Khai báo websocksever
AsyncWebServer server(8000); //tạo websserver
AsyncWebSocket ws("/"); //nâng cấp server lên thành websocket

//sử lý dữ liệu nhận được
void analysisData(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0; // Kết thúc chuỗi dữ liệu bằng ký tự NULL
    Serial.printf("Received original data : %s\n", (char*)data);

    // Parse JSON data
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, (char*)data);
    if (!error) {
      float temperature = doc["Temp"];
      float humidity = doc["Hum"];
      int gas = doc["MQ2"];
      int light_pp = doc["PP"];
      Serial.printf("Temperature: %.2f, Humidity: %.2f, MQ2: %d , Light: %d \n", temperature, humidity,gas,light_pp);
      message = "Temp: ";
      message += String(temperature);
      message += ", Hum: ";
      message += String(humidity);
      message += ", MQ2: ";
      message += String(gas);
      message += ", PP: ";
      message += String(light_pp);
    } else {
      Serial.println("Failed to parse JSON");
    }
  }
}

//Sử lý các thông báo từ websocket
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    //sử lý kết nối
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    //sử lý ngắt kết nối
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    //sử lý data nhận được
    case WS_EVT_DATA:
      analysisData(arg, data, len);
      break;
    //sử lý lỗi
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

//-----------------------------------------------connect websever-------------------------------------------------

WiFiClient client;  // Create a WiFi client to establish a connection
PubSubClient mqtt_client(client); // Create an MQTT client

const char *ssid = "PhongTranhPiCasso";
const char *pass = "1234567890a";

const char *mqtt_server = "192.168.1.6"; // IP address of the MQTT broker
const int mqtt_port = 1883; // MQTT port
const char *mqtt_id = "esp32"; // MQTT client ID
const char *topic_pub = "fromESP";
const char *topic_sub = "toESP";

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Received data from: ");
  Serial.println(topic);
  Serial.println("Message: ");
  String messageInfo;
  for (size_t i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageInfo += (char)payload[i];
  }
  if (String(topic) == "toESP") {
    // Serial.print("Changing Room Light to ");
    // if (messageInfo == "on") {
    //   Serial.println("On");
    // } else if (messageInfo == "off") {
    //   Serial.println("Off");
    // }
  }
  Serial.println();
  Serial.println("------------------------------------------");
}






void setup() {
  Serial.begin(115200); // Start ESP32 serial communication

  ///////////////setup esp
  WiFi.softAP(ssid_local, pass_loacl); //setup ở chế độ phát wifi
  Serial.println(WiFi.softAPIP()); // in IP wifi được tạo

  ws.onEvent(onEvent); // Start listening for events on the WebSocket server
  server.addHandler(&ws); // Add the WebSocket server handler to the webserver
  server.begin(); // Start listening for socket connections

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT broker
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(callback);
  Serial.println("Connecting to MQTT...");
  while (!mqtt_client.connect(mqtt_id)) {
    delay(500);
  }
  Serial.println("Connected to MQTT");
  mqtt_client.subscribe(topic_sub);
  mqtt_client.publish(topic_pub, "Connection successful");

  delay(2000); // Ensure a small delay for the DHT sensor to stabilize
}

void loop() {
  mqtt_client.loop();
  delay(2000);
  mqtt_client.publish(topic_pub, message.c_str());
  ws.textAll(String("sendDataToHub"));

}


