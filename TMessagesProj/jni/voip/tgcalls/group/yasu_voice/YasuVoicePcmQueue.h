#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

struct YasuVoicePcmChunk {
    uint32_t ssrc = 0;
    int sampleRate = 0;
    size_t channels = 0;
    std::vector<int16_t> samples;
};

class YasuVoicePcmQueue {
public:
    static constexpr size_t kMaxChunks = 120;

    void setEnabled(bool enabled);

    bool isEnabled() const;

    bool push(uint32_t ssrc,
              const int16_t *samples,
              size_t sampleCount,
              int sampleRate,
              size_t channels);

    bool pop(YasuVoicePcmChunk &chunk);

    void stop();

private:
    std::atomic<bool> _enabled{false};
    std::mutex _mutex;
    std::condition_variable _condition;
    std::deque<YasuVoicePcmChunk> _queue;
    bool _stopped = false;
};
