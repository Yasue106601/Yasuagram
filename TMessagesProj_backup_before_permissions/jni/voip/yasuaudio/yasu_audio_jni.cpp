#include <jni.h>
#include "yasu_audio_report.h"

extern "C"
JNIEXPORT jstring JNICALL
Java_org_telegram_ui_GroupCallActivity_getYasuAudioReport(
        JNIEnv *env,
        jobject thiz) {

    std::string report =
        YasuAudioReport::Instance().GenerateReport();

    return env->NewStringUTF(report.c_str());
}
