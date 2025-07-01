#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();

    // Iterate over the input data and split each sentence
    for (int idx = 0; idx < inputMap.size(); idx++) {
        auto inputTuple = inputMap[std::to_string(idx)];
        std::map<std::string, std::string> chainedInput;

        chainedInput["host"] = inputTuple["host"];
        chainedInput["event_time"] = inputTuple["event_time"];
        faasm::chainCallNamedId("nwm_fail_aggregation", chainedInput, idx);
    }

    faasmChainInvoke();
    return 0;
}