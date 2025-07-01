#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <iostream>
#include <sstream>

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
          std::string msgType = inputTuple["type"];
          if (msgType == "fail_stats") {
              std::string isFisrt = inputTuple["is_first"];
              if (isFisrt == "true") {
                  recordTime = stoi(inputTuple["event_time"]);
              }
          } else if (msgType == "success_login") {
              int eventTime = std::stoi(inputTuple["event_time"]);
              if (eventTime <= recordTime) {
                  std::string output = "host is locked today";
                  faasm::setOutputId(output, idx);
              }
          }
          // Just for debugging purpose
          //   std::ostringstream oss;
          //   oss << "Processing event -- "
          //       << "Host: " << inputTuple["host"]
          //       << ", Record Time: " << recordTime << ", Type: \"" << msgType
          //       << "\""
          //       << ", Event Time: "
          //       << inputTuple["event_time"]; // <-- event_time added here
          //   std::cout << oss.str() << std::endl;
      };

    faasm::processTodoMap<int>(
      todoKeysMap, processOperator, initState, serialize);

    return 0;
}