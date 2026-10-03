#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

#include "api/task_queue/task_queue_base.h"

namespace tgcalls {

class YasuFastAudioCore final {
public:
    static YasuFastAudioCore& Instance();

    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    void Reset();
    void ResetMeasurements();

    void PushPacket(uint32_t ssrc,
                    uint16_t sequence,
                    uint32_t timestamp,
                    const uint8_t* payload,
                    size_t payload_size);

    void ProcessPacket(uint32_t ssrc,
                        uint16_t sequence,
                        uint32_t timestamp,
                        const uint8_t* payload,
                        size_t payload_size,
                        uint64_t packet_generation,
                        uint64_t packet_arrival_us);

    int ReadPcm(int16_t* output,
                int frames,
                int channels,
                int sample_rate);

    std::string TakeMeasurementLog();

private:
    YasuFastAudioCore();
    ~YasuFastAudioCore();

    YasuFastAudioCore(const YasuFastAudioCore&) = delete;
    YasuFastAudioCore& operator=(const YasuFastAudioCore&) = delete;

    struct Impl;
    Impl* impl_;
};

}  // namespace tgcalls
