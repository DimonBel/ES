#include "kernel_primitives/task/task.h"

namespace kernel_primitives {

void delayMs(uint32_t delayMsValue) {
    vTaskDelay(pdMS_TO_TICKS(delayMsValue));
}

void delayUntilMs(TickType_t *lastWakeTime, uint32_t frequencyMs) {
    vTaskDelayUntil(lastWakeTime, pdMS_TO_TICKS(frequencyMs));
}

bool createTask(TaskFunction_t task,
                const char *name,
                uint32_t stackSize,
                void *params,
                UBaseType_t priority,
                TaskHandle_t *taskHandle) {
    return xTaskCreate(task, name, stackSize, params, priority, taskHandle) == pdPASS;
}

}
