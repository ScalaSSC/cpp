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

        auto parsedMap = faasm::parseJsonToMap(inputJson);
        std::map<std::string, std::string> chainedInput;
        chainedInput["url"] = parsedMap["url"];
        chainedInput["userid"] = parsedMap["userid"];
        chainedInput["loadtime"] = parsedMap["loadtime"];

        // Just used for debugging
        // std::ostringstream oss;
        // oss << "PL " << parsedMap["url"] << " for user \""
        //     << parsedMap["userid"] << "\" with loadtime "
        //     << parsedMap["loadtime"] << ".";
        // std::cout << oss.str() << std::endl;

        std::vector<uint8_t> chainedInputBytes;
        faasm::serializeMap(chainedInputBytes, chainedInput);
        faasmChainNamedId(
          "pl_filter", chainedInputBytes.data(), chainedInputBytes.size(), idx);
    }

    faasmChainInvoke();
    return 0;
}