#include "kernel_primitives/semaphore/binary_semaphore.h"

namespace kernel_primitives {

bool BinarySemaphore::init() {
    if (handle_ != nullptr) {
        return true;
    }

    handle_ = xSemaphoreCreateBinary();
    return handle_ != nullptr;
}

bool BinarySemaphore::take(uint32_t timeoutMs) const {
    if (handle_ == nullptr) {
        return false;
    }

    return xSemaphoreTake(handle_, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

bool BinarySemaphore::give() const {
    if (handle_ == nullptr) {
        return false;
    }

    return xSemaphoreGive(handle_) == pdTRUE;
}

SemaphoreHandle_t BinarySemaphore::native() const {
    return handle_;
}

}
