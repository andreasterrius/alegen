#pragma once

#include<vector>
#include<string>

using namespace std;

std::vector<std::string> stringSplit(const std::string& str, const std::string& delim) {
    std::vector<std::string> result;

    if (delim.empty()) {
        result.push_back(str);
        return result;
    }

    if (str.empty()) {
        return result;
    }

    std::string::size_type start = 0;
    std::string::size_type end = 0;

    while ((end = str.find(delim, start)) != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + delim.length();
    }

    result.push_back(str.substr(start));

    return result;
}