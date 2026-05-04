#ifndef FREERTOS_APP_TASKS_H
#define FREERTOS_APP_TASKS_H

namespace freertos_app::internal {

void vTaskAcquisition(void *pvParameters);
void vTaskPIDControl(void *pvParameters);
void vTaskDisplay(void *pvParameters);
bool createApplicationTasks();

}

#endif