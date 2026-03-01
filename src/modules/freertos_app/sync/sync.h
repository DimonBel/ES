#ifndef FREERTOS_APP_SYNC_H
#define FREERTOS_APP_SYNC_H

namespace freertos_app::internal {

bool initSyncPrimitives();
void updateLCD(const char *line1, const char *line2);

}

#endif