#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoMqttClient.h>
#include <ESP32Servo.h>


WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

//Wifi
const char* ssid = "Piotr";
const char* password = "piotr18862";

//MQTT
const char* broker = "accesscontrol.home";
const int port = 1883;
const char* mqttUser = "lock";
const char* mqttPassword = "admin";

//Servo
Servo servo;

WebServer server(80);

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

void onMQTTMessage(int messageSize) {
    String message = "";
    String topic = mqttClient.messageTopic();

    while (mqttClient.available()) {
        char c = (char)mqttClient.read();
        message += c;
    }

    Serial.print("Received message on topic: ");
    Serial.println(topic);
    Serial.println("Payload: " + message);


    if (topic == "servo/rotate") {
        int angle = message.toInt();
        servo.write(angle);
        Serial.println("Rotating servo");

        //TODO Debug
        // delay(250);
        // digitalWrite(12, HIGH);
        // delay(250);
        // digitalWrite(12, LOW);
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
    mqttClient.subscribe("test/topic");
    mqttClient.subscribe("servo/rotate");
    mqttClient.onMessage(onMQTTMessage);

}
void loopMQTT() {
    if (!mqttClient.connected()) {
        Serial.println("Disconnected from MQTT!");
    }

    mqttClient.poll();
}

void initServo() {
    servo.attach(12);
}


void setup() {
    delay(2000);
    Serial.begin(115200);

    //TODO Debug
    // pinMode(12, OUTPUT);

    initServer();
    initMQTT();
}

void loop() {
    loopServer();
    loopMQTT();
}
