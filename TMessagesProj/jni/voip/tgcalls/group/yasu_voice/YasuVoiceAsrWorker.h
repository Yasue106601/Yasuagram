#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
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

    void runPartialDecoder();

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

    // Queues a snapshot for the dedicated ASR decoder thread.
    // Partial requests never block audio capture.
    // Final requests wait until their decode has completed.
    void decodePartial(bool isFinal);

    void decodeSnapshot(
        const std::vector<float> &audio,
        uint32_t ssrc,
        uint64_t generation,
        bool isFinal,
        uint64_t requestId
    );

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

    void flushChirpPcm();

private:
    std::shared_ptr<YasuVoicePcmQueue> _queue;
    ResultCallback _resultCallback;
    std::atomic<bool> _running{false};
    std::atomic<bool> _enabled{false};
    std::atomic<int> _mode{0};

    std::thread _thread;

    // Dedicated Moonshine partial/final decoder thread.
    // The capture/PCM worker must never block on offline ASR.
    std::thread _partialDecodeThread;
    std::mutex _partialMutex;
    std::condition_variable _partialCondition;
    std::condition_variable _partialDoneCondition;

    bool _partialStop = false;
    bool _partialPending = false;
    bool _partialInFlight = false;
    bool _partialIsFinal = false;

    std::vector<float> _partialAudio;
    uint32_t _partialSsrc = 0;
    uint64_t _partialGeneration = 0;
    uint64_t _partialRequestId = 0;
    uint64_t _partialCompletedRequestId = 0;

    // Owned only by the partial decoder thread.
    uint64_t _partialLastGeneration = 0;
    std::string _partialLastText;

    // ASR intelligence state: tracks repeated partials without
    // freezing Moonshine's raw transcript prematurely.
    std::string _asrPreviousText;
    std::string _asrStablePrefix;
    uint32_t _asrStableRepeats = 0;

    void *_recognizer = nullptr;

    // Owned only by the ASR worker thread.
    uint32_t _activeSsrc = 0;
    uint32_t _candidateSsrc = 0;

    // Invalidates late ASR results from old segments.
    std::atomic<uint64_t> _segmentGeneration{0};

    int _inputSampleRate = 0;

    uint64_t _streamTimeSamples = 0;
    uint64_t _activeLastSpeechSample = 0;
    uint64_t _candidateStartSample = 0;

    float _noiseFloor = 0.003f;

    std::vector<float> _audioBuffer;

    // Independent 16 kHz mono PCM buffer for the Chirp 3
    // streaming path. 80 ms = 1280 samples.
    std::vector<int16_t> _chirpPcmBuffer;

    std::string _lastText;

    int _workerMode = 0;

    size_t _samplesSinceDecode = 0;

    /*
     * Adaptive Moonshine partial interval.
     * It changes according to how quickly the recognized
     * hypothesis grows, instead of using one fixed interval.
     */
    size_t _adaptivePartialDecodeSamples = 1920;
    size_t _lastDecodedAudioSamples = 0;
    size_t _lastDecodedTextBytes = 0;

    std::deque<float> _preRollBuffer;

    static constexpr int kModelSampleRate = 16000;

    // 32 ms at 16 kHz.
    static constexpr size_t kVadFrameSamples = 512;

    // 600 ms pre-roll.
    static constexpr size_t kPreRollSamples = 9600;

    // Partial recognition interval: ~120 ms.
    static constexpr size_t kPartialDecodeSamples = 1920;

    // Send Chirp PCM in ~80 ms batches to reduce JNI/gRPC call overhead.
    static constexpr size_t kChirpPcmBatchSamples = 1280;

    // Maximum continuous recognition segment: 20 seconds.
    static constexpr size_t kMaxSegmentSamples =
        kModelSampleRate * 20;

    // Finalize after ~550 ms of silence.
    static constexpr size_t kSilenceToFinalizeSamples =
        kModelSampleRate * 550 / 1000;

    // Speaker-switch hysteresis: ~250 ms.
    static constexpr size_t kSpeakerSwitchSamples =
        kModelSampleRate * 250 / 1000;

    // Ignore only extremely short noise bursts.
    static constexpr size_t kMinSpeechSamples =
        kModelSampleRate * 50 / 1000;

    // Absolute RMS floor.
    static constexpr float kAbsoluteSpeechRms = 0.008f;

    // Speech must be above estimated noise floor.
    static constexpr float kNoiseMultiplier = 2.5f;
};
