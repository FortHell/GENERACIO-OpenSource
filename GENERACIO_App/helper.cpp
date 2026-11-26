#include <cstdlib>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <utility>

#include "helper.hpp"

namespace helper {

void openUrl(const std::string& url) {
    #if defined(_WIN32)
        std::system(("start \"\" \"" + url + "\"").c_str());
    #else
        std::system(("xdg-open \"" + url + "\"").c_str());
    #endif
}

std::string readValue(const std::string& fname, const std::string& data) {
    std::ifstream f(fname);
    std::string line;
    std::string moddata = '"' + data + '"';
    bool found = false;

    while (std::getline(f, line)) {
        if (line.find(moddata) < line.find(":")) {
            line = line.substr(line.find(":") + 1);
            found = true;
            break;
        }
    }

    if (found) {
        std::string modline;
        bool next = false;

        for (char c : line) {
            if (next) {
                if (c != '"') {
                    modline += c;
                }
                else {
                    next = false;
                    break;
                }
            }

            if (c == '"') {
                next = true;
            }
        }

        if (modline.empty()) {
            for (char c : line) {
                if (c != ' ' && c != ',' && c != '\r') {
                    modline += c;
                }
            }
        }

        if (modline == "\"\"") {
            modline = "";
        }

        return modline;
    }

    return "ERR";
}

std::vector<std::string> readAllValues(const std::string& fname) {
    std::ifstream f(fname);
    std::string line;
    std::vector<std::string> v;

    while (std::getline(f, line)) {
        line = line.substr(line.find(":") + 1);
        
        std::string modline;
        bool next = false;

        for (char c : line) {
            if (next) {
                if (c != '"') {
                    modline += c;
                }
                else {
                    next = false;
                    break;
                }
            }

            if (c == '"') {
                next = true;
            }
        }

        if (modline.empty()) {
            for (char c : line) {
                if (c != ' ' && c != ',' && c != '\r' && c != '{' && c != '}') {
                    modline += c;
                }
            }
        }

        if (modline != "\"\"" && modline != "") {
            v.push_back(modline);
        }
    }

    return v;
}

std::vector<std::string> readAllKeys(const std::string& fname) {
    std::ifstream f(fname);
    std::string line;
    std::vector<std::string> v;

    while (std::getline(f, line)) {
        line = line.substr(0, line.find(":"));
        
        std::string modline;
        bool next = false;

        for (char c : line) {
            if (next) {
                if (c != '"') {
                    modline += c;
                }
                else {
                    next = false;
                    break;
                }
            }

            if (c == '"') {
                next = true;
            }
        }

        if (modline.empty()) {
            for (char c : line) {
                if (c != ' ' && c != ',' && c != '\r' && c != '{' && c != '}') {
                    modline += c;
                }
            }
        }

        if (modline != "\"\"" && modline != "") {
            v.push_back(modline);
        }
    }

    return v;
}

std::vector<std::pair<std::string, std::chrono::steady_clock::time_point>> timerV;

void startTimer(const std::string& workerName) {
    bool success = true;

    for (auto i : timerV) {
        if (i.first == workerName) {
            success = false;
            break;
        }
    }

    if (success) {
        std::pair<std::string, std::chrono::steady_clock::time_point> p = { workerName, std::chrono::steady_clock::now() };
        timerV.push_back(p);

        std::cout << "Timer (" << workerName << ") started.\n";
    }
    else {
        std::cout << "ERR! Timer (" << workerName << ") was not started.\n";
    }
}

void stopTimer(const std::string& workerName) {
    bool success = false;

    for (size_t i = 0; i < timerV.size(); ++i) {
        if (timerV[i].first == workerName) {
            timerV.erase(timerV.begin() + i);
            success = true;
            break;
        }
    }

    if (success) {
        std::cout << "Timer (" << workerName << ") stopped.\n";
    }
    else {
        std::cout << "ERR! Timer (" << workerName << ") was not found.\n";
    }
}

long long elapsedTimer(const std::string& workerName) {
    bool success = false;
    std::chrono::steady_clock::time_point workerTime;

    for (auto i : timerV) {
        if (i.first == workerName) {
            workerTime = i.second;
            success = true;
            break;
        }
    }

    if (success) {
        auto now = std::chrono::steady_clock::now();
        std::cout << "Timer (" << workerName << ") read.\n";
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - workerTime).count();
    }
    else {
        std::cout << "ERR! Timer (" << workerName << ") was not read.\n";
        return -1;
    }
}

int checkProcess(const std::string& procName) {
    #if defined(_WIN32)
        if (std::system(("tasklist /FI \"IMAGENAME eq " + procName + ".exe\" /NH | find /I \"" + procName + ".exe\" > NUL").c_str()) == 0) {
            return 0;
        }
        else {
            return 1;
        }
    #else
        if (std::system(("pgrep -x " + procName + " > /dev/null").c_str()) == 0) {
            return 0;
        }
        else {
            return 1;
        }
    #endif
}

void openProgram(const std::string& path, bool runInBg) {
    if (path.empty()) {
        std::cout << "Couldn't open program: empty path\n";
        return;
    }

    #if defined(_WIN32)
        // Soon
    #else
        std::string begin = "./";
        std::string end = "&";

        if (!runInBg) end = "";
        if (path[0] == '/' || path.substr(0, 2) == "./") begin = "";
        std::system(("chmod +x \"" + begin + path + "\" && \"" + begin + path + "\" " + end).c_str());
    #endif
}

}