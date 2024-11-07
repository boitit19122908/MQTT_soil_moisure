#include <WiFi.h>
#include <PubSubClient.h>

#define soil_moisture_pin 34 

// Thông tin Wi-Fi
const char *ssid = "Baka";       
const char *password = "19122004"; 

// Thông tin Broker MQTT
const char *mqtt_broker = "mqtt-dashboard.com"; 
const int mqtt_port = 1883;                   
const char *topic = "sensor/soil_moisture1"; 
const char *client_id = "clientId-Etqxqq9UOy";
const char *mqtt_username = "Boiinlove";
const char *mqtt_password = "19122004";

WiFiClient espClient;               
PubSubClient client(espClient);      

unsigned long lastSendTime = 0; // Thời gian lần gửi cuối
const unsigned long interval = 300000; // 5 phút (300,000 milliseconds)
bool firstSend = true; // Đánh dấu lần gửi đầu tiên

void reconnect() {
    while (!client.connected()) {
        Serial.println("Đang kết nối với MQTT...");
        if (client.connect(client_id, mqtt_username, mqtt_password)) {
            Serial.println("Đã kết nối với MQTT broker!");
            client.subscribe(topic); // Đăng ký nhận topic
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

    // Kết nối Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Đang kết nối với Wi-Fi...");
    }
    Serial.println("Đã kết nối với Wi-Fi");

    // Kết nối đến MQTT broker
    client.setServer(mqtt_broker, mqtt_port);
    reconnect();
}

void loop() {
    // Kiểm tra kết nối MQTT, nếu mất kết nối sẽ thử lại
    if (!client.connected()) {
        reconnect();
    }
    client.loop(); // Giữ cho client MQTT hoạt động

    unsigned long currentTime = millis();
    
    // Gửi lần đầu ngay lập tức hoặc sau mỗi 5 phút
    if (firstSend || (currentTime - lastSendTime >= interval)) {
        // Đọc giá trị độ ẩm đất và gửi lên MQTT
        int sensor_value = analogRead(soil_moisture_pin);
        float moisture_percent = 100 - ((sensor_value / 4095.00) * 100);

        Serial.print("Soil Moisture: ");
        Serial.print(moisture_percent);
        Serial.println("%");

        String msg = String(moisture_percent);
        if (client.publish(topic, msg.c_str())) {
            Serial.println("Gửi dữ liệu thành công");
        } else {
            Serial.println("Gửi dữ liệu thất bại");
        }

        // Đánh dấu đã gửi lần đầu và cập nhật thời gian lần gửi cuối
        firstSend = false;
        lastSendTime = currentTime;
    }
}
