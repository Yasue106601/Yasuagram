#!/data/data/com.termux/files/usr/bin/bash

set -e

FILE="TMessagesProj/jni/voip/tgcalls/InstanceImpl.cpp"

python3 <<'PY'
from pathlib import Path

p=Path("TMessagesProj/jni/voip/tgcalls/InstanceImpl.cpp")

s=p.read_text()

if "bitrateRecords" not in s:

    print("No GetStats location found")
    exit()

# لا نعدل تلقائياً إذا لم نجد المكان المناسب
print("CallStats available")
PY


echo "Searching GetStats usage..."

grep -n "GetStats()" $FILE

echo "DONE"
