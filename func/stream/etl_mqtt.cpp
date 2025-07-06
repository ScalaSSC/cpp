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

        std::string obsVal = inputTuple["obs_val"];

        // std::ostringstream oss;
        // oss << "etl_mqtt obs_val: " << obsVal
        //     << " for msg_id: " << inputTuple["msg_id"];
        // std::cout << oss.str() << std::endl;

        faasm::setPersistentState("etl_mqtt_" + inputTuple["msg_id"], obsVal);

        std::map<std::string, std::string> chainedInput = std::move(inputTuple);
        faasm::chainCallNamedId("etl_sink", chainedInput, idx);
    }

    faasmChainInvoke();
    return 0;
}