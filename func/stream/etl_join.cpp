#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <iostream>
#include <sstream>

// We must register the function_state in scheduler!

std::vector<uint8_t> serialize(const std::vector<int>& v)
{
    // 1) write a 32-bit length prefix
    uint32_t len = static_cast<uint32_t>(v.size());
    size_t bufSize = sizeof(len) + len * sizeof(int);
    std::vector<uint8_t> buf(bufSize);

    size_t offset = 0;
    // copy length
    std::memcpy(buf.data() + offset, &len, sizeof(len));
    offset += sizeof(len);

    // copy all elements (if any)
    if (!v.empty()) {
        std::memcpy(buf.data() + offset, v.data(), len * sizeof(int));
    }
    return buf;
}

// Deserialize a byte buffer back into std::vector<int>.
std::vector<int> deserialize(const std::vector<uint8_t>& buf)
{
    size_t offset = 0;
    // need at least 4 bytes for the length
    if (buf.size() < sizeof(uint32_t)) {
        return {};
    }

    // read length
    uint32_t len;
    std::memcpy(&len, buf.data() + offset, sizeof(len));
    offset += sizeof(len);

    // verify buffer is large enough
    size_t needed = offset + static_cast<size_t>(len) * sizeof(int);
    if (buf.size() < needed) {
        // malformed buffer
        return {};
    }

    // reconstruct vector
    std::vector<int> v(len);
    if (len > 0) {
        std::memcpy(v.data(), buf.data() + offset, len * sizeof(int));
    }
    return v;
}

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();
    // <partitioned Attribute, <msgIdx, inputTuple>>
    auto todoKeysMap = faasm::generateTodoKeysMap(inputMap, "msg_id");

    auto initState =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> std::vector<int> {
        if (partitionedState.count(key) >= 1 &&
            partitionedState.at(key).size() > 0) {
            return deserialize(partitionedState.at(key));
        }
        return {};
    };

    auto processOperator =
      [](const std::string& key,
         const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
         std::vector<int>& state) {
          size_t idx = std::get<0>(todoData);
          auto inputTuple = std::get<1>(todoData);

          int obsVal = std::stoi(inputTuple["obs_val"]);

          state.push_back(obsVal);
          if (state.size() >= 3) {
              std::string joinedValues;
              bool isFirst = true;
              for (const auto& val : state) {
                  if (!isFirst) {
                      joinedValues += ",";
                  }
                  joinedValues += std::to_string(val);
                  isFirst = false;
              }
              //   std::ostringstream oss;
              //   oss << "jointed values: " << joinedValues
              //       << " for msg_id: " << key
              //       << ", current state size: " << state.size();
              //   std::cout << oss.str() << std::endl;
              state.clear();

              // Build the chained input.
              std::map<std::string, std::string> chainedInput;
              chainedInput["msg_id"] = inputTuple["msg_id"];
              chainedInput["obs_type"] = "joinedValue";
              chainedInput["obs_val"] = joinedValues;

              faasm::chainCallNamedId("etl_annotate", chainedInput, idx);
          }
      };

    faasm::processTodoMap<std::vector<int>>(
      todoKeysMap, processOperator, initState, serialize);

    faasmChainInvoke();

    return 0;
}