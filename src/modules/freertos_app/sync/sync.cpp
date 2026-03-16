#include "freertos_app/sync/sync.h"

#include "freertos_app/state/state.h"
#include "kernel_primitives/task/task.h"

namespace freertos_app::internal {

bool initSyncPrimitives() {
    bool mutexOk = lcdMutex.init();
    bool semSoundDisplayOk = semSoundDisplay.init();
    bool semSoundLedOk = semSoundLED.init();
    bool semTempDisplayOk = semTempDisplay.init();

    return mutexOk && semSoundDisplayOk && semSoundLedOk && semTempDisplayOk;
}

void updateLCD(const char *line1, const char *line2) {
    if (lcd == nullptr) return;

    if (lcdMutex.take(100)) {
        // Don't clear - just overwrite to reduce flickering
        lcd->setCursor(0, 0);
        lcd->print(line1);
        lcd->setCursor(0, 1);
        lcd->print(line2);
        lcdMutex.give();
    }
}

}
