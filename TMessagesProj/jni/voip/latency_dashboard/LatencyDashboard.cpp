#include "LatencyDashboard.h"
#include <sstream>

LatencyDashboard& LatencyDashboard::Instance(){
    static LatencyDashboard instance;
    return instance;
}

void LatencyDashboard::Update(const LatencyReport& r){
    std::lock_guard<std::mutex> lock(mutex);

    if (r.audioBufferFrames)
        current.audioBufferFrames = r.audioBufferFrames;

    if (r.framesPerBurst)
        current.framesPerBurst = r.framesPerBurst;

    if (r.xrunCount)
        current.xrunCount = r.xrunCount;

    if (r.networkRTT)
        current.networkRTT = r.networkRTT;

    if (r.jitter)
        current.jitter = r.jitter;

    if (r.packetLoss)
        current.packetLoss = r.packetLoss;

    if (r.ringBufferFill)
        current.ringBufferFill = r.ringBufferFill;

    if (r.lastReceiveTimestamp)
        current.lastReceiveTimestamp = r.lastReceiveTimestamp;

    if (r.incomingPacketCount)
        current.incomingPacketCount = r.incomingPacketCount;

    if (r.incomingPacketDelay)
        current.incomingPacketDelay = r.incomingPacketDelay;

    if (r.opusDecodeTime)
        current.opusDecodeTime = r.opusDecodeTime;

    if (r.audioRenderQueueDelay)
        current.audioRenderQueueDelay = r.audioRenderQueueDelay;

    if (r.audioRenderInputTimestamp)
        current.audioRenderInputTimestamp = r.audioRenderInputTimestamp;

    if (r.audioRenderOutputTimestamp)
        current.audioRenderOutputTimestamp = r.audioRenderOutputTimestamp;

    if (r.audioFramesPerCallback)
        current.audioFramesPerCallback = r.audioFramesPerCallback;

    if (r.audioBurstSize)
        current.audioBurstSize = r.audioBurstSize;

    if (r.audioXrunCount)
        current.audioXrunCount = r.audioXrunCount;

    if (r.jitterBufferCurrentDelay)
        current.jitterBufferCurrentDelay = r.jitterBufferCurrentDelay;

    if (r.jitterBufferTargetDelay)
        current.jitterBufferTargetDelay = r.jitterBufferTargetDelay;

    if (r.jitterBufferPacketsWaiting)

    if (r.audioInputLevel)
        current.audioInputLevel = r.audioInputLevel;
    if (r.audioInputIsSpeech)
        current.audioInputIsSpeech = r.audioInputIsSpeech;
    if (r.audioOutputLevel)
        current.audioOutputLevel = r.audioOutputLevel;
    if (r.packetArrivalTimestamp)
        current.packetArrivalTimestamp = r.packetArrivalTimestamp;
    if (r.speakerPlaybackTimestamp)
        current.speakerPlaybackTimestamp = r.speakerPlaybackTimestamp;
    if (r.endToEndLatency)
        current.endToEndLatency = r.endToEndLatency;
    if (r.aaudioBufferCapacity)
        current.aaudioBufferCapacity = r.aaudioBufferCapacity;
    if (r.aaudioFramesPerDataCallback)
        current.aaudioFramesPerDataCallback = r.aaudioFramesPerDataCallback;
    if (r.aaudioPositionFrames)
        current.aaudioPositionFrames = r.aaudioPositionFrames;
        current.jitterBufferPacketsWaiting = r.jitterBufferPacketsWaiting;
}

std::string LatencyDashboard::GetReport(){
    std::lock_guard<std::mutex> lock(mutex);
    return current.GenerateText();
}

std::string LatencyReport::GenerateText(){

    std::stringstream s;

    s<<"=== Yasuagram Latency Report ===\n";

    s<<"Audio Buffer: "<<audioBufferFrames<<" frames\n";
    s<<"Frames Burst: "<<framesPerBurst<<"\n";
    s<<"XRuns: "<<xrunCount<<"\n";

    s<<"Network RTT: "<<networkRTT<<" ms\n";
    s<<"Jitter: "<<jitter<<" ms\n";
    s<<"Packet Loss: "<<packetLoss<<" %\n";

    s<<"Ring Buffer: "<<ringBufferFill<<"\n";

    s<<"Last Receive Timestamp: "<<lastReceiveTimestamp<<" us\n";
    s<<"Incoming Packets: "<<incomingPacketCount<<"\n";
    s<<"Incoming Packet Delay: "<<incomingPacketDelay<<" ms\n";
    s<<"Opus Decode Time: "<<opusDecodeTime<<" ms\n";
    s<<"Audio Render Queue Delay: "<<audioRenderQueueDelay<<" ms\n";
    s<<"Render Timestamp Difference: "
     <<(audioRenderOutputTimestamp - audioRenderInputTimestamp)/1000.0
     <<" ms\n";
    s<<"Audio Frames Per Callback: "<<audioFramesPerCallback<<"\n";
    s<<"Audio Burst Size: "<<audioBurstSize<<"\n";
    s<<"Audio XRun Count: "<<audioXrunCount<<"\n";
    s<<"Jitter Buffer Current Delay: "<<jitterBufferCurrentDelay<<" ms\n";
    s<<"Jitter Buffer Target Delay: "<<jitterBufferTargetDelay<<" ms\n";
    s<<"Jitter Buffer Packets Waiting: "<<jitterBufferPacketsWaiting<<"\n";

    s<<"Audio Input Level: "<<(int)audioInputLevel<<" (speech: "<<(audioInputIsSpeech ? "yes" : "no")<<")\n";
    s<<"Audio Output Level: "<<(int)audioOutputLevel<<"\n";
    s<<"Packet Arrival Timestamp: "<<packetArrivalTimestamp<<" us\n";
    s<<"Speaker Playback Timestamp: "<<speakerPlaybackTimestamp<<" us\n";
    s<<"End-to-End Latency: "<<endToEndLatency<<" ms\n";
    s<<"AAudio Buffer Capacity: "<<aaudioBufferCapacity<<" frames\n";
    s<<"AAudio Frames Per Callback: "<<aaudioFramesPerDataCallback<<"\n";
    s<<"AAudio Position Frames: "<<aaudioPositionFrames<<"\n";
    return s.str();
}

