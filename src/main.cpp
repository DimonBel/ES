#include "freertos_app/freertos_app.h"
#include "freertos_app/state/state.h"
#include "serial_stdio.h"

void setup() {
    SerialStdio::begin(115200);
    freertos_app::setup();
}

void loop() {
    char buffer[16];
    if (SerialStdio::readCommand(buffer, sizeof(buffer))) {
        strncpy(freertos_app::internal::sharedData.serial_command_buffer,
                buffer,
                sizeof(freertos_app::internal::sharedData.serial_command_buffer) - 1);
        freertos_app::internal::sharedData.serial_command_buffer[15] = '\0';
        freertos_app::internal::sharedData.serial_command_received = true;
    }
    delay(50);
}
