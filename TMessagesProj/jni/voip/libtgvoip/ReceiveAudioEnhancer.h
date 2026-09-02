#ifndef TGVOIP_RECEIVE_AUDIO_ENHANCER_H
#define TGVOIP_RECEIVE_AUDIO_ENHANCER_H

#include <cmath>
#include <stdint.h>
#include <stddef.h>

namespace tgvoip {

class ReceiveAudioEnhancer {
public:
    ReceiveAudioEnhancer()
        : envelope(0.0f),
          hpPrevInput(0.0f),
          hpPrevOutput(0.0f),
          limiterEnvelope(0.0f) {
    }

    void Reset() {
        envelope = 0.0f;
        hpPrevInput = 0.0f;
        hpPrevOutput = 0.0f;
        limiterEnvelope = 0.0f;
    }

    void Process(float* samples, size_t count) {
        const float inputGain = 1.58f;
        const float hpCoeff = 0.987f;
        const float compressorThreshold = 0.18f;
        const float compressorRatio = 2.0f;
        const float makeupGain = 1.18f;
        const float limiterThreshold = 0.91f;

        const float attack = 0.003f;
        const float release = 0.045f;

        for (size_t i = 0; i < count; ++i) {
            float x = samples[i] * (1.0f / 32768.0f);

            x *= inputGain;

            float hp = hpCoeff * (hpPrevOutput + x - hpPrevInput);

            hpPrevInput = x;
            hpPrevOutput = hp;
            x = hp;

            float absX = std::fabs(x);

            if (absX > envelope)
                envelope += (absX - envelope) * attack;
            else
                envelope += (absX - envelope) * release;

            float gainReduction = 1.0f;

            if (envelope > compressorThreshold) {
                float compressed =
                    compressorThreshold +
                    (envelope - compressorThreshold) / compressorRatio;

                if (envelope > 0.000001f)
                    gainReduction = compressed / envelope;
            }

            x *= gainReduction;
            x *= makeupGain;

            float absAfter = std::fabs(x);

            if (absAfter > limiterEnvelope)
                limiterEnvelope += (absAfter - limiterEnvelope) * 0.25f;
            else
                limiterEnvelope += (absAfter - limiterEnvelope) * 0.02f;

            if (limiterEnvelope > limiterThreshold) {
                float limitGain = limiterThreshold / limiterEnvelope;
                x *= limitGain;
            }

            if (x > 0.985f)
                x = 0.985f;
            else if (x < -0.985f)
                x = -0.985f;

            samples[i] = x * 32767.0f;
        }
    }

private:
    float envelope;
    float hpPrevInput;
    float hpPrevOutput;
    float limiterEnvelope;
};

}

#endif
