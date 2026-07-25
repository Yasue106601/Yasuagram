#!/data/data/com.termux/files/usr/bin/bash

OUT="Yasuagram_latency_check.txt"

echo "===== Yasuagram Latency Verification =====" > $OUT
echo "Date: $(date)" >> $OUT
echo "" >> $OUT


echo "===== 1) Jitter Buffer minDelay / avgDelay =====" >> $OUT
grep -RniE "minDelay|avgDelay|delayHistory|Jitter|jitter" \
TMessagesProj/jni/voip TMessagesProj/src/main/java/org/telegram/messenger/voip \
>> $OUT 2>&1


echo "" >> $OUT
echo "===== 2) AudioTrack Output Size =====" >> $OUT
grep -RniE "audioTrack.write|480|441|960|882|getBufferSize|setBufferSize" \
TMessagesProj/src/main/java/org/telegram/messenger/voip \
TMessagesProj/jni/voip \
>> $OUT 2>&1


echo "" >> $OUT
echo "===== 3) Native Buffer / libtgvoip Buffer =====" >> $OUT
grep -RniE "setBufferSize|bufferSize|framesPerBuffer|samplesPerFrame|frameSize" \
TMessagesProj/jni/voip \
>> $OUT 2>&1


echo "" >> $OUT
echo "===== 4) AudioRecord Input =====" >> $OUT
grep -RniE "AudioRecord|read\\(|960|480|buffer" \
TMessagesProj/src/main/java/org/telegram/messenger/voip \
TMessagesProj/jni/voip \
>> $OUT 2>&1


echo "" >> $OUT
echo "===== 5) Resampler Removal Check =====" >> $OUT
grep -RniE "Resampler|convert48to44|convert44to48|Rescale60To80|Rescale60To40" \
TMessagesProj/jni/voip \
TMessagesProj/src/main/java/org/telegram/messenger/voip \
>> $OUT 2>&1


echo "" >> $OUT
echo "===== 6) Old Backup Files Check =====" >> $OUT
find TMessagesProj -name "*.bak*" -o -name "*backup*" \
>> $OUT 2>&1


echo "" >> $OUT
echo "===== 7) CMake Audio Sources =====" >> $OUT
grep -RniE "Resampler.cpp|AudioOutput|AudioInput|tgcalls|libtgvoip" \
TMessagesProj/jni/voip/CMakeLists.txt \
>> $OUT 2>&1


echo "" >> $OUT
echo "===== END =====" >> $OUT

echo "Report saved: $OUT"
