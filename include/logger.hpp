#ifndef FILE_SHARING_LOGGER_HPP
#define FILE_SHARING_LOGGER_HPP

#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

namespace app_log {
inline std::mutex& mutex() {
    static std::mutex m;
    return m;
}

inline void write(const char* level, const std::string& message) {
    static constexpr const char* path = "/var/log/ftpserver.log";
    std::lock_guard<std::mutex> lock(mutex());
    FILE* file = std::fopen(path, "a");
    if (!file) return;

    const std::time_t now = std::time(nullptr);
    std::tm tm_now{};
    localtime_r(&now, &tm_now);
    char timestamp[24]{};
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_now);
    std::fprintf(file, "%s [%s] %s\n", timestamp, level, message.c_str());
    std::fclose(file);
}
inline void info(const std::string& message) { write("INFO", message); }
inline void error(const std::string& message) { write("ERROR", message); }
}
#endif
