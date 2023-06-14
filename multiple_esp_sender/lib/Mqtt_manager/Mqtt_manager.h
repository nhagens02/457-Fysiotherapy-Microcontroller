#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>
#include "Wifi_manager.h"

// Class for managing the MQTT connection
class Mqtt_manager{
private:
    Wifi_manager* wifi_manager;

    String& broker_address;
    uint16_t& broker_port;

    String& username;
    String& password;

public:
    // This is public so that you can set the callback function
    PubSubClient mqtt_client;

    // Constructor
    Mqtt_manager(Wifi_manager* wifi_manager, String& broker_address, uint16_t broker_port, String& username, String& password) : 
        wifi_manager(wifi_manager), broker_address(broker_address), broker_port(broker_port), username(username), password(password), mqtt_client(wifi_manager->secure_client){
            this->mqtt_client.setServer(this->broker_address.c_str(), this->broker_port);
        }

    // Connect to the MQTT broker
    int connect_to_broker(){
        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);
        this->mqtt_client.connect(clientId.c_str(), this->username.c_str(), this->password.c_str());
        delay(500);
        return this->mqtt_client.state();
    }

    // Publish a message to a topic
    void publish(String topic, String payload){
        this->mqtt_client.publish(topic.c_str(), payload.c_str());
    }

    // Subscribe to a topic
    bool subscribe(String topic){
        return this->mqtt_client.subscribe(topic.c_str());
    }
};

#endif // MQTT_MANAGER_H