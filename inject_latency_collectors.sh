#!/data/data/com.termux/files/usr/bin/bash

set -e

VOIP="TMessagesProj/jni/voip"

echo "=== Injecting latency collectors ==="


# إضافة تحديث AAudio
python3 <<'PY'
from pathlib import Path

p=Path("TMessagesProj/jni/voip/webrtc/modules/audio_device/android/aaudio_wrapper.cc")

s=p.read_text()

if "LatencyDashboard::Instance().Update" not in s:

    old="return AAudioStream_getXRunCount(stream_);"

    new="""int xruns = AAudioStream_getXRunCount(stream_);

LatencyReport report;
report.xrunCount = xruns;
report.audioBufferFrames = AAudioStream_getBufferSizeInFrames(stream_);
report.framesPerBurst = AAudioStream_getFramesPerBurst(stream_);

LatencyDashboard::Instance().Update(report);

return xruns;"""

    if old in s:
        s=s.replace(old,new)
        p.write_text(s)
        print("AAudio collector added")
    else:
        print("AAudio target not found")
else:
    print("AAudio collector already exists")

PY


# إضافة إخراج التقرير من InstanceImpl

python3 <<'PY'
from pathlib import Path

p=Path("TMessagesProj/jni/voip/tgcalls/InstanceImpl.cpp")

s=p.read_text()

if "GetReport()" not in s:

    old="std::string InstanceImpl::getLatencyStats() {"

    if old in s:
        s=s.replace(
        old,
        old+"\n    return LatencyDashboard::Instance().GetReport();"
        )
        p.write_text(s)
        print("Stats output added")
    else:
        print("getLatencyStats not found")
else:
    print("Stats output already exists")

PY


echo "=== DONE ==="
