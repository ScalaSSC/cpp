#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

#include <iostream>

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();

    // Iterate over the input data and split each sentence
    for (int idx = 0; idx < inputMap.size(); idx++) {
        auto inputTuple = inputMap[std::to_string(idx)];
        std::string status = inputTuple["status"];

        if (status != "fail") {
            continue;
        }

        std::map<std::string, std::string> chainedInput = std::move(inputTuple);
        faasm::chainCallNamedId("nwm_fail_parse", chainedInput, idx);
    }

    faasmChainInvoke();
    return 0;
}