#include "rtc_base/logging.h"
#include "YasuVoiceAsrWorker.h"
#include "YasuVoicePcmQueue.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <chrono>

#include "tgcalls/group/yasu_voice/sherpa-onnx/c-api/c-api.h"

namespace {

constexpr int kModelSampleRate = 16000;

constexpr size_t kYasuFastPartialDecodeSamples =
    kModelSampleRate * 50 / 1000;

constexpr size_t kYasuNormalPartialDecodeSamples =
    kModelSampleRate * 80 / 1000;

constexpr size_t kYasuSlowPartialDecodeSamples =
    kModelSampleRate * 140 / 1000;

static float clampFloat(float value) {
    return std::max(-1.0f, std::min(1.0f, value));
}

static size_t yasuWordCount(const std::string &text) {
    size_t count = 0;
    bool inWord = false;

    for (unsigned char c : text) {
        const bool space =
            c == ' ' ||
            c == '\n' ||
            c == '\t' ||
            c == '\r';

        if (space) {
            inWord = false;
        } else if (!inWord) {
            inWord = true;
            ++count;
        }
    }

    return count;
}

static std::string yasuStableWordPrefix(
    const std::string &previous,
    const std::string &current
) {
    const size_t limit =
        std::min(previous.size(), current.size());

    size_t i = 0;

    while (i < limit &&
           previous[i] == current[i]) {
        ++i;
    }

    /*
     * Never cut a UTF-8 character.
     */
    while (i > 0 &&
           i < current.size() &&
           (static_cast<unsigned char>(current[i]) & 0xC0) == 0x80) {
        --i;
    }

    /*
     * Only keep complete words. The last word may still
     * be changing while Moonshine is decoding a partial.
     */
    if (i == current.size()) {
        const size_t lastSpace =
            current.rfind(' ');

        if (lastSpace == std::string::npos) {
            return "";
        }

        return current.substr(
            0,
            lastSpace
        );
    }

    const size_t boundary =
        current.rfind(' ', i);

    if (boundary == std::string::npos) {
        return "";
    }

    return current.substr(
        0,
        boundary
    );
}


} // namespace

YasuVoiceAsrWorker::YasuVoiceAsrWorker(
    std::shared_ptr<YasuVoicePcmQueue> queue,
    ResultCallback resultCallback
) :
    _queue(std::move(queue)),
    _resultCallback(std::move(resultCallback)) {
}

YasuVoiceAsrWorker::~YasuVoiceAsrWorker() {
    stop();

    if (_recognizer) {
        SherpaOnnxDestroyOfflineRecognizer(
            static_cast<SherpaOnnxOfflineRecognizer *>(_recognizer)
        );

        _recognizer = nullptr;
    }
}

bool YasuVoiceAsrWorker::initialize(
    const std::string &modelDir
) {
    if (_recognizer) {
        return true;
    }

    return initializeRecognizer(modelDir);
}

bool YasuVoiceAsrWorker::initializeRecognizer(
    const std::string &modelDir
) {
    SherpaOnnxOfflineRecognizerConfig config;
    std::memset(&config, 0, sizeof(config));

    config.feat_config.sample_rate = kModelSampleRate;
    config.feat_config.feature_dim = 80;

    const std::string tokensPath =
        modelDir + "/tokens.txt";

    const std::string encoderPath =
        modelDir + "/encoder_model.ort";

    const std::string decoderPath =
        modelDir + "/decoder_model_merged.ort";

    config.model_config.tokens =
        tokensPath.c_str();

    config.model_config.num_threads = 3;
    config.model_config.debug = 0;
    config.model_config.provider = "cpu";

    config.model_config.moonshine.encoder =
        encoderPath.c_str();

    config.model_config.moonshine.merged_decoder =
        decoderPath.c_str();

    config.decoding_method = "greedy_search";

    const SherpaOnnxOfflineRecognizer *recognizer =
        SherpaOnnxCreateOfflineRecognizer(&config);

    if (!recognizer) {
        RTC_LOG(LS_ERROR)
            << "[YASU VOICE] Sherpa recognizer creation FAILED"
            << " modelDir=" << modelDir;
        return false;
    }

    RTC_LOG(LS_INFO)
        << "[YASU VOICE] Sherpa recognizer initialized"
        << " modelDir=" << modelDir;

    _recognizer =
        const_cast<SherpaOnnxOfflineRecognizer *>(recognizer);

    return true;
}

