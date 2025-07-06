#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <iostream>
#include <sstream>

std::vector<uint8_t> serialize(const std::tuple<int, int>& tup)
{
    int a = std::get<0>(tup);
    int b = std::get<1>(tup);

    std::vector<uint8_t> buf(sizeof(a) + sizeof(b));
    std::memcpy(buf.data(), &a, sizeof(a));
    std::memcpy(buf.data() + sizeof(a), &b, sizeof(b));
    return buf;
}

std::tuple<int, int> deserialize(const std::vector<uint8_t>& buf)
{
    if (buf.size() < sizeof(int) * 2) {
        return { 0, 0 };
    }

    int a, b;
    std::memcpy(&a, buf.data(), sizeof(a));
    std::memcpy(&b, buf.data() + sizeof(a), sizeof(b));
    return { a, b };
}

std::pair<int, int> splitToInts(const std::string& s)
{
    auto pos = s.find(',');
    if (pos == std::string::npos) {
        return { 0, 0 };
    }

    const std::string firstPart = s.substr(0, pos);
    const std::string secondPart = s.substr(pos + 1);

    int a = std::stoi(firstPart);
    int b = std::stoi(secondPart);

    return { a, b };
}

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();
    // <partitioned Attribute, <msgIdx, inputTuple>>
    auto todoKeysMap = faasm::generateTodoKeysMap(inputMap, "obs_type");

    auto initState =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> std::tuple<int, int> {
        if (partitionedState.count(key) >= 1 &&
            partitionedState.at(key).size() > 0) {
            return deserialize(partitionedState.at(key));
        }
        std::string range =
          faasm::getPersistentState("etl_filter_range_" + key);
        auto [min, max] = splitToInts(range);
        return { min, max };
    };

    auto processOperator =
      [](const std::string& key,
         const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
         std::tuple<int, int>& state) {
          size_t idx = std::get<0>(todoData);
          auto inputTuple = std::get<1>(todoData);

          int obsVal = std::stoi(inputTuple["obs_val"]);
          int min = std::get<0>(state);
          int max = std::get<1>(state);

          // std::ostringstream oss;
          // oss << "Range filter: obsVal: " << obsVal << ", min: " << min
          //     << ", max: " << max;
          // std::cout << oss.str() << std::endl;

          if (obsVal < min || obsVal > max) {
              obsVal = 0; // Set to 0 if out of range
          }

          // Build the chained input.
          std::map<std::string, std::string> chainedInput =
            std::move(inputTuple);
          chainedInput["obs_val"] = std::to_string(obsVal);
          faasm::chainCallNamedId("etl_filter_bloom", chainedInput, idx);
      };

    faasm::processTodoMap<std::tuple<int, int>>(
      todoKeysMap, processOperator, initState, serialize);

    faasmChainInvoke();

    return 0;
}