#include "LogSinkImpl.h"

#include "Instance.h"
#include "YasuMeasurementGate.h"

#ifdef WEBRTC_WIN
#include "windows.h"
#include <ctime>
#else // WEBRTC_WIN
#include <sys/time.h>
#endif // WEBRTC_WIN

namespace tgcalls {

LogSinkImpl::LogSinkImpl(const FilePath &logPath) {
	if (!logPath.data.empty()) {
		_file.open(logPath.data);
	}
}

void LogSinkImpl::OnLogMessage(const std::string &msg, rtc::LoggingSeverity severity, const char *tag) {
	OnLogMessage(std::string(tag) + ": " + msg);
}

void LogSinkImpl::OnLogMessage(const std::string &message, rtc::LoggingSeverity severity) {
	OnLogMessage(message);
}


void LogSinkImpl::OnLogMessage(const std::string &message) {
    std::time_t rawTime;
    std::time(&rawTime);

    struct tm timeinfo;

#ifdef _WIN32
    localtime_s(&timeinfo, &rawTime);

    FILETIME ft;
    unsigned __int64 full = 0;
    GetSystemTimeAsFileTime(&ft);

    full |= ft.dwHighDateTime;
    full <<= 32;
    full |= ft.dwLowDateTime;

    const auto deltaEpochInMicrosecs = 11644473600000000Ui64;
    full -= deltaEpochInMicrosecs;
    full /= 10;
    int32_t milliseconds = (long)(full % 1000000UL) / 1000;
#else
    timeval curTime = { 0 };
    localtime_r(&rawTime, &timeinfo);
    gettimeofday(&curTime, nullptr);
    int32_t milliseconds = curTime.tv_usec / 1000;
#endif

    auto &stream = _file.is_open() ? (std::ostream&)_file : _data;

    // YASU: capture telemetry ONLY while the measurement gate is enabled.
    if (YasuMeasurementsEnabled() &&
        (message.find("YASU ") != std::string::npos ||
         message.find("YASUAGRAM HARDWARE TIMESTAMP") != std::string::npos)) {
        {
            std::lock_guard<std::mutex> lock(_dataMutex);
            _data << message << std::endl;
            // YASU: keep the live buffer small. One huge ostringstream copies
            // the whole log every time it grows, stalling the audio thread.
            if (static_cast<std::streamoff>(_data.tellp()) > 262144) {
                _chunks.push_back(_data.str());
                _data.str(std::string());
            }
        }
    }

    stream
            << (timeinfo.tm_year + 1900)
            << "-" << (timeinfo.tm_mon + 1)
            << "-" << (timeinfo.tm_mday)
            << " " << timeinfo.tm_hour
            << ":" << timeinfo.tm_min
            << ":" << timeinfo.tm_sec
            << ":" << milliseconds
            << " " << message;

#if DEBUG
    printf("%d-%d-%d %d:%d:%d:%d %s\n",
           timeinfo.tm_year + 1900,
           timeinfo.tm_mon + 1,
           timeinfo.tm_mday,
           timeinfo.tm_hour,
           timeinfo.tm_min,
           timeinfo.tm_sec,
           milliseconds,
           message.c_str());
#endif
}

} // namespace tgcalls
