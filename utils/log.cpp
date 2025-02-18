#include "log.hpp"

namespace utils {
    void Log(const std::string instance, const std::string message) {
        std::ofstream log_file;
        log_file.open("logs/log_" + instance + ".txt", std::ios::app);
        log_file << message << std::endl;
        log_file.close();
    }
}