#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

#include <iostream>
#include <sstream>

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();

    // Iterate over the input data and split each sentence
    for (int idx = 0; idx < inputMap.size(); idx++) {
        auto inputTuple = inputMap[std::to_string(idx)];

        std::string inputJson = inputTuple["json"];

        auto parsedMap = faasm::parseJsonToMap(inputJson);
        std::map<std::string, std::string> chainedInput;
        chainedInput["host"] = parsedMap["host"];
        chainedInput["status"] = parsedMap["status"];
        chainedInput["method"] = parsedMap["method"];
        chainedInput["region"] = parsedMap["region"];
        chainedInput["event_time"] = parsedMap["event_time"];

        // Only used for debugging purposes
        // std::ostringstream oss;
        // oss << "Processing event -- "
        //     << "Host: " << parsedMap["host"] << ", Status: \""
        //     << parsedMap["status"] << "\""
        //     << ", Method: \"" << parsedMap["method"] << "\""
        //     << ", Region: " << parsedMap["region"]
        //     << ", Time: " << parsedMap["event_time"];
        // std::cout << oss.str() << std::endl;

        std::vector<uint8_t> chainedInputBytes;
        faasm::serializeMap(chainedInputBytes, chainedInput);
        faasmChainNamedId(
          "nwm_split", chainedInputBytes.data(), chainedInputBytes.size(), idx);
    }

    faasmChainInvoke();
    return 0;
}