from pathlib import Path
import re
import subprocess
import shutil
from datetime import datetime

ROOT = Path.cwd()
STAMP = datetime.now().strftime("%Y%m%d_%H%M%S")
BACKUP = ROOT / f".yasu_measurement_backup_{STAMP}"

FILES = {
    "network": ROOT/"TMessagesProj/jni/voip/tgcalls/NetworkManager.cpp",
    "group": ROOT/"TMessagesProj/jni/voip/tgcalls/group/GroupNetworkManager.cpp",
    "channel": ROOT/"TMessagesProj/jni/voip/webrtc/audio/channel_receive.cc",
    "neteq": ROOT/"TMessagesProj/jni/voip/webrtc/modules/audio_coding/neteq/neteq_impl.cc",
    "acm": ROOT/"TMessagesProj/jni/voip/webrtc/modules/audio_coding/acm2/acm_receiver.cc",
    "transport": ROOT/"TMessagesProj/jni/voip/webrtc/audio/audio_transport_impl.cc",
    "adb": ROOT/"TMessagesProj/jni/voip/webrtc/modules/audio_device/audio_device_buffer.cc",
    "fine": ROOT/"TMessagesProj/jni/voip/webrtc/modules/audio_device/fine_audio_buffer.cc",
    "aaudio": ROOT/"TMessagesProj/jni/voip/webrtc/modules/audio_device/android/aaudio_wrapper.cc",
}

def txt(p):
    return p.read_text(errors="replace")

def backup(p):
    d = BACKUP / p.relative_to(ROOT)
    d.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(p, d)

def replace_once(p, old, new, name):
    s = txt(p)
    n = s.count(old)
    if n == 1:
        backup(p)
        p.write_text(s.replace(old, new, 1))
        print("[ADDED]", name)
    elif n == 0:
        print("[SKIP ]", name, "- pattern not found")
    else:
        print("[SKIP ]", name, f"- found {n} times")

print("============================================================")
print(" YASUAGRAM E2E MEASUREMENT AUTO AUDIT")
print("============================================================")

for p in FILES.values():
    if not p.exists():
        print("[ERROR] Missing:", p)
        raise SystemExit(1)

# Remove the old incorrect T0 from start().
old_t0 = '''    // YASU FORENSIC T0
    static int yasu_t0_count = 0;
    if ((++yasu_t0_count % 100) == 0) {
        RTC_LOG(LS_VERBOSE)
            << "YASU FORENSIC T0 UDP_RX "
            << "time_us=" << rtc::TimeMicros();
    }

'''

replace_once(
    FILES["network"],
    old_t0,
    "",
    "remove old invalid T0"
)

# Add the real T0 to transportPacketReceived().
p = FILES["network"]
s = txt(p)

if "YASU FORENSIC T0 UDP_RX_REAL" not in s:
    pattern = re.compile(
        r'(void NetworkManager::transportPacketReceived\('
        r'rtc::PacketTransportInternal \*transport, const char \*bytes, '
        r'size_t size, const int64_t &timestamp, int unused\) \{\s*'
        r'assert\(_thread->IsCurrent\(\)\);\s*)'
    )

    m = pattern.search(s)

    if m:
        insert = r'''\1
    // YASU FORENSIC T0: actual network packet arrival.
    static uint64_t yasu_t0_packet_id = 0;
    const uint64_t yasu_t0_id = ++yasu_t0_packet_id;
    const int64_t yasu_t0_time_us = rtc::TimeMicros();

    RTC_LOG(LS_VERBOSE)
        << "YASU FORENSIC T0 UDP_RX_REAL"
        << " t0_id=" << yasu_t0_id
        << " time_us=" << yasu_t0_time_us
        << " transport_timestamp=" << timestamp
        << " size=" << size;

'''
        backup(p)
        p.write_text(s[:m.start()] + insert + s[m.end():])
        print("[ADDED] real T0")
    else:
        print("[SKIP ] real T0 - function not found")
else:
    print("[OK   ] real T0 already exists")

print()
print("----- MARKER AUDIT -----")

checks = [
    ("T0 REAL", "network", "YASU FORENSIC T0 UDP_RX_REAL"),
    ("T1 RTP", "channel", "YASU FORENSIC T1 RTP_RX"),
    ("T2 NETEQ", "neteq", "YASU FORENSIC T2 NETEQ_INSERT"),
    ("T3 DECISION", "neteq", "YASU FORENSIC T3 NETEQ_DECISION"),
    ("T11 OUTPUT", "acm", "T11_NETEQ_OUTPUT"),
    ("T12 PLAYOUT", "transport", "T12_PLAYOUT_FRAME"),
    ("T6 ADB", "adb", "YASU FORENSIC T6 PLAYOUT_REQUEST"),
    ("T9 ADB TOTAL", "adb", "YASU FORENSIC T9 REQUEST_TOTAL"),
    ("T13 AAUDIO", "aaudio", "T13_AAUDIO_END"),
    ("AAUDIO HW", "aaudio", "YASU AAUDIO HW"),
]

missing = []

for name, key, marker in checks:
    count = txt(FILES[key]).count(marker)
    if count:
        print(f"PASS  {name:15} {count}")
    else:
        print(f"FAIL  {name:15} MISSING")
        missing.append(name)

print()
print("----- RTP/NETEQ CORRELATION -----")

neteq = txt(FILES["neteq"])

for field in ("e2e_id=", "ssrc=", "seq=", "rtp_ts="):
    print(
        f"{'PASS' if field in neteq else 'FAIL'}  {field}"
    )

print()
print("----- AAUDIO HARDWARE SOURCE -----")

aa = txt(FILES["aaudio"])

for field in (
    "AAudioStream_getTimestamp(",
    "frames_written()",
    "frames_read()",
    "EstimateLatencyMillis()",
):
    print(
        f"{'PASS' if field in aa else 'FAIL'}  {field}"
    )

print()
print("----- FINE BUFFER BRIDGE -----")

fine = txt(FILES["fine"])

for field in (
    "RequestPlayoutData(",
    "GetPlayoutData(",
    "playout_buffer_",
):
    print(
        f"{'PASS' if field in fine else 'FAIL'}  {field}"
    )

print()
print("----- GROUP RTP ENTRYPOINTS -----")

group = txt(FILES["group"])

for field in (
    "RtpPacketReceived",
    "DeliverRtpPacket",
    "processRtpPacket",
):
    print(
        f"{'PASS' if field in group else 'MISS'}  {field}"
    )

print()
print("----- GIT DIFF CHECK -----")

r = subprocess.run(
    ["git", "diff", "--check"],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)

if r.returncode == 0:
    print("PASS  git diff --check")
else:
    print("FAIL  git diff --check")
    print(r.stdout)

print()
print("----- FINAL SOURCE CHAIN -----")

for name, key, marker in checks:
    ok = marker in txt(FILES[key])
    print(f"{'PASS' if ok else 'FAIL':5} {name}")

print()
print("----- MODIFIED FILES -----")

r = subprocess.run(
    ["git", "status", "--short"],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
)

print(r.stdout)

print("============================================================")

if missing:
    print("RESULT: INCOMPLETE")
    print("Missing:", ", ".join(missing))
else:
    print("RESULT: COMPLETE SOURCE MARKER CHAIN")

print()
print("BUILD : NOT RUN")
print("PUSH  : NOT RUN")
print("BACKUP:", BACKUP)
print("============================================================")
