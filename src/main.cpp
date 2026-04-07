#include "Arduino.h"

void setup() {
    Serial.begin(115200);
    Serial.print("\n\n");
    Serial.println("----------------------------------");
    Serial.printf("ESP32 Chip: %s\n", ESP.getChipModel());
    Serial.printf("Arduino Version: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
    Serial.printf("ESP-IDF Version: %d.%d.%d\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
    Serial.printf("ARDUINO_LOOP_STACK_SIZE %d words (32 bit)\n", CONFIG_ARDUINO_LOOP_STACK_SIZE);
    Serial.println("----------------------------------");
    Serial.print("\n\n");


}

void loop() {
    vTaskDelay(1);
}

