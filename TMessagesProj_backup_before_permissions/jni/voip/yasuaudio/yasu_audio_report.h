#ifndef YASU_AUDIO_REPORT_H_
#define YASU_AUDIO_REPORT_H_

#include <stdint.h>
#include <string>
#include <mutex>


class YasuAudioReport {
public:

 static YasuAudioReport& Instance();


 void AddCaptureTime(int64_t us);

 void AddPlaybackTime(int64_t us);

 void AddJitter(int64_t us);

 void AddXRun(int32_t count);


 std::string GenerateReport();


private:

 YasuAudioReport();

 int64_t capture_total_;
 int64_t playback_total_;
 int64_t jitter_total_;

 int64_t capture_max_;
 int64_t playback_max_;
 int64_t jitter_max_;

 int64_t capture_count_;
 int64_t playback_count_;

 int64_t xrun_count_;

 std::mutex mutex_;

};


#endif
