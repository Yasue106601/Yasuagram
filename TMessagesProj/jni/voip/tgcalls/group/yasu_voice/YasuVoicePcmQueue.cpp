#include "YasuVoicePcmQueue.h"

void YasuVoicePcmQueue::setEnabled(bool enabled) {
    _enabled.store(enabled, std::memory_order_relaxed);
}

bool YasuVoicePcmQueue::isEnabled() const {
    return _enabled.load(std::memory_order_relaxed);
}

bool YasuVoicePcmQueue::push(uint32_t ssrc,
                             const int16_t *samples,
                             size_t sampleCount,
                             int sampleRate,
                             size_t channels) {
    /*
     * FAST GATE:
     * Do not copy incoming PCM or touch the mutex when
     * Yasu Voice is disabled.
     */
    if (!_enabled.load(std::memory_order_relaxed)) {
        return false;
    }

    if (!samples || sampleCount == 0 ||
        sampleRate <= 0 || channels == 0) {
        return false;
    }

    YasuVoicePcmChunk chunk;
    chunk.ssrc = ssrc;
    chunk.sampleRate = sampleRate;
    chunk.channels = channels;
    chunk.samples.assign(samples, samples + sampleCount);

    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_stopped) {
            return false;
        }

        if (_queue.size() >= kMaxChunks) {
            _queue.pop_front();
        }

        _queue.emplace_back(std::move(chunk));
    }

    _condition.notify_one();
    return true;
}

bool YasuVoicePcmQueue::pop(YasuVoicePcmChunk &chunk) {
    std::unique_lock<std::mutex> lock(_mutex);

    _condition.wait(lock, [this] {
        return _stopped || !_queue.empty();
    });

    if (_queue.empty()) {
        return false;
    }

    chunk = std::move(_queue.front());
    _queue.pop_front();
    return true;
}

void YasuVoicePcmQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stopped = true;
        _queue.clear();
    }

    _condition.notify_all();
}
