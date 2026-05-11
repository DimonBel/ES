#include "freertos_app/freertos_app.h"
#include "serial_stdio.h"

void setup() {
    SerialStdio::begin(115200);
    freertos_app::setup();
}

void loop() {
    delay(1000);
}
