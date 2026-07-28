//
// libtgvoip is free and unencumbered public domain software.
// For more information, see http://unlicense.org or the UNLICENSE file
// you should have received with this source code distribution.
//

#include "AudioOutputAndroid.h"
#include <stdio.h>
#include "../../logging.h"
#include "tgnet/FileLog.h"
#include "../latency_dashboard/LatencyDashboard.h"
#include "rtc_base/time_utils.h"

extern JavaVM* sharedJVM;

using namespace tgvoip;
using namespace tgvoip::audio;

jmethodID AudioOutputAndroid::initMethod=NULL;
jmethodID AudioOutputAndroid::releaseMethod=NULL;
jmethodID AudioOutputAndroid::startMethod=NULL;
jmethodID AudioOutputAndroid::stopMethod=NULL;
jclass AudioOutputAndroid::jniClass=NULL;

AudioOutputAndroid::AudioOutputAndroid(){
	JNIEnv* env=NULL;
	bool didAttach=false;
	sharedJVM->GetEnv((void**) &env, JNI_VERSION_1_6);
	if(!env){
		sharedJVM->AttachCurrentThread(&env, NULL);
		didAttach=true;
	}

	jmethodID ctor=env->GetMethodID(jniClass, "<init>", "(J)V");
	jobject obj=env->NewObject(jniClass, ctor, (jlong)(intptr_t)this);
	DEBUG_REF("AudioOutputAndroid");
	javaObject=env->NewGlobalRef(obj);

	env->CallVoidMethod(javaObject, initMethod, 48000, 16, 1, 480*2);

	if(didAttach){
		sharedJVM->DetachCurrentThread();
	}
	running=false;
}

AudioOutputAndroid::~AudioOutputAndroid(){
	JNIEnv* env=NULL;
	bool didAttach=false;
	sharedJVM->GetEnv((void**) &env, JNI_VERSION_1_6);
	if(!env){
		sharedJVM->AttachCurrentThread(&env, NULL);
		didAttach=true;
	}

	env->CallVoidMethod(javaObject, releaseMethod);
	DEBUG_DELREF("AudioOutputAndroid");
	env->DeleteGlobalRef(javaObject);
	javaObject=NULL;

	if(didAttach){
		sharedJVM->DetachCurrentThread();
	}
}

void AudioOutputAndroid::Start(){
	JNIEnv* env=NULL;
	bool didAttach=false;
	sharedJVM->GetEnv((void**) &env, JNI_VERSION_1_6);
	if(!env){
		sharedJVM->AttachCurrentThread(&env, NULL);
		didAttach=true;
	}

	env->CallVoidMethod(javaObject, startMethod);

	if(didAttach){
		sharedJVM->DetachCurrentThread();
	}
	running=true;
}

void AudioOutputAndroid::Stop(){
	running=false;
	JNIEnv* env=NULL;
	bool didAttach=false;
	sharedJVM->GetEnv((void**) &env, JNI_VERSION_1_6);
	if(!env){
		sharedJVM->AttachCurrentThread(&env, NULL);
		didAttach=true;
	}

	env->CallVoidMethod(javaObject, stopMethod);

	if(didAttach){
		sharedJVM->DetachCurrentThread();
	}
}

void AudioOutputAndroid::HandleCallback(JNIEnv* env, jbyteArray buffer){
	if(!running)
		return;
	unsigned char* buf=(unsigned char*) env->GetByteArrayElements(buffer, NULL);
	size_t len=(size_t) env->GetArrayLength(buffer);
	int64_t renderStart = rtc::TimeMicros();

        LatencyReport inputReport;
        inputReport.audioRenderInputTimestamp = renderStart;
        LatencyDashboard::Instance().Update(inputReport);

        InvokeCallback(buf, len);

        int64_t renderEnd = rtc::TimeMicros();

        LatencyReport latencyReport;
        latencyReport.audioRenderOutputTimestamp = renderEnd;
        latencyReport.audioRenderQueueDelay =
            (renderEnd - renderStart) / 1000.0;

        LatencyDashboard::Instance().Update(latencyReport);
	env->ReleaseByteArrayElements(buffer, (jbyte *) buf, 0);
}


bool AudioOutputAndroid::IsPlaying(){
	return running;
}
