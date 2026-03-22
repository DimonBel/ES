#include "freertos_app/freertos_app.h"
#include "freertos_app/state/state.h"

void setup() {
    freertos_app::setup();

    // Initialize button pin with internal pull-up
    pinMode(freertos_app::internal::BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
    // Handle serial commands
    if (Serial.available() > 0) {
        char c = Serial.read();

        // Echo character
        Serial.write(c);

        // Check for newline or carriage return (command complete)
        if (c == '\n' || c == '\r') {
            if (freertos_app::internal::sharedData.serial_command_index > 0) {
                // Null-terminate the command
                freertos_app::internal::sharedData.serial_command_buffer[freertos_app::internal::sharedData.serial_command_index] = '\0';

                // Signal that a command is received
                freertos_app::internal::sharedData.serial_command_received = true;

                Serial.println();  // New line after command
            }

            // Reset command buffer
            freertos_app::internal::sharedData.serial_command_index = 0;
        }
        // Ignore spaces and tabs
        else if (c != ' ' && c != '\t') {
            // Convert to lowercase
            if (c >= 'A' && c <= 'Z') {
                c = c + 32;
            }

            // Add to command buffer if there's space
            if (freertos_app::internal::sharedData.serial_command_index < sizeof(freertos_app::internal::sharedData.serial_command_buffer) - 1) {
                freertos_app::internal::sharedData.serial_command_buffer[freertos_app::internal::sharedData.serial_command_index++] = c;
            }
        }
    }

    // Small delay to prevent CPU overload
    delay(10);
}
