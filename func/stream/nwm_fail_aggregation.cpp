#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

// We must register the function_state in scheduler!

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

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();
    // <partitioned Attribute, <msgIdx, inputTuple>>
    auto todoKeysMap = faasm::generateTodoKeysMap(inputMap, "host");

    auto initState =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> std::tuple<int, int> {
        if (partitionedState.find(key) != partitionedState.end() &&
            partitionedState.at(key).size() > 0) {
            return deserialize(partitionedState.at(key));
        }
        return { 0, 0 };
    };

    auto processOperator =
      [](const std::string& key,
         const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
         std::tuple<int, int>& state) {
          size_t idx = std::get<0>(todoData);
          auto inputTuple = std::get<1>(todoData);

          int& recordTime = std::get<0>(state);
          int& recordCount = std::get<1>(state);

          int eventTime = std::stoi(inputTuple["event_time"]);
          if (eventTime == recordTime) {
              recordCount++;
          } else {
              recordTime = eventTime;
              recordCount = 1;
          }

          // Build the chained input.
          std::map<std::string, std::string> chainedInput;
          chainedInput["host"] = inputTuple["host"];
          chainedInput["record_time"] = std::to_string(recordTime);
          chainedInput["record_count"] = std::to_string(recordCount);

          faasm::chainCallNamedId("nwm_fail_aggrfilter", chainedInput, idx);
      };

    faasm::processTodoMap<std::tuple<int, int>>(
      todoKeysMap, processOperator, initState, serialize);

    faasmChainInvoke();

    return 0;
}