#include "YasuMeasurementGate.h"

namespace tgcalls {

namespace {
std::atomic<bool> g_yasu_measurements_enabled{false};
}

bool YasuMeasurementsEnabled() {
    return g_yasu_measurements_enabled.load(std::memory_order_relaxed);
}

void SetYasuMeasurementsEnabled(bool enabled) {
    g_yasu_measurements_enabled.store(enabled, std::memory_order_relaxed);
}

} // namespace tgcalls
