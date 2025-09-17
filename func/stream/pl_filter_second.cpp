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
        auto inputTuple = inputMap[std::to_string(idx)];
        std::string category = inputTuple["category"];

        // Just used for debugging
        // std::ostringstream oss;
        // oss << "Event type for " << inputTuple["ad_id"] << " is " << type
        //     << ".";
        // std::cout << oss.str() << std::endl;

        if (category == "unknown") {
            continue;
        }
        std::map<std::string, std::string> chainedInput = std::move(inputTuple);
        std::vector<uint8_t> chainedInputBytes;
        faasm::serializeMap(chainedInputBytes, chainedInput);
        faasmChainNamedId("pl_aggregation",
                          chainedInputBytes.data(),
                          chainedInputBytes.size(),
                          idx);
    }

    faasmChainInvoke();
    return 0;
}