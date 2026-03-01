#ifndef KERNEL_PRIMITIVES_MUTEX_H
#define KERNEL_PRIMITIVES_MUTEX_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace kernel_primitives {

class Mutex {
public:
    bool init();
    bool take(uint32_t timeoutMs) const;
    bool give() const;
    SemaphoreHandle_t native() const;

private:
    SemaphoreHandle_t handle_ = nullptr;
};

}

#endif