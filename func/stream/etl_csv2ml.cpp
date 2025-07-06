#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

#include <iostream>
#include <sstream>

std::string toJsonArray(const std::string& csv)
{
    std::istringstream iss(csv);
    std::string token;
    std::vector<std::string> parts;

    // split on commas
    while (std::getline(iss, token, ',')) {
        parts.push_back(token);
    }

    // build JSON array string
    std::string json = "[";
    for (size_t i = 0; i < parts.size(); ++i) {
        // escape any " inside parts[i] if needed
        json += "\"" + parts[i] + "\"";
        if (i + 1 < parts.size())
            json += ",";
    }
    json += "]";
    return json;
}

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();

    // Iterate over the input data and split each sentence
    for (int idx = 0; idx < inputMap.size(); idx++) {
        auto inputTuple = inputMap[std::to_string(idx)];

        std::string obsVal = inputTuple["obs_val"];
        if (obsVal.empty()) {
            obsVal = "0"; // Default value if empty
        } else {
            // Convert obs_val from CSV to JSON array
            obsVal = toJsonArray(obsVal);
        }

        // std::ostringstream oss;
        // oss << "etl_csv2ml obs_val: " << obsVal
        //     << " for msg_id: " << inputTuple["msg_id"];
        // std::cout << oss.str() << std::endl;

        std::map<std::string, std::string> chainedInput = std::move(inputTuple);
        chainedInput["obs_val"] = obsVal;

        faasm::chainCallNamedId("etl_mqtt", chainedInput, idx);
    }

    faasmChainInvoke();
    return 0;
}