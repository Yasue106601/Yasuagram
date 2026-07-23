#include "yasu_audio_report.h"

#include <sstream>
#include <mutex>


YasuAudioReport::YasuAudioReport()
:
capture_total_(0),
playback_total_(0),
jitter_total_(0),
capture_max_(0),
playback_max_(0),
jitter_max_(0),
capture_count_(0),
playback_count_(0),
xrun_count_(0)
{}



YasuAudioReport&
YasuAudioReport::Instance()
{
 static YasuAudioReport instance;
 return instance;
}



void YasuAudioReport::AddCaptureTime(int64_t us)
{
 std::lock_guard<std::mutex> lock(mutex_);
 capture_total_ += us;
 capture_count_++;

 if(us > capture_max_)
  capture_max_=us;
}



void YasuAudioReport::AddPlaybackTime(int64_t us)
{
 std::lock_guard<std::mutex> lock(mutex_);
 playback_total_ += us;
 playback_count_++;

 if(us > playback_max_)
  playback_max_=us;
}



void YasuAudioReport::AddJitter(int64_t us)
{
 std::lock_guard<std::mutex> lock(mutex_);
 jitter_total_ += us;

 if(us > jitter_max_)
  jitter_max_=us;
}



void YasuAudioReport::AddXRun(int32_t count)
{
 std::lock_guard<std::mutex> lock(mutex_);
 xrun_count_ += count;
}



std::string YasuAudioReport::GenerateReport()
{

 std::lock_guard<std::mutex> lock(mutex_);

 std::stringstream out;


 out<<"===== YASUAGRAM AUDIO REPORT =====\n";


 out<<"Capture Avg(us): "
 << (capture_count_ ?
 capture_total_/capture_count_:0)
 <<"\n";


 out<<"Capture Max(us): "
 <<capture_max_
 <<"\n";


 out<<"Playback Avg(us): "
 << (playback_count_ ?
 playback_total_/playback_count_:0)
 <<"\n";


 out<<"Playback Max(us): "
 <<playback_max_
 <<"\n";


 out<<"Jitter Max(us): "
 <<jitter_max_
 <<"\n";


 out<<"XRuns: "
 <<xrun_count_
 <<"\n";


 out<<"=================================\n";


 return out.str();

}
