#include "YasuVoiceAsrWorker.h"
#include "YasuVoicePcmQueue.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "yasu_voice/sherpa-onnx/c-api/c-api.h"

namespace {

constexpr int kModelSampleRate = 16000;

static float clampFloat(float value) {
    return std::max(-1.0f, std::min(1.0f, value));
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

    config.model_config.num_threads = 2;
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
        return false;
    }

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

    _thread = std::thread([this]() {
        run();
    });
}

void YasuVoiceAsrWorker::stop() {
    if (!_running.exchange(false)) {
        return;
    }

    /*
     * The queue is stopped by GroupInstanceCustomInternal
     * during complete group teardown. This wakes pop().
     */
    if (_thread.joinable()) {
        _thread.join();
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

    _audioBuffer.clear();
    _preRollBuffer.clear();

    _lastText.clear();

    _activeSsrc = 0;
    _candidateSsrc = 0;

    _activeLastSpeechSample = 0;
    _candidateStartSample = 0;

    _samplesSinceDecode = 0;

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

    _audioBuffer.clear();
    _preRollBuffer.clear();
    _lastText.clear();

    _activeSsrc = 0;
    _candidateSsrc = 0;

    _activeLastSpeechSample = 0;
    _candidateStartSample = 0;

    _samplesSinceDecode = 0;
}

void YasuVoiceAsrWorker::decodePartial(
    bool isFinal
) {
    if (!_recognizer ||
        _audioBuffer.empty() ||
        _activeSsrc == 0) {
        return;
    }

    const SherpaOnnxOfflineRecognizer *recognizer =
        static_cast<const SherpaOnnxOfflineRecognizer *>(
            _recognizer
        );

    const SherpaOnnxOfflineStream *stream =
        SherpaOnnxCreateOfflineStream(recognizer);

    if (!stream) {
        return;
    }

    SherpaOnnxAcceptWaveformOffline(
        stream,
        kModelSampleRate,
        _audioBuffer.data(),
        static_cast<int32_t>(_audioBuffer.size())
    );

    SherpaOnnxDecodeOfflineStream(
        recognizer,
        stream
    );

    const SherpaOnnxOfflineRecognizerResult *result =
        SherpaOnnxGetOfflineStreamResult(stream);

    if (result &&
        result->text &&
        result->text[0] != '\0' &&
        _resultCallback) {

        const std::string text(result->text);

        /*
         * Partial results are emitted only when changed.
         * A final result is emitted even if it is identical
         * to the previous partial result, so the UI receives
         * the final state explicitly.
         */
        if (isFinal || text != _lastText) {
            _lastText = text;

            _resultCallback(
                _activeSsrc,
                text,
                isFinal
            );
        }
    }

    if (result) {
        SherpaOnnxDestroyOfflineRecognizerResult(
            result
        );
    }

    SherpaOnnxDestroyOfflineStream(stream);
}

void YasuVoiceAsrWorker::processChunk(
    uint32_t ssrc,
    const int16_t *samples,
    size_t sampleCount,
    int sampleRate,
    size_t channels
) {
    if (!_recognizer ||
        !samples ||
        sampleCount == 0 ||
        sampleRate <= 0 ||
        channels == 0) {
        return;
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
            if (!speech) {
                continue;
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
                    kPartialDecodeSamples) {

                decodePartial(false);

                _samplesSinceDecode = 0;
            }

            const uint64_t silenceSamples =
                _streamTimeSamples -
                _activeLastSpeechSample;

            if (silenceSamples >=
                kSilenceToFinalizeSamples) {

                finishCurrentSegment();
            }

            if (_audioBuffer.size() >=
                kMaxSegmentSamples) {

                decodePartial(true);

                _audioBuffer.clear();
                _preRollBuffer.clear();
                _lastText.clear();

                _samplesSinceDecode = 0;
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

                    finishCurrentSegment();

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
