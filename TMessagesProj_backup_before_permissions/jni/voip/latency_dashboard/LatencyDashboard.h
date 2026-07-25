#pragma once
#include <stdint.h>
#include <string>
#include <mutex>

struct LatencyReport {
    int audioBufferFrames = 0;
    int framesPerBurst = 0;
    int xrunCount = 0;

    double networkRTT = 0;
    double jitter = 0;
    double packetLoss = 0;

    int ringBufferFill = 0;


    // Yasuagram incoming audio latency metrics
    int64_t lastReceiveTimestamp = 0;
    uint64_t incomingPacketCount = 0;
    double incomingPacketDelay = 0;

    // Yasuagram Opus decoder timing
    double opusDecodeTime = 0;

    // Yasuagram Audio Render Queue Delay
    double audioRenderQueueDelay = 0;

    // Real render timestamps
    int64_t audioRenderInputTimestamp = 0;
    int64_t audioRenderOutputTimestamp = 0;

    // Yasuagram AAudio metrics
    uint32_t audioFramesPerCallback = 0;
    uint32_t audioBurstSize = 0;
    uint32_t audioXrunCount = 0;

    // Yasuagram Jitter Buffer metrics
    double jitterBufferCurrentDelay = 0;
    double jitterBufferTargetDelay = 0;
    uint64_t jitterBufferPacketsWaiting = 0;

    // Yasuagram: Audio Level Metrics
    uint8_t audioInputLevel = 0;
    bool audioInputIsSpeech = false;
    uint8_t audioOutputLevel = 0;

    // Yasuagram: End-to-End Latency
    int64_t packetArrivalTimestamp = 0;
    int64_t speakerPlaybackTimestamp = 0;
    double endToEndLatency = 0;

    // Yasuagram: AAudio Device Metrics
    int32_t aaudioBufferCapacity = 0;
    int32_t aaudioFramesPerDataCallback = 0;
    int64_t aaudioPositionFrames = 0;
    std::string GenerateText();
};

class LatencyDashboard {
public:
    static LatencyDashboard& Instance();

    void Update(const LatencyReport& report);
    std::string GetReport();

private:
    LatencyDashboard() {}
    std::mutex mutex;
    LatencyReport current;
};

