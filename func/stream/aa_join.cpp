#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// We must register the function_state in scheduler!

std::vector<uint8_t> serialize(const std::string& str)
{
    uint32_t len = static_cast<uint32_t>(str.size());
    std::vector<uint8_t> buf(sizeof(len) + len);
    std::memcpy(buf.data(), &len, sizeof(len));
    if (len > 0) {
        std::memcpy(buf.data() + sizeof(len), str.data(), len);
    }
    return buf;
}

std::string deserialize(const std::vector<uint8_t>& buffer)
{
    if (buffer.size() < sizeof(uint32_t)) {
        return "";
    }
    uint32_t len;
    std::memcpy(&len, buffer.data(), sizeof(len));
    if (buffer.size() < sizeof(len) + len) {
        return "";
    }
    return std::string(
      reinterpret_cast<const char*>(buffer.data() + sizeof(len)), len);
}

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();
    // <partitioned Attribute, <msgIdx, inputTuple>>
    auto todoKeysMap = faasm::generateTodoKeysMap(inputMap, "ad_id");

    auto initState =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> std::string {
        if (partitionedState.find(key) != partitionedState.end()) {
            return deserialize(partitionedState.at(key));
        }
        return "Unknown";
    };

    auto processOperator =
      [](const std::string& key,
         const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
         std::string& state) {
          size_t idx = std::get<0>(todoData);
          auto tupleMap = std::get<1>(todoData);

          // If state is not initialized, we need to fetch it from Redis
          if (state == "Unknown") {
              state = faasm::getPersistentState(key);
          }

          // Just used for debugging
          //   std::ostringstream oss;
          //   oss << "Ad " << key << " is belong to champaign " << state <<
          //   "."; std::cout << oss.str() << std::endl;

          // Build the chained input.
          std::map<std::string, std::string> chainedInput;
          chainedInput["campaign_id"] = state;
          chainedInput["ad_id"] = key;
          chainedInput["event_time"] = tupleMap["event_time"];

          faasm::chainCallNamedId("aa_campaign", chainedInput, idx);
      };

    faasm::processTodoMap<std::string>(
      todoKeysMap, processOperator, initState, serialize);

    faasmChainInvoke();

    return 0;
}