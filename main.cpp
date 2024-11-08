#include <WiFi.h>
#include <PubSubClient.h>

#define relayPin 2              
#define soil_moisture_pin 34  

// Wi-Fi
const char *ssid = "Baka";
const char *password = "19122004";

// Broker MQTT
const char *mqtt_broker = "mqtt-dashboard.com";
const int mqtt_port = 1883;
const char *pub_topic = "sensor/soil_moisture1";
const char *sub_topic = "relay/control";
const char *client_id = "clientId-e4FfvjPwyg";
const char *mqtt_username = "Boiinlove";
const char *mqtt_password = "19122004";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastSendTime = 0;
const unsigned long interval = 300000;
bool firstSend = true;

// Hàm callback
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
            digitalWrite(relayPin, LOW);  // ON relay
            Serial.println("Relay ON");
        } else if (message == "0") {
            digitalWrite(relayPin, HIGH); // OFF relay
            Serial.println("Relay OFF");
        }
    }
}

// Reconnect
void reconnect() {
    while (!client.connected()) {
        Serial.println("Đang kết nối với MQTT...");
        if (client.connect(client_id, mqtt_username, mqtt_password)) {
            Serial.println("Đã kết nối với MQTT broker!");
            client.subscribe(sub_topic); 
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
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, HIGH); 

    // Connect Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Đang kết nối với Wi-Fi...");
    }
    Serial.println("Đã kết nối với Wi-Fi");

    // Connect MQTT broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    reconnect();
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    unsigned long currentTime = millis();

    if (firstSend || (currentTime - lastSendTime >= interval)) {
        // sensor
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
        firstSend = false;
        lastSendTime = currentTime;
    }
}
