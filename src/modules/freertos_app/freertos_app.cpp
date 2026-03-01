#include "freertos_app/freertos_app.h"

#include "freertos_app/init/init.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app {

void setup() {
    internal::setupApplication();
}

void loop() {
    kernel_primitives::delayMs(1000);
}

}
