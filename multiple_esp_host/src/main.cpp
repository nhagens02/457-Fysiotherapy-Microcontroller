// Libraries
#include <Arduino.h>
#include "Dot_manager.h"

HardwareSerial mySerial(2);
String serial_buffer;

// Bools for keeping track of the state of the measurement
bool new_payload = false;
bool new_serial = false;

// Variables for storing the payload data. Unions are used because the data comes in as bytes, combined into 32-bit integers, but we want to interpret them as floats.
uint32_t timestamp_host;
union euler_x_host{
    uint32_t i;
    float f;
}euler_x_host;
union euler_y_host{
    uint32_t i;
    float f;
}euler_y_host;
union euler_z_host{
    uint32_t i;
    float f;
}euler_z_host;
union free_acc_x_host{
    uint32_t i;
    float f;
}free_acc_x_host;
union free_acc_y_host{
    uint32_t i;
    float f;
}free_acc_y_host;
union free_acc_z_host{
    uint32_t i;
    float f;
}free_acc_z_host;

// Callback for when a new measurement is received
static void payload_notify_callback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
    timestamp_host = pData[0] | (pData[1] << 8) | (pData[2] << 16) | (pData[3] << 24);
    euler_x_host.i = pData[4] | (pData[5] << 8) | (pData[6] << 16) | (pData[7] << 24);
    euler_y_host.i = pData[8] | (pData[9] << 8) | (pData[10] << 16) | (pData[11] << 24);
    euler_z_host.i = pData[12] | (pData[13] << 8) | (pData[14] << 16) | (pData[15] << 24);
    free_acc_x_host.i = pData[16] | (pData[17] << 8) | (pData[18] << 16) | (pData[19] << 24);
    free_acc_y_host.i = pData[20] | (pData[21] << 8) | (pData[22] << 16) | (pData[23] << 24);
    free_acc_z_host.i = pData[24] | (pData[25] << 8) | (pData[26] << 16) | (pData[27] << 24);
    new_payload = true;
}

void setup() {
    // Set pin 5 as an input for starting and stopping the measurement
    pinMode(5, INPUT);

    // Set the built-in LED as an output for indicating if a measurement is running
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // LED works with inverted logic, so this turns it off

    // Start serial communication
    Serial.begin(230400);
    mySerial.begin(230400);
    delay(100);

    Dot_manager dot_manager;
    // Connect to DOT sensor
    dot_manager.connect_sensor();
    dot_manager.set_polling_rate(1);
    dot_manager.payload_characteristic->registerForNotify(payload_notify_callback);
    dot_manager.enable_notify();

    // Blink LED once to indicate that the sensor is connected
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);

    bool is_measurement_running = false;
    volatile bool _true = true;
    while(_true){
        // Check for serial data
        if(mySerial.available()){
            serial_buffer = mySerial.readStringUntil('\n');
            new_serial = true;
        }

        // When there's new serial data and a new measurement, send it over serial
        if(new_serial && new_payload){
            digitalWrite(LED_BUILTIN, HIGH);
            mySerial.print(serial_buffer);
            mySerial.print(",");
            mySerial.print(timestamp_host);
            mySerial.print(",");
            mySerial.print(euler_x_host.f);
            mySerial.print(",");
            mySerial.print(euler_y_host.f);
            mySerial.print(",");
            mySerial.print(euler_z_host.f);
            mySerial.print(",");
            mySerial.print(free_acc_x_host.f);
            mySerial.print(",");
            mySerial.print(free_acc_y_host.f);
            mySerial.print(",");
            mySerial.print(free_acc_z_host.f);
            mySerial.print("\n");
            // And reset the flags
            new_serial = false;
            new_payload = false;
        }

        // Check pin 5 for starting and stopping the measurement
        if(digitalRead(5) == HIGH && !is_measurement_running){
            is_measurement_running = true;
            dot_manager.start_measurement();
            // Also indicate that a measurement is running
            digitalWrite(LED_BUILTIN, LOW);
        }
        if(digitalRead(5) == LOW && is_measurement_running){
            is_measurement_running = false;
            dot_manager.stop_measurement();
            // Also indicate that no measurement is running
            digitalWrite(LED_BUILTIN, HIGH);
        }
    }
}

void loop() {
    // This should never be reached. Some bugs occur if I try to use the loop function, so I just loop in the setup function instead.
    Serial.println("Loop reached! This should never happen!");
    exit(1);
}
