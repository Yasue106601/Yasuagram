#!/data/data/com.termux/files/usr/bin/bash

echo "=== Connecting latency sources ==="

# إضافة include إلى InstanceImpl.cpp
grep -q "LatencyDashboard.h" TMessagesProj/jni/voip/tgcalls/InstanceImpl.cpp || \
sed -i '1i #include "../latency_dashboard/LatencyDashboard.h"' TMessagesProj/jni/voip/tgcalls/InstanceImpl.cpp


# إضافة include إلى aaudio_wrapper.cc
grep -q "LatencyDashboard.h" TMessagesProj/jni/voip/webrtc/modules/audio_device/android/aaudio_wrapper.cc || \
sed -i '1i #include "../../../latency_dashboard/LatencyDashboard.h"' TMessagesProj/jni/voip/webrtc/modules/audio_device/android/aaudio_wrapper.cc


echo "Checking files..."

grep -R "LatencyDashboard.h" -n TMessagesProj/jni/voip | head


echo "=== DONE ==="
