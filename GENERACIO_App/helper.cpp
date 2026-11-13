#include <cstdlib>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>

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

    return "Error!";
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

int main() {
    //openUrl("steam://run/250820");
    /*
    std::string key;
    std::getline(std::cin, key);
    std::cout << readValue("example.json", key) << "\n";
    */
    std::vector<std::string> resKey = readAllKeys("resolutions.json");
    std::vector<std::string> resVal = readAllValues("resolutions.json");

    for (const auto& i : resKey) {
        std::cout << i << "\n";
    }

    for (const auto& i : resVal) {
        std::cout << i << "\n";
    }
}