void YasuVoiceAsrWorker::start() {
    bool expected = false;

    if (!_running.compare_exchange_strong(
            expected,
            true)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(
            _partialMutex
        );

        _partialStop = false;
        _partialPending = false;
        _partialInFlight = false;
        _partialIsFinal = false;

        _partialAudio.clear();
        _partialSsrc = 0;
        _partialGeneration = 0;
        _partialRequestId = 0;
        _partialCompletedRequestId = 0;

        _partialLastGeneration = 0;
        _partialLastText.clear();
    }

    // YASU VOICE: the PCM worker is always required for Chirp 3.
    // The legacy Sherpa partial decoder must not consume CPU when
    // no legacy recognizer is initialized.
    if (_recognizer) {
        _partialDecodeThread = std::thread([this]() {
            runPartialDecoder();
        });
    }

    _thread = std::thread([this]() {
        run();
    });
}

void YasuVoiceAsrWorker::stop() {
    _running.store(
        false,
        std::memory_order_relaxed
    );

    {
        std::lock_guard<std::mutex> lock(
            _partialMutex
        );

        _partialStop = true;
    }

    _partialCondition.notify_all();
    _partialDoneCondition.notify_all();

    if (_thread.joinable()) {
        _thread.join();
    }

    if (_partialDecodeThread.joinable()) {
        _partialDecodeThread.join();
    }
}

void YasuVoiceAsrWorker::setEnabled(bool enabled) {
    /*
     * Publish the state atomically.
     * The queue uses the same state as a fast gate so
     * disabled audio is rejected before PCM is copied.
     */
    _enabled.store(enabled, std::memory_order_relaxed);

    if (_queue) {
        _queue->setEnabled(enabled);
    }
}

void YasuVoiceAsrWorker::setMode(int mode) {
    if (mode < 0 || mode > 1) {
        mode = 0;
    }

    _mode.store(mode);
}

float YasuVoiceAsrWorker::pcm16ToFloat(
    int16_t value
) {
    return static_cast<float>(value) / 32768.0f;
}

void YasuVoiceAsrWorker::downmixToMono(
    const int16_t *samples,
    size_t sampleCount,
    size_t channels,
    std::vector<float> &out
) {
    out.clear();

    if (!samples ||
        sampleCount == 0 ||
        channels == 0) {
        return;
    }

    const size_t frames =
        sampleCount / channels;

    out.reserve(frames);

    for (size_t i = 0; i < frames; ++i) {
        int32_t sum = 0;

        for (size_t c = 0; c < channels; ++c) {
            sum += samples[i * channels + c];
        }

        const float value =
            static_cast<float>(sum) /
            (32768.0f * static_cast<float>(channels));

        out.push_back(clampFloat(value));
    }
}

void YasuVoiceAsrWorker::resampleLinear(
    const std::vector<float> &input,
    int inputRate,
    std::vector<float> &output
) {
    output.clear();

    if (input.empty() ||
        inputRate <= 0) {
        return;
    }

    if (inputRate == kModelSampleRate) {
        output = input;
        return;
    }

    const double ratio =
        static_cast<double>(kModelSampleRate) /
        static_cast<double>(inputRate);

    const size_t outputSize =
        static_cast<size_t>(
            static_cast<double>(input.size()) * ratio
        );

    if (outputSize == 0) {
        return;
    }

    output.resize(outputSize);

    for (size_t i = 0; i < outputSize; ++i) {
        const double source =
            static_cast<double>(i) / ratio;

        const size_t index =
            static_cast<size_t>(source);

        const double fraction =
            source - static_cast<double>(index);

        if (index + 1 < input.size()) {
            output[i] =
                static_cast<float>(
                    input[index] * (1.0 - fraction) +
                    input[index + 1] * fraction
                );
        } else {
            output[i] = input.back();
        }
    }
}

