#ifndef TGCALLS_YASU_MEASUREMENT_GATE_H
#define TGCALLS_YASU_MEASUREMENT_GATE_H

#include <atomic>

namespace tgcalls {

bool YasuMeasurementsEnabled();
void SetYasuMeasurementsEnabled(bool enabled);

} // namespace tgcalls

#endif // TGCALLS_YASU_MEASUREMENT_GATE_H
