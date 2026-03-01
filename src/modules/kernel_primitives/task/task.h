#ifndef KERNEL_PRIMITIVES_TASK_H
#define KERNEL_PRIMITIVES_TASK_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace kernel_primitives {

void delayMs(uint32_t delayMs);
void delayUntilMs(TickType_t *lastWakeTime, uint32_t frequencyMs);
bool createTask(TaskFunction_t task,
                const char *name,
                uint32_t stackSize,
                void *params,
                UBaseType_t priority,
                TaskHandle_t *taskHandle);

}

#endif