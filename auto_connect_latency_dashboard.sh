#!/data/data/com.termux/files/usr/bin/bash

set -e

echo "=== AUTO CONNECT LATENCY DASHBOARD ==="

VOIP="TMessagesProj/jni/voip"

# Backup
cp $VOIP/tgcalls/InstanceImpl.cpp $VOIP/tgcalls/InstanceImpl.cpp.bak
cp $VOIP/webrtc/modules/audio_device/android/aaudio_wrapper.cc $VOIP/webrtc/modules/audio_device/android/aaudio_wrapper.cc.bak
cp $VOIP/CMakeLists.txt $VOIP/CMakeLists.txt.bak

echo "[1] Adding dashboard source to CMake..."

grep -q "latency_dashboard/LatencyDashboard.cpp" $VOIP/CMakeLists.txt || \
sed -i '/voip\/tgcalls\/InstanceImpl.cpp/a\        voip/latency_dashboard/LatencyDashboard.cpp' $VOIP/CMakeLists.txt


echo "[2] Adding includes..."

grep -q "LatencyDashboard.h" $VOIP/tgcalls/InstanceImpl.cpp || \
sed -i '1i #include "../latency_dashboard/LatencyDashboard.h"' \
$VOIP/tgcalls/InstanceImpl.cpp


grep -q "LatencyDashboard.h" $VOIP/webrtc/modules/audio_device/android/aaudio_wrapper.cc || \
sed -i '1i #include "../../../../latency_dashboard/LatencyDashboard.h"' \
$VOIP/webrtc/modules/audio_device/android/aaudio_wrapper.cc


echo "[3] Creating dashboard bridge helper..."

mkdir -p $VOIP/latency_dashboard


cat > $VOIP/latency_dashboard/LatencyBridge.h <<'CPP'
#pragma once
#include "LatencyDashboard.h"

inline std::string GetLatencyDashboardReport()
{
    return LatencyDashboard::Instance().GetReport();
}
CPP


echo "[4] Checking connection..."

grep -R "LatencyDashboard.h" -n $VOIP | head -20

grep -R "LatencyDashboard.cpp" -n $VOIP/CMakeLists.txt


echo "================================"
echo " LATENCY DASHBOARD CONNECTED "
echo "================================"

