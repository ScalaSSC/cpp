#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <iostream>

// We must register the function_state in scheduler!

std::vector<uint8_t> serialize(int x)
{
    std::vector<uint8_t> buf(sizeof(x));
    std::memcpy(buf.data(), &x, sizeof(x));
    return buf;
}

int deserialize(const std::vector<uint8_t>& buf)
{
    if (buf.size() < sizeof(int)) {
        // not enough bytes
        return 0;
    }
    int x;
    std::memcpy(&x, buf.data(), sizeof(x));
    return x;
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
      -> int {
        if (partitionedState.find(key) != partitionedState.end() &&
            partitionedState.at(key).size() > 0) {
            return deserialize(partitionedState.at(key));
        }
        return 0;
    };

    auto processOperator =
      [](const std::string& key,
         const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
         int& state) {
          size_t idx = std::get<0>(todoData);
          auto inputTuple = std::get<1>(todoData);

          int& recordTime = state;
          int eventTime = std::stoi(inputTuple["record_time"]);
          std::string isFirst = "false";
          if (eventTime > recordTime) {
              recordTime = eventTime;
              isFirst = "true";
          }

          // Build the chained input.
          std::map<std::string, std::string> chainedInput;
          chainedInput["host"] = inputTuple["host"];
          chainedInput["event_time"] = std::to_string(recordTime);
          chainedInput["type"] = "fail_stats";
          chainedInput["is_first"] = isFirst;

          faasm::chainCallNamedId("nwm_join", chainedInput, idx);
      };

    faasm::processTodoMap<int>(
      todoKeysMap, processOperator, initState, serialize);

    faasmChainInvoke();

    return 0;
}