#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoMqttClient.h>
#include <ESP32Servo.h>

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

//Identity
const char* DEVICE_TYPE = "Lock";
const char* DEVICE_NAME = "Smart Door Lock";
String DEVICE_ID;

//Wifi
// const char* ssid = "Piotr";
// const char* password = "piotr18862";
const char* ssid = "13A";
const char* password = "InforDBWabbes3DPL";

//MQTT
const char* broker = "accesscontrol.home";
const int port = 1883;
const char* mqttUser = "lock";
const char* mqttPassword = "admin";

//Servo
Servo servo;

//Batery
int BAT = 35;
unsigned long last_battery_check;

WebServer server(80);

//Identity
String getChipId() {
    uint64_t chip_id = ESP.getEfuseMac();
    return String(chip_id, HEX);
}
String getIdentityJson() {
    return  "{ \"id\": \""+DEVICE_ID+"\", \"name\": \""+DEVICE_NAME+"\", \"type\": \""+DEVICE_TYPE+"\" }";
}
void initIdentity() {
    DEVICE_ID = getChipId();
    Serial.print("Device ID: ");
    Serial.println(DEVICE_ID);
    Serial.println(getIdentityJson());
}


//Server
void handleRoot() {
    server.send(200, "text/plain", "ESP32-S3 is running!");
}
void initServer() {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting to the wifi....");
        delay(500);
    }

    server.on("/", handleRoot);
    server.begin();

    Serial.print("Connected to ");
    Serial.println(ssid);

    Serial.println("HTTP server started");
}
void loopServer() {
    server.handleClient();
}

//MQTT
void publishMQTT(String topic, String payload) {
    Serial.print("Publishing to ");
    Serial.print(topic);
    Serial.print(" : ");
    Serial.println(payload);

    mqttClient.beginMessage(topic);
    mqttClient.print(payload);
    mqttClient.endMessage();
}
void onMQTTMessage(int messageSize) {
    String message = "";
    String topic = mqttClient.messageTopic();

    while (mqttClient.available()) {
        char c = (char)mqttClient.read();
        message += c;
    }

    if (topic == "lock/open") {
        int angle = message.toInt();

        servo.write(angle);
        Serial.println("Rotating servo");

        delay(1500);
        Serial.println("Reversing servo");
        servo.write(0);


        //TODO Debug
        // delay(250);
        // digitalWrite(12, HIGH);
        // delay(angle * 10);
        // digitalWrite(12, LOW);
    }
    else if (topic == "lock/get/battery") {
        int ADC = analogRead(BAT);
        last_battery_check = millis();

        Serial.print("Battery voltage: ");
        Serial.println(ADC);

        publishMQTT("debug/debug", String(ADC));
    }
    else if (topic == "lock/get/rssi") {
        int rssi = WiFi.RSSI();

        Serial.print("RSSI: ");
        Serial.println(rssi);

        publishMQTT("debug/debug", String(rssi));
    }
    else if (topic == "presence") {
        publishMQTT("presence/confirm", getIdentityJson());
    }

}
void initMQTT() {
    mqttClient.setUsernamePassword(mqttUser, mqttPassword);


    Serial.println("Connecting to MQTT...");

    if (!mqttClient.connect(broker, port)) {
        Serial.println("Failed to connect to MQTT broker. Error: ");
        Serial.println(mqttClient.connectError());
        return;
    }

    Serial.println("Connected to MQTT broker!");
    mqttClient.subscribe("lock/open");
    mqttClient.subscribe("lock/get/battery");
    mqttClient.subscribe("lock/get/rssi");
    mqttClient.subscribe("presence");
    mqttClient.onMessage(onMQTTMessage);

}
void loopMQTT() {
    if (!mqttClient.connected()) {
        Serial.println("Disconnected from MQTT!");
    }

    mqttClient.poll();
}

//Servo
void initServo() {
    servo.attach(16);
}

//Battery
void loopBattery() {
    int voltage = analogRead(BAT);

    if (!last_battery_check || millis() - last_battery_check > 1000) {
        last_battery_check = millis();

        Serial.print("Battery voltage: ");
        Serial.println(voltage);

        // mqttClient.beginMessage("debug/debug");
        // mqttClient.print(voltage);
        // mqttClient.endMessage();
    }
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    initIdentity();
    initServo();
    initServer();
    initMQTT();
}

void loop() {
    loopServer();
    loopMQTT();
    // loopBattery();
}
