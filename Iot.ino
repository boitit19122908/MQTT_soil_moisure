#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Cấu hình chân cảm biến và relay
#define SOIL_MOISTURE_PIN 34  // Cảm biến độ ẩm đất
#define RELAY1_PIN 2          // Relay 1
#define RELAY2_PIN 5          // Relay 2
#define ONE_WIRE_BUS 4        // Chân DATA của DS18B20

// Wi-Fi thông tin
const char* ssid = "Baka";          // Tên Wi-Fi
const char* password = "19122004pqnl";  // Mật khẩu Wi-Fi

// HiveMQ Cloud thông tin
const char* mqtt_server = "1ac120f392214b618b370212efb4a8bd.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "Boi";
const char* mqtt_password = "19122004";  // Thay mật khẩu thật tại đây

// Chủ đề MQTT
const char* soil_moisture_topic = "esp32/soil_moisture";  // Chủ đề gửi dữ liệu độ ẩm đất
const char* temperature_topic = "esp32/temperature";      // Chủ đề gửi dữ liệu nhiệt độ
const char* sub_topic = "esp32/relay_control";            // Chủ đề nhận lệnh điều khiển relay

WiFiClientSecure espClient;
PubSubClient client(espClient);

// DS18B20 Setup
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Biến trạng thái relay và thời gian gửi
bool isRelayOn = false;       // Trạng thái relay 1 (bật: true, tắt: false)
bool isRelay2On = false;      // Trạng thái relay 2 (bật: true, tắt: false)
unsigned long soilSendInterval = 300000;  // Thời gian mặc định gửi dữ liệu độ ẩm đất (5 phút)
unsigned long soilSendIntervalWhenOn = 1000;  // Gửi dữ liệu độ ẩm đất mỗi giây khi relay bật
unsigned long soilLastSendTime = 0;

unsigned long tempSendInterval = 120000;  // Thời gian gửi nhiệt độ (2 phút)
unsigned long tempLastSendTime = 0;

// Hàm gửi dữ liệu cảm biến độ ẩm đất
void sendSoilMoistureData() {
  int soilValue = analogRead(SOIL_MOISTURE_PIN);
  float soilMoisture = 100 - ((soilValue / 4095.0) * 100);  // Đo độ ẩm đất

  DynamicJsonDocument soilDoc(512);
  soilDoc["soil_moisture"] = soilMoisture;
  char soilMessage[128];
  serializeJson(soilDoc, soilMessage);
  publishMessage(soil_moisture_topic, soilMessage, true);  // Gửi dữ liệu độ ẩm đất qua MQTT

  // In ra giá trị độ ẩm đất
  Serial.print("Độ ẩm đất: ");
  Serial.print(soilMoisture);
  Serial.println("%");
}

// Hàm gửi dữ liệu nhiệt độ
void sendTemperatureData() {
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);  // Đọc nhiệt độ

  DynamicJsonDocument tempDoc(512);
  tempDoc["temperature"] = temperature;
  char tempMessage[128];
  serializeJson(tempDoc, tempMessage);
  publishMessage(temperature_topic, tempMessage, true);  // Gửi dữ liệu nhiệt độ qua MQTT

  // In ra giá trị nhiệt độ
  Serial.print("Nhiệt độ: ");
  Serial.print(temperature);
  Serial.println("°C");
}

// Kết nối Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Đang kết nối với Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nĐã kết nối Wi-Fi!");
  Serial.print("Địa chỉ IP: ");
  Serial.println(WiFi.localIP());
}

// Kết nối lại MQTT nếu mất kết nối
void reconnect() {
  while (!client.connected()) {
    Serial.println("Đang kết nối với MQTT...");
    String clientID = "ESP32Client-";
    clientID += String(random(0xffff), HEX);

    if (client.connect(clientID.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Kết nối MQTT thành công!");
      client.subscribe(sub_topic);  // Đăng ký lắng nghe chủ đề điều khiển relay
      sendSoilMoistureData();  // Gửi dữ liệu ngay khi kết nối MQTT thành công
      sendTemperatureData();   // Gửi nhiệt độ ngay khi kết nối MQTT thành công
    } else {
      Serial.print("Kết nối thất bại, mã lỗi: ");
      Serial.print(client.state());
      Serial.println(" - Thử lại sau 5 giây");
      delay(5000);
    }
  }
}

// Gửi dữ liệu MQTT
void publishMessage(const char* topic, String payload, boolean retained) {
  if (client.publish(topic, payload.c_str(), retained)) {
    Serial.println("Dữ liệu đã gửi [" + String(topic) + "]: " + payload);
  } else {
    Serial.println("Gửi dữ liệu thất bại.");
  }
}

// Callback xử lý tin nhắn MQTT nhận được
void callback(char* topic, byte* payload, unsigned int length) {
  String incomingMessage = "";
  for (unsigned int i = 0; i < length; i++) {
    incomingMessage += (char)payload[i];
  }
  Serial.println("Nhận tin nhắn từ chủ đề [" + String(topic) + "]: " + incomingMessage);

  // Điều khiển relay 1
  if (incomingMessage == "0") { 
    digitalWrite(RELAY1_PIN, LOW); // Tắt relay 1
    Serial.println("Relay 1 tắt");
    isRelayOn = false;
  } 
  else if (incomingMessage == "1") {
    digitalWrite(RELAY1_PIN, HIGH); // Bật relay 1
    Serial.println("Relay 1 bật");
    isRelayOn = true;
  }

  // Điều khiển relay 2
  if (incomingMessage == "2") {
    digitalWrite(RELAY2_PIN, LOW); // Tắt relay 2
    Serial.println("Relay 2 tắt");
    isRelay2On = false;
  } 
  else if (incomingMessage == "3") {
    digitalWrite(RELAY2_PIN, HIGH); // Bật relay 2
    Serial.println("Relay 2 bật");
    isRelay2On = true;
  }

  // Điều khiển cả hai relay cùng lúc
  if (incomingMessage == "8") {
    digitalWrite(RELAY1_PIN, HIGH);  // Bật cả Relay 1
    digitalWrite(RELAY2_PIN, HIGH);  // Bật cả Relay 2
    Serial.println("Cả hai relay đều bật");
    isRelayOn = true;
    isRelay2On = true;
  }
  else if (incomingMessage == "7") {
    digitalWrite(RELAY1_PIN, LOW);  // Tắt cả Relay 1
    digitalWrite(RELAY2_PIN, LOW);  // Tắt cả Relay 2
    Serial.println("Cả hai relay đều tắt");
    isRelayOn = false;
    isRelay2On = false;
  }
  else {
    Serial.println("Lệnh không hợp lệ");
  }
}

// Setup
void setup() {
  Serial.begin(9600);

  // Cấu hình chân relay
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  // Tắt relay mặc định
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);

  setup_wifi();
  espClient.setInsecure();  // Bỏ qua xác minh chứng chỉ
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  sensors.begin();  // Khởi tạo DS18B20
}

// Loop
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();

  // Gửi dữ liệu cảm biến độ ẩm đất
  if (currentMillis - soilLastSendTime >= (isRelayOn ? soilSendIntervalWhenOn : soilSendInterval)) {
    sendSoilMoistureData();  // Gửi dữ liệu độ ẩm đất
    soilLastSendTime = currentMillis;
  }

  // Gửi dữ liệu nhiệt độ mỗi 2 phút
  if (currentMillis - tempLastSendTime >= tempSendInterval) {
    sendTemperatureData();  // Gửi dữ liệu nhiệt độ
    tempLastSendTime = currentMillis;
  }
}
