#pragma once
#include <string>
#include <vector>

namespace helper {
    void openUrl(const std::string& url);
    std::string readValue(const std::string& fname, const std::string& data);
    std::vector<std::string> readAllValues(const std::string& fname);
    std::vector<std::string> readAllKeys(const std::string& fname);
    void startTimer(const std::string& workerName);
    void stopTimer(const std::string& workerName);
    long long elapsedTimer(const std::string& workerName);
    int checkProcess(const std::string& procName);
    void openProgram(const std::string& path, bool runInBg = true);
}