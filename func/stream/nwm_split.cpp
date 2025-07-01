#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

#include <map>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();

    // Iterate over the input data and split each sentence
    for (int idx = 0; idx < inputMap.size(); idx++) {
        auto inputTuple = inputMap[std::to_string(idx)];

        auto chainedInput = std::move(inputTuple);

        faasm::chainCallNamedId("nwm_success_filter", chainedInput, idx);
        faasm::chainCallNamedId("nwm_fail_filter", chainedInput, idx);
    }

    faasmChainInvoke();
    return 0;
}