float YasuVoiceAsrWorker::calculateRms(
    const std::vector<float> &samples
) const {
    if (samples.empty()) {
        return 0.0f;
    }

    double sum = 0.0;

    for (float sample : samples) {
        sum +=
            static_cast<double>(sample) *
            static_cast<double>(sample);
    }

    return static_cast<float>(
        std::sqrt(
            sum /
            static_cast<double>(samples.size())
        )
    );
}

bool YasuVoiceAsrWorker::isSpeech(
    const std::vector<float> &samples
) const {
    if (samples.empty()) {
        return false;
    }

    const float rms =
        calculateRms(samples);

    const float dynamicThreshold =
        std::max(
            kAbsoluteSpeechRms,
            _noiseFloor * kNoiseMultiplier
        );

    return rms >= dynamicThreshold;
}

void YasuVoiceAsrWorker::resetRecognitionState(
    bool emitFinal
) {
    if (emitFinal &&
        !_audioBuffer.empty() &&
        _audioBuffer.size() >= kMinSpeechSamples) {
        decodePartial(true);
    }

    _segmentGeneration.fetch_add(
        1,
        std::memory_order_acq_rel
    );

    _audioBuffer.clear();
    _preRollBuffer.clear();

    _lastText.clear();

    _asrPreviousText.clear();
    _asrStablePrefix.clear();
    _asrStableRepeats = 0;

    _activeSsrc = 0;
    _candidateSsrc = 0;

    _activeLastSpeechSample = 0;
    _candidateStartSample = 0;

    _samplesSinceDecode = 0;

    /*
     * Reset adaptive partial timing for the next speech segment.
     * The new segment must not inherit the speech-rate estimate
     * from the previous segment.
     */
    _adaptivePartialDecodeSamples =
        kYasuNormalPartialDecodeSamples;
    _lastDecodedAudioSamples = 0;
    _lastDecodedTextBytes = 0;

    /*
     * Do NOT reset _streamTimeSamples here.
     * It is the worker's continuous timeline.
     */
    _inputSampleRate = 0;
}

void YasuVoiceAsrWorker::finishCurrentSegment() {
    if (_activeSsrc == 0) {
        return;
    }

    if (_audioBuffer.size() >= kMinSpeechSamples) {
        decodePartial(true);
    }

    _segmentGeneration.fetch_add(
        1,
        std::memory_order_acq_rel
    );

    _audioBuffer.clear();
    _preRollBuffer.clear();
    _lastText.clear();

    _asrPreviousText.clear();
    _asrStablePrefix.clear();
    _asrStableRepeats = 0;

    _activeSsrc = 0;
    _candidateSsrc = 0;

    _activeLastSpeechSample = 0;
    _candidateStartSample = 0;

    _samplesSinceDecode = 0;

    /*
     * Reset adaptive partial timing for the next speech segment.
     */
    _adaptivePartialDecodeSamples =
        kYasuNormalPartialDecodeSamples;
    _lastDecodedAudioSamples = 0;
    _lastDecodedTextBytes = 0;
}

