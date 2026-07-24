#!/data/data/com.termux/files/usr/bin/bash

set -e

VOIP="TMessagesProj/jni/voip"

echo "=== FINAL LATENCY DASHBOARD LINK ==="


python3 <<'PY'
from pathlib import Path

p=Path("TMessagesProj/jni/voip/tgcalls/MediaManager.cpp")

s=p.read_text()

if "networkRTT" not in s:

    old="""float sendBitrateKbps = ((float)stats.send_bandwidth_bps / 1000.0f);"""

    new="""float sendBitrateKbps = ((float)stats.send_bandwidth_bps / 1000.0f);


/* Yasuagram latency dashboard network collector */

LatencyReport report;

report.networkRTT = stats.rtt_ms;

report.packetLoss =
stats.packets_lost / 
(float)(stats.packets_received + stats.packets_lost + 1) * 100.0;

LatencyDashboard::Instance().Update(report);

"""

    if old in s:
        s=s.replace(old,new)
        p.write_text(s)
        print("Network collector connected")
    else:
        print("MediaManager target missing")

else:
    print("Network collector already exists")

PY



python3 <<'PY'
from pathlib import Path

p=Path("TMessagesProj/jni/voip/tgcalls/InstanceImpl.cpp")

s=p.read_text()

old="""std::string InstanceImpl::getLatencyStats() {
    return LatencyDashboard::Instance().GetReport();
}"""

new="""std::string InstanceImpl::getLatencyStats() {

    return LatencyDashboard::Instance().GetReport();

}"""


if old not in s:
    print("Latency function already patched")
else:
    s=s.replace(old,new)
    p.write_text(s)
    print("Latency output ready")

PY



echo "Checking..."

grep -R "LatencyDashboard::Instance().Update" -n $VOIP

grep -R "getLatencyStats" -n $VOIP/tgcalls


echo "================================"
echo " DASHBOARD READY FOR UI BRIDGE "
echo "================================"

