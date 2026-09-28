#ifndef TGCALLS_LOG_SINK_IMPL_H
#define TGCALLS_LOG_SINK_IMPL_H

#include "rtc_base/logging.h"
#include <atomic>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>

namespace tgcalls {

struct FilePath;

class LogSinkImpl final : public rtc::LogSink {
public:
	LogSinkImpl(const FilePath &logPath);

	void OnLogMessage(const std::string &msg, rtc::LoggingSeverity severity, const char *tag) override;
	void OnLogMessage(const std::string &message, rtc::LoggingSeverity severity) override;
	void OnLogMessage(const std::string &message) override;

	std::string result() const {
                std::lock_guard<std::mutex> lock(_dataMutex);
                const std::string tail = _data.str();
                size_t total = tail.size();
                for (const auto &chunk : _chunks) {
                        total += chunk.size();
                }
                std::string out;
                out.reserve(total);
                for (const auto &chunk : _chunks) {
                        out += chunk;
                }
                out += tail;
                return out;
        }

private:
	std::ofstream _file;
	std::ostringstream _data;
        std::vector<std::string> _chunks;
        mutable std::mutex _dataMutex;

};

} // namespace tgcalls

#endif