void YasuVoiceAsrWorker::decodePartial(
    bool isFinal
) {
    if (!_recognizer ||
        _audioBuffer.empty() ||
        _activeSsrc == 0) {
        return;
    }

    const uint64_t generation =
        _segmentGeneration.load(
            std::memory_order_acquire
        );

    std::unique_lock<std::mutex> lock(
        _partialMutex
    );

    if (_partialStop) {
        return;
    }

    /*
     * A final request has priority over partial requests.
     */
    if (!isFinal && _partialIsFinal) {
        return;
    }

    const uint64_t requestId =
        ++_partialRequestId;

    /*
     * Copy only the current audio snapshot.
     * Moonshine inference happens on the dedicated
     * decoder thread, never on the PCM/audio worker.
     */
    _partialAudio = _audioBuffer;
    _partialSsrc = _activeSsrc;
    _partialGeneration = generation;
    _partialIsFinal = isFinal;
    _partialPending = true;

    _partialCondition.notify_one();

    if (isFinal) {
        /*
         * Final output must be completed before the
         * current recognition segment is destroyed.
         */
        _partialDoneCondition.wait(
            lock,
            [this, requestId]() {
                return _partialCompletedRequestId >= requestId ||
                       _partialStop;
            }
        );
    }
}

void YasuVoiceAsrWorker::runPartialDecoder() {
    while (true) {
        std::vector<float> audio;
        uint32_t ssrc = 0;
        uint64_t generation = 0;
        uint64_t requestId = 0;
        bool isFinal = false;

        {
            std::unique_lock<std::mutex> lock(
                _partialMutex
            );

            _partialCondition.wait(
                lock,
                [this]() {
                    return _partialStop ||
                           _partialPending;
                }
            );

            if (_partialStop &&
                !_partialPending) {
                break;
            }

            audio = std::move(_partialAudio);
            _partialAudio.clear();

            ssrc = _partialSsrc;
            generation = _partialGeneration;
            requestId = _partialRequestId;
            isFinal = _partialIsFinal;

            _partialPending = false;
            _partialInFlight = true;

            /*
             * Important:
             * clear the final flag after taking the request.
             * Otherwise every later partial request would be
             * rejected after the first final decode.
             */
            _partialIsFinal = false;
        }

        decodeSnapshot(
            audio,
            ssrc,
            generation,
            isFinal,
            requestId
        );

        {
            std::lock_guard<std::mutex> lock(
                _partialMutex
            );

            _partialInFlight = false;

            if (requestId >
                _partialCompletedRequestId) {
                _partialCompletedRequestId =
                    requestId;
            }
        }

        _partialDoneCondition.notify_all();
    }
}

