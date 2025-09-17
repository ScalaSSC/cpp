#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <iostream>
#include <sstream>

std::vector<uint8_t> serialize(const std::tuple<double, int>& tup)
{
    // 1. Get the elements from the tuple
    double a = std::get<0>(tup);
    int b = std::get<1>(tup);

    // 2. Create a buffer of the correct size
    std::vector<uint8_t> buf(sizeof(a) + sizeof(b));

    // 3. Copy the bytes of the double into the buffer
    std::memcpy(buf.data(), &a, sizeof(a));

    // 4. Copy the bytes of the int into the buffer, after the double
    std::memcpy(buf.data() + sizeof(a), &b, sizeof(b));

    return buf;
}

std::tuple<double, int> deserialize(const std::vector<uint8_t>& buf)
{
    // 1. Check if the buffer is large enough to hold both a double and an int
    if (buf.size() < sizeof(double) + sizeof(int)) {
        // Return a default value or throw an exception if the buffer is too
        // small
        return { 0.0, 0 };
    }

    // 2. Declare variables to hold the deserialized data
    double a;
    int b;

    // 3. Copy bytes from the buffer into the double
    std::memcpy(&a, buf.data(), sizeof(a));

    // 4. Copy bytes from the buffer (after the double) into the int
    std::memcpy(&b, buf.data() + sizeof(a), sizeof(b));

    // 5. Return the newly constructed tuple
    return { a, b };
}

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();
    // <partitioned Attribute, <msgIdx, inputTuple>>
    auto todoKeysMap = faasm::generateTodoKeysMap(inputMap, "category");

    auto initState =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> std::tuple<double, int> {
        if (partitionedState.count(key) >= 1 &&
            partitionedState.at(key).size() > 0) {
            return deserialize(partitionedState.at(key));
        }
        return { 0.0, 0 };
    };

    auto processOperator =
      [](const std::string& key,
         const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
         std::tuple<double, int>& state) {
          size_t idx = std::get<0>(todoData);
          auto inputTuple = std::get<1>(todoData);
          double& avg = std::get<0>(state);
          int& count = std::get<1>(state);

          int loadtime = std::stoi(inputTuple["loadtime"]);
          count++;
          avg += (loadtime - avg) / static_cast<double>(count);

          if (count > 1000) {
              avg = 0.0;
              count = 0;
          }

          // std::ostringstream oss;
          // oss << "Aggr - category: " << key << ", avg: " << avg
          //     << ", count: " << count << ", loadtime: " << loadtime;
          // std::cout << oss.str() << std::endl;

          // Build the chained input.
          std::map<std::string, std::string> chainedInput =
            std::move(inputTuple);
          chainedInput["avg"] = std::to_string(avg);
          faasm::chainCallNamedId("pl_sink", chainedInput, idx);
      };

    faasm::processTodoMap<std::tuple<double, int>>(
      todoKeysMap, processOperator, initState, serialize);

    faasmChainInvoke();

    return 0;
}