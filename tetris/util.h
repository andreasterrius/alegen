#pragma once

#include<vector>
#include<string>

using namespace std;

vector<string> stringSplit(string s, string delimiter) {
    size_t pos = 0;
    std::string token;
    vector<string> strs;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        strs.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    return strs;
}
