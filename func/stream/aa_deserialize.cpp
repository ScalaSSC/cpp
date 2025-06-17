#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

#include <cctype>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <vector>

std::map<std::string, std::string> parseJsonToMap(const std::string& json) {
    std::map<std::string, std::string> result;
    size_t pos = 0, n = json.size();

    auto skipWs = [&]() {
        while (pos < n && std::isspace((unsigned char)json[pos])) ++pos;
    };

    skipWs();
    if (pos >= n || json[pos] != '{') {
        std::cerr << "parseJsonToMap: expected '{' at beginning\n";
        return result;
    }
    ++pos;
    skipWs();

    while (pos < n && json[pos] != '}') {
        if (json[pos] != '"') {
            std::cerr << "parseJsonToMap: expected '\"' at start of key\n";
            break;
        }
        ++pos;
        size_t keyStart = pos;
        while (pos < n && json[pos] != '"') {
            if (json[pos] == '\\') pos += 2;
            else ++pos;
        }
        if (pos >= n) {
            std::cerr << "parseJsonToMap: unterminated key string\n";
            break;
        }
        std::string key = json.substr(keyStart, pos - keyStart);
        ++pos; skipWs();

        if (pos >= n || json[pos] != ':') {
            std::cerr << "parseJsonToMap: expected ':' after key\n";
            break;
        }
        ++pos; skipWs();

        if (pos >= n || json[pos] != '"') {
            std::cerr << "parseJsonToMap: expected '\"' at start of value\n";
            break;
        }
        ++pos;
        size_t valStart = pos;
        while (pos < n && json[pos] != '"') {
            if (json[pos] == '\\') pos += 2;
            else ++pos;
        }
        if (pos >= n) {
            std::cerr << "parseJsonToMap: unterminated value string\n";
            break;
        }
        std::string val = json.substr(valStart, pos - valStart);
        ++pos;

        result.emplace(std::move(key), std::move(val));
        skipWs();
        if (pos < n && json[pos] == ',') {
            ++pos; skipWs();
        } else {
            break;
        }
    }

    // no throw on missing '}' — just log
    if (pos >= n || json[pos] != '}') {
        std::cerr << "parseJsonToMap: expected '}' at end\n";
    }
    return result;
}

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();

    // Iterate over the input data and split each sentence
    for (int idx = 0; idx < inputMap.size(); idx++) {
        // EXAMPLE JSON:
        // std::string s = R"({
        //                  "name": "alice",
        //                  "email": "alice@example.com",
        //                  "role": "admin"
        //                  })"

        std::string inputJson = inputMap[std::to_string(idx)]["json"];

        // Just used for debugging
        // std::cout << "Processing input JSON: " << inputJson << std::endl;

        auto parsedMap = parseJsonToMap(inputJson);
        std::map<std::string, std::string> chainedInput;
        chainedInput["user_id"] = parsedMap["user_id"];
        chainedInput["page_id"] = parsedMap["page_id"];
        chainedInput["ad_id"] = parsedMap["ad_id"];
        chainedInput["ad_type"] = parsedMap["ad_type"];
        chainedInput["event_type"] = parsedMap["event_type"];
        chainedInput["event_time"] = parsedMap["event_time"];
        chainedInput["ip_address"] = parsedMap["ip_address"];

        // Just used for debugging
        // std::ostringstream oss;
        // oss << "Ad " << parsedMap["ad_id"] << " had event \""
        //     << parsedMap["event_type"] << "\" at time "
        //     << parsedMap["event_time"] << ".";
        // std::cout << oss.str() << std::endl;

        std::vector<uint8_t> chainedInputBytes;
        faasm::serializeMap(chainedInputBytes, chainedInput);
        faasmChainNamedId(
          "aa_filter", chainedInputBytes.data(), chainedInputBytes.size(), idx);
    }

    faasmChainInvoke();
    return 0;
}