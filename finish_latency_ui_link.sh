#!/data/data/com.termux/files/usr/bin/bash

set -e

echo "=== Connecting Latency Dashboard UI ==="


python3 <<'PY'
from pathlib import Path


# =========================
# 1) VoIPService.java
# =========================

p=Path("TMessagesProj/src/main/java/org/telegram/messenger/voip/VoIPService.java")

s=p.read_text()

if "public String getLatencyStats()" not in s:

    pos=s.rfind("\n}")

    code=r'''

    public String getLatencyStats() {

        try {

            if (tgVoip != null && tgVoip.length > 0 && tgVoip[0] != null) {

                return tgVoip[0].getLatencyStats();

            }

        } catch (Exception e) {

            return "Latency error: " + e.getMessage();

        }

        return "No active VoIP instance";
    }

'''

    s=s[:pos]+code+s[pos:]

    p.write_text(s)

    print("VoIPService linked")

else:
    print("VoIPService already linked")



# =========================
# 2) GroupCallActivity.java
# =========================

p=Path("TMessagesProj/src/main/java/org/telegram/ui/GroupCallActivity.java")

s=p.read_text()


old="""if (tgVoip != null && tgVoip.length > 0 && tgVoip[CAPTURE_DEVICE_CAMERA] != null) {
                 report = tgVoip[CAPTURE_DEVICE_CAMERA].getLatencyStats();
             }"""


new="""VoIPService service = VoIPService.getSharedInstance();

            if (service != null) {
                report = service.getLatencyStats();
            }"""


if old in s:

    s=s.replace(old,new)

    p.write_text(s)

    print("GroupCallActivity fixed")

elif "service.getLatencyStats()" in s:

    print("GroupCallActivity already fixed")

else:

    print("GroupCallActivity pattern not found")

PY



echo
echo "Checking..."

grep -n "getLatencyStats" TMessagesProj/src/main/java/org/telegram/messenger/voip/VoIPService.java

grep -n "service.getLatencyStats" TMessagesProj/src/main/java/org/telegram/ui/GroupCallActivity.java


echo
echo "=============================="
echo " LATENCY UI LINK COMPLETE "
echo "=============================="