void YasuVoiceAsrWorker::decodeSnapshot(
    const std::vector<float> &audio,
    uint32_t ssrc,
    uint64_t generation,
    bool isFinal,
    uint64_t requestId
) {
    if (!_recognizer ||
        audio.empty() ||
        ssrc == 0) {
        return;
    }

    /*
     * Do not waste CPU decoding an already-invalid segment.
     */
    if (generation !=
            _segmentGeneration.load(
                std::memory_order_acquire
            ) ||
        !_enabled.load(
            std::memory_order_relaxed
        )) {
        return;
    }

    const SherpaOnnxOfflineRecognizer *recognizer =
        static_cast<
            const SherpaOnnxOfflineRecognizer *
        >(_recognizer);

    const SherpaOnnxOfflineStream *stream =
        SherpaOnnxCreateOfflineStream(
            recognizer
        );

    if (!stream) {
        return;
    }

    const auto yasuDecodeStart =
        std::chrono::steady_clock::now();

    SherpaOnnxAcceptWaveformOffline(
        stream,
        kModelSampleRate,
        audio.data(),
        static_cast<int32_t>(
            audio.size()
        )
    );

    SherpaOnnxDecodeOfflineStream(
        recognizer,
        stream
    );

    const auto yasuDecodeEnd =
        std::chrono::steady_clock::now();

    const int64_t yasuDecodeMs =
        std::chrono::duration_cast<
            std::chrono::milliseconds
        >(
            yasuDecodeEnd -
            yasuDecodeStart
        ).count();

    RTC_LOG(LS_INFO)
        << "[YASU VOICE] decode"
        << " audio_ms="
        << (audio.size() * 1000 /
            kModelSampleRate)
        << " decode_ms="
        << yasuDecodeMs
        << " final="
        << isFinal
        << " async=1"
        << " request="
        << requestId;

    const SherpaOnnxOfflineRecognizerResult *result =
        SherpaOnnxGetOfflineStreamResult(
            stream
        );

    if (result &&
        result->text &&
        result->text[0] != '\0' &&
        _resultCallback) {

        const std::string text(
            result->text
        );

        /*
         * Adaptive partial timing.
         *
         * Moonshine is offline and receives the complete current
         * segment. We therefore use the growth of its hypothesis
         * as a speech-rate signal:
         *
         *   large text growth -> fast speech -> decode sooner
         *   small/no growth    -> slow speech -> allow more context
         *
         * This changes recognition responsiveness only; it never
         * changes the ASR engine or its decoding method.
         */
        if (!isFinal) {
            const size_t audioSamples =
                audio.size();

            const size_t textWords =
                yasuWordCount(text);

            if (_lastDecodedAudioSamples > 0 &&
                audioSamples > _lastDecodedAudioSamples) {

                const size_t audioDelta =
                    audioSamples -
                    _lastDecodedAudioSamples;

                const size_t previousWords =
                    yasuWordCount(_asrPreviousText);

                const size_t wordDelta =
                    textWords >= previousWords
                        ? textWords - previousWords
                        : 0;

                /*
                 * Measure speech growth by words rather than UTF-8
                 * bytes. This is especially important for Arabic,
                 * where one character may occupy several bytes.
                 */
                const float wordsPerSecond =
                    static_cast<float>(wordDelta) *
                    static_cast<float>(kModelSampleRate) /
                    static_cast<float>(audioDelta);

                if (wordDelta >= 2 ||
                    wordsPerSecond >= 3.5f) {

                    _adaptivePartialDecodeSamples =
                        kYasuFastPartialDecodeSamples;

                } else if (wordDelta == 0 ||
                           wordsPerSecond <= 0.8f) {

                    _adaptivePartialDecodeSamples =
                        kYasuSlowPartialDecodeSamples;

                } else {

                    _adaptivePartialDecodeSamples =
                        kYasuNormalPartialDecodeSamples;
                }
            }

            _lastDecodedAudioSamples =
                audioSamples;

            _lastDecodedTextBytes =
                text.size();
        }

        /*
         * Build a conservative stable prefix from consecutive
         * Moonshine partials. This does not replace the raw
         * transcript yet; it only gives the intelligence layer
         * a safe state to work from.
         */
        if (generation != _partialLastGeneration) {
            _asrPreviousText.clear();
            _asrStablePrefix.clear();
            _asrStableRepeats = 0;

            _partialLastGeneration = generation;
            _partialLastText.clear();
        }

        if (text == _asrPreviousText) {
            /*
             * Moonshine produced exactly the same hypothesis
             * twice. The complete text is now a stable word
             * sequence, including a single-word result.
             */
            if (!_asrStablePrefix.empty()) {
                ++_asrStableRepeats;
            } else if (!text.empty()) {
                _asrStablePrefix = text;
                _asrStableRepeats = 1;
            }
        } else {
            const std::string candidate =
                yasuStableWordPrefix(
                    _asrPreviousText,
                    text
                );

            if (!candidate.empty()) {
                if (_asrStablePrefix.empty()) {
                    /*
                     * First confirmed stable word sequence.
                     */
                    _asrStablePrefix = candidate;
                    _asrStableRepeats = 1;

                } else if (candidate == _asrStablePrefix) {
                    /*
                     * Existing stable prefix survived another
                     * Moonshine revision.
                     */
                    ++_asrStableRepeats;

                } else if (
                    candidate.size() > _asrStablePrefix.size() &&
                    candidate.compare(
                        0,
                        _asrStablePrefix.size(),
                        _asrStablePrefix
                    ) == 0 &&
                    candidate[_asrStablePrefix.size()] == ' ') {

                    /*
                     * Stable prefix grew forward.
                     * It is never allowed to move backwards.
                     */
                    _asrStablePrefix = candidate;
                    _asrStableRepeats = 1;
                }
                /*
                 * Conflicting candidates are ignored here.
                 * Moonshine may revise its unstable tail freely.
                 */
            }

            _asrPreviousText = text;
        }

        /*
         * Inference can finish after the segment has
         * already changed, so validate the generation
         * again before sending anything to the UI.
         */
        const bool generationStillValid =
            generation ==
                _segmentGeneration.load(
                    std::memory_order_acquire
                ) &&
            _enabled.load(
                std::memory_order_relaxed
            );

        if (generationStillValid) {

            RTC_LOG(LS_INFO)
                << "[YASU ASR INTEL]"
                << " request=" << requestId
                << " final=" << isFinal
                << " repeats=" << _asrStableRepeats
                << " stable_chars=" << _asrStablePrefix.size()
                << " text_chars=" << text.size();

            if (generation !=
                _partialLastGeneration) {

                _partialLastGeneration =
                    generation;

                _partialLastText.clear();
            }

            /*
             * FAIL-CLOSED ASR OUTPUT:
             *
             * Moonshine is an offline recognizer and may produce
             * plausible-looking text from weak/ambiguous audio.
             * Never forward an unstable partial hypothesis.
             *
             * A partial result must survive at least two identical
             * recognition states before it reaches the UI.
             * Final results are allowed through because they represent
             * the completed speech segment.
             *
             * IMPORTANT:
             * The recognized text itself is never rewritten,
             * corrected, translated, or invented here.
             */
            const bool partialStableEnough =
                isFinal ||
                _asrStableRepeats >= 2;

            if (partialStableEnough &&
                (isFinal || text != _partialLastText)) {

                _partialLastText = text;

                _resultCallback(
                    ssrc,
                    text,
                    isFinal
                );
            } else {
                RTC_LOG(LS_INFO)
                    << "[YASU ASR DROP UNSTABLE]"
                    << " request=" << requestId
                    << " final=" << isFinal
                    << " repeats=" << _asrStableRepeats
                    << " text_chars=" << text.size();
            }
        }

        static std::atomic<bool>
            loggedFirstResult{false};

        bool expectedFirstResult = false;

        if (loggedFirstResult.compare_exchange_strong(
                expectedFirstResult,
                true)) {

            RTC_LOG(LS_INFO)
                << "[YASU VOICE] First ASR result: "
                << text;
        }
    }

    if (result) {
        SherpaOnnxDestroyOfflineRecognizerResult(
            result
        );
    }

    SherpaOnnxDestroyOfflineStream(
        stream
    );
}


