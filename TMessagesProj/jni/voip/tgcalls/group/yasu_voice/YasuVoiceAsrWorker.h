#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class YasuVoicePcmQueue;

class YasuVoiceAsrWorker {
public:
    using ResultCallback = std::function<void(
        uint32_t ssrc,
        const std::string &partialText,
        bool isFinal
    )>;

    YasuVoiceAsrWorker(
        std::shared_ptr<YasuVoicePcmQueue> queue,
        ResultCallback resultCallback
    );

    ~YasuVoiceAsrWorker();

    bool initialize(const std::string &modelDir);

    void start();
    void stop();

    void setEnabled(bool enabled);
    void setMode(int mode);

private:
    void run();

    bool initializeRecognizer(const std::string &modelDir);

    void processChunk(
        uint32_t ssrc,
        const int16_t *samples,
        size_t sampleCount,
        int sampleRate,
        size_t channels
    );

    void resetRecognitionState(bool emitFinal);
    void finishCurrentSegment();
    void decodePartial(bool isFinal);

    bool isSpeech(
        const std::vector<float> &samples
    ) const;

    float calculateRms(
        const std::vector<float> &samples
    ) const;

    static float pcm16ToFloat(int16_t value);

    static void downmixToMono(
        const int16_t *samples,
        size_t sampleCount,
        size_t channels,
        std::vector<float> &out
    );

    static void resampleLinear(
        const std::vector<float> &input,
        int inputRate,
        std::vector<float> &output
    );

private:
    std::shared_ptr<YasuVoicePcmQueue> _queue;
    ResultCallback _resultCallback;

    std::atomic<bool> _running{false};
    std::atomic<bool> _enabled{false};
    std::atomic<int> _mode{0};

    std::thread _thread;

    void *_recognizer = nullptr;

    // Owned only by the ASR worker thread.
    uint32_t _activeSsrc = 0;
    uint32_t _candidateSsrc = 0;

    int _inputSampleRate = 0;

    uint64_t _streamTimeSamples = 0;
    uint64_t _activeLastSpeechSample = 0;
    uint64_t _candidateStartSample = 0;

    float _noiseFloor = 0.003f;

    std::vector<float> _audioBuffer;

    std::string _lastText;

    int _workerMode = 0;

    size_t _samplesSinceDecode = 0;

    std::deque<float> _preRollBuffer;

    static constexpr int kModelSampleRate = 16000;

    // 32 ms at 16 kHz.
    static constexpr size_t kVadFrameSamples = 512;

    // 400 ms pre-roll.
    static constexpr size_t kPreRollSamples = 6400;

    // Partial recognition interval: ~200 ms.
    static constexpr size_t kPartialDecodeSamples = 3200;

    // Maximum continuous recognition segment: 8 seconds.
    static constexpr size_t kMaxSegmentSamples =
        kModelSampleRate * 8;

    // Finalize after ~350 ms of silence.
    static constexpr size_t kSilenceToFinalizeSamples =
        kModelSampleRate * 350 / 1000;

    // Speaker-switch hysteresis: ~220 ms.
    static constexpr size_t kSpeakerSwitchSamples =
        kModelSampleRate * 220 / 1000;

    // Ignore extremely short noise bursts.
    static constexpr size_t kMinSpeechSamples =
        kModelSampleRate * 120 / 1000;

    // Absolute RMS floor.
    static constexpr float kAbsoluteSpeechRms = 0.008f;

    // Speech must be above estimated noise floor.
    static constexpr float kNoiseMultiplier = 2.5f;
};
