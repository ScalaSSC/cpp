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

        std::string joinedValues = inputTuple["obs_val"];
        auto pos = joinedValues.find(',');
        std::string annotateKey =
          (pos != std::string::npos) ? joinedValues.substr(0, pos) : "unknown";

        std::string annotation =
          faasm::getPersistentState("etl_annotate_" + annotateKey);
        joinedValues += "," + annotation;

        // std::ostringstream oss;
        // oss << "etl_annotate joined values: " << joinedValues
        //     << " with annotation: " << annotation
        //     << " for msg_id: " << inputTuple["msg_id"];
        // std::cout << oss.str() << std::endl;

        std::map<std::string, std::string> chainedInput = std::move(inputTuple);
        chainedInput["obs_val"] = joinedValues;

        faasm::chainCallNamedId("etl_azure", chainedInput, idx);
        faasm::chainCallNamedId("etl_csv2ml", chainedInput, idx);
    }

    faasmChainInvoke();
    return 0;
}