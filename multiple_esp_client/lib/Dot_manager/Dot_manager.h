#ifndef DOT_MANAGER_H
#define DOT_MANAGER_H

#include <BLEDevice.h>

static BLEAddress *pServerAddress; // The MAC address of the DOT sensor
static bool doConnect = false; // Whether or not to connect to the DOT sensor

// Manages the connection to the DOT sensor
class Dot_manager{
private:
    BLEScan* pBLEScan;

    BLEClient* sensor_client;

    // BLE Services
    BLEUUID configuration_service_uuid;
    BLEUUID measurement_service_uuid;
    BLEUUID battery_service_uuid;

    BLERemoteService* configuration_service;
    BLERemoteService* measurement_service;
    BLERemoteService* battery_service;

    // BLE Characteristics
    BLEUUID battery_characteristic_uuid;
    BLEUUID payload_characteristic_uuid;
    BLEUUID payload_control_characteristic_uuid;
    BLEUUID device_control_characteristic_uuid;

    BLERemoteCharacteristic* battery_characteristic;
    BLERemoteCharacteristic* payload_control_characteristic;
    BLERemoteCharacteristic* device_control_characteristic;

    uint8_t notificationOn[2] = {0x1, 0x0};
    uint8_t notificationOff[2] = {0x0, 0x0};

    std::string start_measurement_val = "000";
    std::string stop_measurement_val = "000";

public:
    BLERemoteCharacteristic* payload_characteristic;

    // Constructor
    Dot_manager(){
        BLEDevice::init("FysioTherapy");
        pBLEScan = BLEDevice::getScan();

        // This starts and stops the measurement, but changing the measurement type does not work
        this->start_measurement_val[0] = (char)1; this->start_measurement_val[1] = (char)1; this->start_measurement_val[2] = (char)16; // [1, 1, 16]
        this->stop_measurement_val[0] = (char)1; this->stop_measurement_val[1] = (char)0; this->stop_measurement_val[2] = (char)16; // [1, 0, 16]
    }

    void connect_sensor(); // Can't be defined here because it uses Ble_advertised_device_callback, which is defined later

    // For some reason, this function does not work
    void set_polling_rate(uint8_t rate_hz){

        std::string device_configuration = device_control_characteristic->readValue();

        // Check if the rate is valid
        if(rate_hz == 1 || rate_hz == 4 || rate_hz == 10 || rate_hz == 12 || rate_hz == 15 || rate_hz == 20 || rate_hz == 30 || rate_hz == 60){
        Serial.print("Old measure rate: ");
        Serial.println(device_configuration[24], HEX);
        device_configuration[24] = (char)rate_hz;
        Serial.print("New measure rate: ");
        Serial.println(device_configuration[24], HEX);
        device_control_characteristic->writeValue(device_configuration); // Write the new rate to the device
        } else{
        Serial.print("Invalid measure rate provided! Staying at ");
        Serial.print(device_configuration[24], HEX);
        Serial.println(" hz.");
        }
    }

    void enable_notify(){
        // Enable notifications for the payload characteristic
        this->payload_characteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)this->notificationOn, 2, true);  
    }

    void disable_notify(){
        // Disable notifications for the payload characteristic
        this->payload_characteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)this->notificationOff, 2, true);
    }

    // Start and stop the measurement
    void start_measurement(){
        payload_control_characteristic->writeValue(this->start_measurement_val);
    }

    void stop_measurement(){
        payload_control_characteristic->writeValue(this->stop_measurement_val);
    }
};

// Callback for finding the DOT sensor
class Ble_advertised_device_callback: public BLEAdvertisedDeviceCallbacks{
    void onResult(BLEAdvertisedDevice advertisedDevice){
        std::string addr = advertisedDevice.getAddress().toString();
        if(addr.substr(0, 8) == std::string("d4:22:cd")){
            advertisedDevice.getScan()->stop();
            pServerAddress = new BLEAddress(advertisedDevice.getAddress());
            doConnect = true;
        }
    }
};

// Connects to the DOT sensor
void Dot_manager::connect_sensor(){
    this->pBLEScan->setAdvertisedDeviceCallbacks(new Ble_advertised_device_callback());
    this->pBLEScan->setActiveScan(true);
    this->pBLEScan->start(30);
    // Wait for the sensor to be found
    while(!doConnect){
        delay(1);
    }
    // Connect to the BLE Server
    this->sensor_client = BLEDevice::createClient();
    this->sensor_client->connect(*pServerAddress);

    // Add UUIDs and verify that the services and characteristics exist
    this->configuration_service_uuid = BLEUUID("15171000-4947-11E9-8646-D663BD873D93"); // Configuration service UUID
    this->configuration_service = this->sensor_client->getService(this->configuration_service_uuid);
    if(this->configuration_service == nullptr){
        Serial.println("Configuration service not found");
        return;
    }
    
    this->measurement_service_uuid = BLEUUID("15172000-4947-11E9-8646-D663BD873D93"); // Measurement service UUID
    this->measurement_service = this->sensor_client->getService(this->measurement_service_uuid);
    if(this->measurement_service == nullptr){
        Serial.println("Measurement service not found");
        return;
    }

    this->battery_service_uuid = BLEUUID("15173000-4947-11E9-8646-D663BD873D93"); // Battery service UUID
    this->battery_service = this->sensor_client->getService(this->battery_service_uuid);
    if(this->battery_service == nullptr){
        Serial.println("Battery service not found");
        return;
    }

    this->battery_characteristic_uuid = BLEUUID("15173001-4947-11E9-8646-D663BD873D93"); // Battery characteristic UUID
    this->battery_characteristic = this->battery_service->getCharacteristic(this->battery_characteristic_uuid);
    if(this->battery_characteristic == nullptr){
        Serial.println("Battery characteristic not found");
        return;
    }

    this->payload_characteristic_uuid = BLEUUID("15172003-4947-11E9-8646-D663BD873D93"); // Payload characteristic UUID
    this->payload_characteristic = this->measurement_service->getCharacteristic(this->payload_characteristic_uuid);
    if(this->payload_characteristic == nullptr){
        Serial.println("Payload characteristic not found");
        return;
    }

    this->payload_control_characteristic_uuid = BLEUUID("15172001-4947-11E9-8646-D663BD873D93"); // Payload control characteristic UUID
    this->payload_control_characteristic = this->measurement_service->getCharacteristic(this->payload_control_characteristic_uuid);
    if(this->payload_control_characteristic == nullptr){
        Serial.println("Payload control characteristic not found");
        return;
    }

    this->device_control_characteristic_uuid = BLEUUID("15171002-4947-11E9-8646-D663BD873D93"); // Device control characteristic UUID
    this->device_control_characteristic = this->configuration_service->getCharacteristic(this->device_control_characteristic_uuid);
    if(this->device_control_characteristic == nullptr){
        Serial.println("Device control characteristic not found");
        return;
    }
}

#endif // DOT_MANAGER_H