void YasuVoiceAsrWorker::flushChirpPcm() {
        _chirpPcmBuffer.empty()) {
        return;
    }

        _chirpPcmBuffer.data(),
        _chirpPcmBuffer.size()
    );

    _chirpPcmBuffer.clear();
}

void YasuVoiceAsrWorker::processChunk(
    uint32_t ssrc,
    const int16_t *samples,
    size_t sampleCount,
    int sampleRate,
    size_t channels
) {
    // Chirp 3 does not require the legacy Sherpa recognizer.
    if (!samples ||
        sampleCount == 0 ||
        sampleRate <= 0 ||
        channels == 0) {
        return;
    }

    static std::atomic<bool> loggedFirstPcm{false};
    bool expectedFirstPcm = false;
    if (loggedFirstPcm.compare_exchange_strong(
            expectedFirstPcm, true)) {
        RTC_LOG(LS_INFO)
            << "[YASU VOICE] First PCM reached processChunk"
            << " ssrc=" << ssrc
            << " samples=" << sampleCount
            << " rate=" << sampleRate
            << " channels=" << channels;
    }

    const int currentMode =
        _mode.load();

    if (currentMode != _workerMode) {
        _workerMode = currentMode;

        resetRecognitionState(true);
    }

    std::vector<float> mono;

    downmixToMono(
        samples,
        sampleCount,
        channels,
        mono
    );

    if (mono.empty()) {
        return;
    }

    std::vector<float> audio16k;

    resampleLinear(
        mono,
        sampleRate,
        audio16k
    );

    if (audio16k.empty()) {
        return;
    }

    // YASU ASR: conservative adaptive gain normalization.
    // Keeps quiet speech intelligible without aggressively amplifying noise.
    {
        float sumSquares = 0.0f;
        float peak = 0.0f;

        for (float sample : audio16k) {
            const float a = std::fabs(sample);
            sumSquares += sample * sample;
            peak = std::max(peak, a);
        }

        const float rms = std::sqrt(
            sumSquares / static_cast<float>(audio16k.size())
        );

        if (rms > 0.003f && peak > 0.01f) {
            const float targetRms = 0.10f;
            float gain = targetRms / rms;
            gain = std::max(0.75f, std::min(2.5f, gain));

            for (float &sample : audio16k) {
                sample = std::max(
                    -1.0f,
                    std::min(1.0f, sample * gain)
                );
            }
        }
    }

    // Chirp 3 receives normalized 16 kHz mono PCM.

    _inputSampleRate = sampleRate;

    /*
     * Reuse the same frame allocation for all 32 ms VAD
     * windows inside this incoming PCM chunk.
     */
    std::vector<float> frame;
    frame.reserve(kVadFrameSamples);

    size_t offset = 0;

    while (offset < audio16k.size()) {
        const size_t remaining =
            audio16k.size() - offset;

        const size_t frameSize =
            std::min(
                kVadFrameSamples,
                remaining
            );

        frame.assign(
            audio16k.begin() + offset,
            audio16k.begin() + offset + frameSize
        );

        offset += frameSize;

        const float rms =
            calculateRms(frame);

        const bool speech =
            isSpeech(frame);

        if (!speech) {
            constexpr float kNoiseAdaptation = 0.015f;

            _noiseFloor =
                _noiseFloor *
                    (1.0f - kNoiseAdaptation) +
                rms *
                    kNoiseAdaptation;

            _noiseFloor =
                std::max(
                    0.0005f,
                    std::min(
                        0.08f,
                        _noiseFloor
                    )
                );
        }

        /*
         * Timeline advances once per actual 32 ms frame,
         * rather than once per complete incoming chunk.
         */
        _streamTimeSamples += frameSize;

        if (_activeSsrc == 0) {
            /*
             * Keep recent audio even while no speaker is active.
             * This gives Moonshine the beginning of short, fast,
             * quiet, or slightly clipped words and numbers.
             */
            for (float sample : frame) {
                _preRollBuffer.push_back(sample);

                while (_preRollBuffer.size() > kPreRollSamples) {
                    _preRollBuffer.pop_front();
                }
            }

            if (!speech) {
                continue;
            }

            _activeSsrc = ssrc;

            _activeLastSpeechSample =
                _streamTimeSamples;

            _candidateSsrc = 0;
            _candidateStartSample = 0;

            _audioBuffer.clear();
            _lastText.clear();
            _samplesSinceDecode = 0;

            /*
             * Start with the recent pre-roll instead of only the
             * first detected speech frame.
             */
            _audioBuffer.insert(
                _audioBuffer.end(),
                _preRollBuffer.begin(),
                _preRollBuffer.end()
            );

            continue;
        }

        if (ssrc == _activeSsrc) {
            if (speech) {
                _activeLastSpeechSample =
                    _streamTimeSamples;
            }

            /*
             * Keep a bounded recent buffer. It is intentionally
             * separate from playback and never blocks the audio
             * callback.
             */
            for (float sample : frame) {
                _preRollBuffer.push_back(sample);

                while (
                    _preRollBuffer.size() >
                    kPreRollSamples
                ) {
                    _preRollBuffer.pop_front();
                }
            }

            _audioBuffer.insert(
                _audioBuffer.end(),
                frame.begin(),
                frame.end()
            );

            _samplesSinceDecode += frame.size();

            if (_audioBuffer.size() >=
                    kMinSpeechSamples &&
                _samplesSinceDecode >=
                    _adaptivePartialDecodeSamples) {

                if (_recognizer) {
                    decodePartial(false);
                }

                _samplesSinceDecode = 0;
            }

            const uint64_t silenceSamples =
                _streamTimeSamples -
                _activeLastSpeechSample;

            if (silenceSamples >=
                kSilenceToFinalizeSamples) {

                if (_recognizer) {
                    finishCurrentSegment();
                }
            }

            if (_audioBuffer.size() >=
                kMaxSegmentSamples) {

                /*
                 * Finalize the current long segment, but keep the
                 * same active speaker. This prevents long spoken
                 * numbers/sentences from losing speaker continuity.
                 */
                if (_recognizer) {
                    decodePartial(true);
                }

                _segmentGeneration.fetch_add(
                    1,
                    std::memory_order_acq_rel
                );

                _audioBuffer.clear();
                _lastText.clear();

                _asrPreviousText.clear();
                _asrStablePrefix.clear();
                _asrStableRepeats = 0;

                _samplesSinceDecode = 0;

                /*
                 * _activeSsrc intentionally remains unchanged.
                 * The next frames belong to the same speaker and
                 * form a fresh Moonshine segment.
                 */
            }

            continue;
        }

        /*
         * Another participant is speaking.
         * Require a short hysteresis period before switching
         * the single ASR worker to that participant.
         */
        if (speech) {
            if (_candidateSsrc != ssrc) {
                _candidateSsrc = ssrc;

                _candidateStartSample =
                    _streamTimeSamples;
            } else {
                const uint64_t candidateDuration =
                    _streamTimeSamples -
                    _candidateStartSample;

                const uint64_t activeSilence =
                    _streamTimeSamples -
                    _activeLastSpeechSample;

                if (activeSilence >=
                        kSilenceToFinalizeSamples ||
                    candidateDuration >=
                        kSpeakerSwitchSamples) {

                    if (_recognizer) {
                    finishCurrentSegment();
                }

                    _activeSsrc = ssrc;

                    _activeLastSpeechSample =
                        _streamTimeSamples;

                    _candidateSsrc = 0;
                    _candidateStartSample = 0;

                    _audioBuffer.clear();
                    _preRollBuffer.clear();
                    _lastText.clear();
                    _samplesSinceDecode = 0;

                    _audioBuffer.insert(
                        _audioBuffer.end(),
                        frame.begin(),
                        frame.end()
                    );
                }
            }
        }
    }
}

void YasuVoiceAsrWorker::run() {
    while (_running.load()) {
        if (!_queue) {
            break;
        }

        YasuVoicePcmChunk chunk;

        if (!_queue->pop(chunk)) {
            break;
        }

        /*
         * Even when disabled, continue draining the queue so
         * stale incoming participant audio cannot accumulate.
         */
        if (!_enabled.load()) {
            if (_activeSsrc != 0 ||
                !_audioBuffer.empty() ||
                !_lastText.empty()) {

                resetRecognitionState(false);
            }

            continue;
        }

        processChunk(
            chunk.ssrc,
            chunk.samples.data(),
            chunk.samples.size(),
            chunk.sampleRate,
            chunk.channels
        );
    }

    if (_activeSsrc != 0 &&
        !_audioBuffer.empty()) {
        finishCurrentSegment();
    }
}
