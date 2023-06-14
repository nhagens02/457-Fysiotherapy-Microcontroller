#include "Wifi_manager.h"
#include "Mqtt_manager.h"

Wifi_manager wifi_manager;
Mqtt_manager *mqtt_manager;

HardwareSerial mySerial(2);
String serial_buffer = "";

// MQTT callback function
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    // Store the payload in a string
    String payload_string = "";
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        payload_string += (char)payload[i];
    }

    // If the topic is dot/measurementstatus, start or stop the measurement based on the payload being either 'start' or 'stop'
    if(strcmp(topic, "dot/measurementstatus") == 0){
        if(strcmp(payload_string.c_str(), "start") == 0){
            // Start the measurement by setting pin 5 high
            Serial.println("Starting measurement");
            digitalWrite(5, HIGH);
        }
        else if(strcmp(payload_string.c_str(), "stop") == 0){
            // Stop the measurement by setting pin 5 low
            Serial.println("Stopping measurement");
            digitalWrite(5, LOW);
        }
    }
}

void setup() {
    // Set pin 5 as an output pin
    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);

    // Start the serial ports and the wifi manager
    wifi_manager.begin();
    Serial.begin(230400);
    mySerial.begin(230400);
    delay(100);

    // Connect to wifi and the mqtt broker
    if(!wifi_manager.connect_wifi()){Serial.println("Could not connect to wifi, starting webserver"); WiFi.mode(WIFI_AP); wifi_manager.run_webserver();}
    Serial.println("WiFi connected!");

    String broker_address = "<ENTER BROKER ADDRESS HERE>";
    uint16_t broker_port = 8883;
    String username = "<ENTER USERNAME HERE>";
    String password = "<ENTER PASSWORD HERE>";
    mqtt_manager = new Mqtt_manager(&wifi_manager, broker_address, broker_port, username, password);
    if(mqtt_manager->connect_to_broker() == 0){
        Serial.println("Connected to broker!");
    }
    else{
        Serial.println("Could not connect to broker!");
    }

    // Set the mqtt callback function and subscribe to the dot/measurementstatus topic for receiving start and stop commands
    mqtt_manager->mqtt_client.setCallback(mqtt_callback);
    bool subscribed = mqtt_manager->subscribe("dot/measurementstatus");
    if(subscribed){
        Serial.println("Subscribed to dot/measurementstatus");
    }
    else{
        Serial.println("Could not subscribe to dot/measurementstatus");
    }
}

void loop() {
    // Run the mqtt client loop so that the mqtt client can receive messages
    mqtt_manager->mqtt_client.loop();
    
    // If there is data available on the serial port, read it until a newline character is found and publish it to the dot/sensordata topic
    if(mySerial.available()){
        serial_buffer = mySerial.readStringUntil('\n');
        mqtt_manager->publish("dot/sensordata", serial_buffer);
    }
}
