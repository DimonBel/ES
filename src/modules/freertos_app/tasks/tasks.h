#ifndef FREERTOS_APP_TASKS_H
#define FREERTOS_APP_TASKS_H

namespace freertos_app::internal {

void vTaskFSM(void *pvParameters);
void vTaskDisplay(void *pvParameters);
bool createApplicationTasks();

}

#endif
