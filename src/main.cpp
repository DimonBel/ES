#include "freertos_app/freertos_app.h"
#include "freertos_app/state/state.h"
#include "serial_stdio.h"

void setup() {
    // Инициализируем наш отдельный драйвер
    SerialStdio::begin(115200);

    freertos_app::setup();

    // Initialize button pin with internal pull-up
    pinMode(freertos_app::internal::BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
    char buffer[16];
    if (SerialStdio::readCommand(buffer, sizeof(buffer))) {
        strncpy(freertos_app::internal::sharedData.serial_command_buffer, buffer, sizeof(freertos_app::internal::sharedData.serial_command_buffer) - 1);
        freertos_app::internal::sharedData.serial_command_buffer[15] = '\0'; 
        // Подаем сигнал задаче FreeRTOS, что пришла новая команда
        freertos_app::internal::sharedData.serial_command_received = true;
    }

    // Small delay to prevent CPU overload
    delay(50);
}
