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
        std::string msgId = inputTuple["msg_id"];
        std::string inputJson = inputTuple["json"];

        auto parsedMap = faasm::parseJsonToMap(inputJson);
        std::map<std::string, std::string> chainedInput;
        chainedInput["msg_id"] = msgId;
        chainedInput["sensor_id"] = parsedMap["sensor_id"];
        chainedInput["meta"] = parsedMap["meta"];
        chainedInput["obs_type"] = parsedMap["obs_type"];
        chainedInput["obs_val"] = parsedMap["obs_val"];

        // Just used for debugging
        // std::ostringstream oss;
        // oss << "msg_id: " << msgId << ", sensor_id: " << parsedMap["sensor_id"]
        //     << ", obs_type: " << parsedMap["obs_type"]
        //     << ", obs_val: " << parsedMap["obs_val"]
        //     << ", meta: " << parsedMap["meta"];
        // std::cout << oss.str() << std::endl;

        faasm::chainCallNamedId("etl_filter_range", chainedInput, idx);
    }

    faasmChainInvoke();
    return 0;
}