#!/data/data/com.termux/files/usr/bin/bash

echo "=== Creating latency dashboard integration ==="

mkdir -p TMessagesProj/jni/voip/latency_dashboard

cat > TMessagesProj/jni/voip/latency_dashboard/LatencyDashboard.h <<'CPP'
#pragma once
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
CPP


cat > TMessagesProj/jni/voip/latency_dashboard/LatencyDashboard.cpp <<'CPP'
#include "LatencyDashboard.h"
#include <sstream>

LatencyDashboard& LatencyDashboard::Instance(){
    static LatencyDashboard instance;
    return instance;
}

void LatencyDashboard::Update(const LatencyReport& r){
    std::lock_guard<std::mutex> lock(mutex);
    current=r;
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

    return s.str();
}
CPP


echo "Dashboard core created."

echo "Searching integration points..."

grep -R "getLatencyStats" -n TMessagesProj/jni/voip/tgcalls

grep -R "AAudioStream_getXRunCount" -n TMessagesProj/jni/voip/webrtc/modules/audio_device/android

echo "DONE"
