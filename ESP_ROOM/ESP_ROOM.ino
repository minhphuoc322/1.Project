#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <DHT.h>

#define MQ2 34       // Pin connected to the MQ2 gas sensor
#define PP 35        // Pin connected to the additional analog sensor
#define DHTPIN 32     // Pin connected to the DHT sensor
#define DHTTYPE DHT11 // Type of DHT sensor
DHT dht(DHTPIN, DHTTYPE);

int gas;
int light_pp;
WebSocketsClient webSocket;

const char *ssid = "MinhPhuoc";
const char *password = "11223344";
const char *server_ip = "192.168.4.1"; // IP của ESP32 server
const uint16_t port = 8000;

void handleWebSocketMessage(uint8_t *payload, size_t length) {
  // Kết thúc chuỗi dữ liệu bằng ký tự NULL
  payload[length] = 0;
  Serial.printf("Received: %s\n", (char*)payload);

  if (strcmp((char*)payload, "sendDataToHub") == 0) {
    // Đọc dữ liệu từ cảm biến DHT11
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    light_pp = analogRead(PP);
    gas = analogRead(MQ2);

    // Tạo JSON object
    StaticJsonDocument<200> doc;
    doc["Temp"] = temperature;
    doc["Hum"] = humidity;
    doc["MQ2"] = gas;
    doc["PP"] = light_pp;
    String jsonString;
    serializeJson(doc, jsonString);

    // Gửi dữ liệu tới server
    webSocket.sendTXT(jsonString);

    Serial.printf("Temperature: %.2f, Humidity: %.2f, MQ2: %d , Light: %d \n", temperature, humidity,gas,light_pp);
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("WebSocket connected to server");
      break;
    case WStype_DISCONNECTED:
      Serial.println("WebSocket disconnected from server");
      break;
    case WStype_TEXT:
      handleWebSocketMessage(payload, length);
      break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_PING:
    case WStype_PONG:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  dht.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  webSocket.begin(server_ip, port);
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
}
