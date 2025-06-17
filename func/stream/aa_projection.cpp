#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

#include <cctype>
#include <iostream>
#include <map>
#include <random>
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
        std::map<std::string, std::string> chainedInput;
        chainedInput["ad_id"] = inputTuple["ad_id"];
        chainedInput["event_time"] = inputTuple["event_time"];
        std::vector<uint8_t> chainedInputBytes;
        faasm::serializeMap(chainedInputBytes, chainedInput);
        faasmChainNamedId(
          "aa_join", chainedInputBytes.data(), chainedInputBytes.size(), idx);
    }

    faasmChainInvoke();
    return 0;
}