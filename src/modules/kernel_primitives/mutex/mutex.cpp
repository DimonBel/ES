#include "kernel_primitives/mutex/mutex.h"

namespace kernel_primitives {

bool Mutex::init() {
    if (handle_ != nullptr) {
        return true;
    }

    handle_ = xSemaphoreCreateMutex();
    return handle_ != nullptr;
}

bool Mutex::take(uint32_t timeoutMs) const {
    if (handle_ == nullptr) {
        return false;
    }

    return xSemaphoreTake(handle_, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

bool Mutex::give() const {
    if (handle_ == nullptr) {
        return false;
    }

    return xSemaphoreGive(handle_) == pdTRUE;
}

SemaphoreHandle_t Mutex::native() const {
    return handle_;
}

}
