#include <Arduino.h>
#include "Dot_manager.h"

// Hardware serial port 2 is used for communication with the other microcontroller
HardwareSerial mySerial(2);

// Payload data
uint32_t timestamp;
union euler_x{
    uint32_t i;
    float f;
}euler_x;
union euler_y{
    uint32_t i;
    float f;
}euler_y;
union euler_z{
    uint32_t i;
    float f;
}euler_z;
union free_acc_x{
    uint32_t i;
    float f;
}free_acc_x;
union free_acc_y{
    uint32_t i;
    float f;
}free_acc_y;
union free_acc_z{
    uint32_t i;
    float f;
}free_acc_z;

// Callback for when the payload data is received
static void payload_notify_callback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){ 
    digitalWrite(LED_BUILTIN, HIGH);
    // Extract the data from the payload
    timestamp = pData[0] | (pData[1] << 8) | (pData[2] << 16) | (pData[3] << 24);
    euler_x.i = pData[4] | (pData[5] << 8) | (pData[6] << 16) | (pData[7] << 24);
    euler_y.i = pData[8] | (pData[9] << 8) | (pData[10] << 16) | (pData[11] << 24);
    euler_z.i = pData[12] | (pData[13] << 8) | (pData[14] << 16) | (pData[15] << 24);
    free_acc_x.i = pData[16] | (pData[17] << 8) | (pData[18] << 16) | (pData[19] << 24);
    free_acc_y.i = pData[20] | (pData[21] << 8) | (pData[22] << 16) | (pData[23] << 24);
    free_acc_z.i = pData[24] | (pData[25] << 8) | (pData[26] << 16) | (pData[27] << 24);

    // And print the data to the other microcontroller
    mySerial.print(timestamp);
    mySerial.print(",");
    mySerial.print(euler_x.f);
    mySerial.print(",");
    mySerial.print(euler_y.f);
    mySerial.print(",");
    mySerial.print(euler_z.f);
    mySerial.print(",");
    mySerial.print(free_acc_x.f);
    mySerial.print(",");
    mySerial.print(free_acc_y.f);
    mySerial.print(",");
    mySerial.print(free_acc_z.f);
    mySerial.print("\n");
}

// Create a DOT manager object
Dot_manager* dot_manager;

void setup(){
    // Set pin 5 as an input for starting and stopping the measurement
    pinMode(5, INPUT);

    // Set the built-in LED as an output for indicating if a measurement is running
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // LED works with inverted logic, so this turns it off

    Serial.begin(230400);
    mySerial.begin(230400);
    delay(100);

    // Create a DOT manager object
    dot_manager = new Dot_manager();
    // Connect to the DOT sensor
    dot_manager->connect_sensor();
    dot_manager->set_polling_rate(1);
    dot_manager->payload_characteristic->registerForNotify(payload_notify_callback);
    dot_manager->enable_notify();
    
    // Blink LED once to indicate that the sensor is connected
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
}

bool is_measurement_running = false;

void loop(){
    // Check pin 5 for starting and stopping the measurement
    if(digitalRead(5) == HIGH && !is_measurement_running){
        is_measurement_running = true;
        dot_manager->start_measurement();
        // Also indicate that a measurement is running
        digitalWrite(LED_BUILTIN, LOW);
    }
    if(digitalRead(5) == LOW && is_measurement_running){
        is_measurement_running = false;
        dot_manager->stop_measurement();
        // Also indicate that no measurement is running
        digitalWrite(LED_BUILTIN, HIGH);
    }
    delay(100);
}