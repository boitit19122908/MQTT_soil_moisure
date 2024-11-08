#include <WiFi.h>
#include <PubSubClient.h>

#define relayPin 2               // Chân điều khiển relay (cập nhật chân relay tùy theo board của bạn)
#define soil_moisture_pin 34     // Chân đo độ ẩm đất

// Thông tin Wi-Fi
const char *ssid = "Baka";
const char *password = "19122004";

// Thông tin Broker MQTT
const char *mqtt_broker = "mqtt-dashboard.com";
const int mqtt_port = 1883;
const char *pub_topic = "sensor/soil_moisture1";  // Topic để gửi độ ẩm đất
const char *sub_topic = "relay/control";          // Topic để điều khiển relay
const char *client_id = "clientId-e4FfvjPwyg";
const char *mqtt_username = "Boiinlove";
const char *mqtt_password = "19122004";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastSendTime = 0;
const unsigned long interval = 300000;
bool firstSend = true;

// Hàm callback xử lý tin nhắn nhận được từ MQTT
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.print("Message received on topic ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(message);
    
    if (String(topic) == sub_topic) {
        if (message == "1") {
            digitalWrite(relayPin, LOW);  // Bật relay
            Serial.println("Relay ON");
        } else if (message == "0") {
            digitalWrite(relayPin, HIGH); // Tắt relay
            Serial.println("Relay OFF");
        }
    }
}

// Hàm kết nối lại với MQTT nếu bị mất kết nối
void reconnect() {
    while (!client.connected()) {
        Serial.println("Đang kết nối với MQTT...");
        if (client.connect(client_id, mqtt_username, mqtt_password)) {
            Serial.println("Đã kết nối với MQTT broker!");
            client.subscribe(sub_topic); // Đăng ký nhận topic điều khiển relay
        } else {
            Serial.print("Kết nối thất bại, mã lỗi: ");
            Serial.print(client.state());
            Serial.println(" - thử lại sau 5 giây");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(9600);
    pinMode(relayPin, OUTPUT);      // Thiết lập relayPin là OUTPUT
    digitalWrite(relayPin, HIGH);   // Đặt relay tắt lúc khởi động

    // Kết nối Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Đang kết nối với Wi-Fi...");
    }
    Serial.println("Đã kết nối với Wi-Fi");

    // Kết nối đến MQTT broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);   // Đặt hàm callback cho client MQTT
    reconnect();
}

void loop() {
    // Kiểm tra kết nối MQTT, nếu mất kết nối sẽ thử lại
    if (!client.connected()) {
        reconnect();
    }
    client.loop(); // Giữ cho client MQTT hoạt động

    unsigned long currentTime = millis();
    
    // Gửi lần đầu ngay lập tức hoặc sau mỗi 3 giây
    if (firstSend || (currentTime - lastSendTime >= interval)) {
        // Đọc giá trị độ ẩm đất và gửi lên MQTT
        int sensor_value = analogRead(soil_moisture_pin);
        float moisture_percent = 100 - ((sensor_value / 4095.00) * 100);

        Serial.print("Soil Moisture: ");
        Serial.print(moisture_percent);
        Serial.println("%");

        String msg = String(moisture_percent);
        if (client.publish(pub_topic, msg.c_str())) {
            Serial.println("Gửi dữ liệu thành công");
        } else {
            Serial.println("Gửi dữ liệu thất bại");
        }

        // Đánh dấu đã gửi lần đầu và cập nhật thời gian lần gửi cuối
        firstSend = false;
        lastSendTime = currentTime;
    }
}
