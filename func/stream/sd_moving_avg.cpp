#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// We must register the function_state in scheduler!

std::vector<uint8_t> serialize(const std::pair<double, std::list<double>>& p)
{
    std::vector<uint8_t> buffer;

    // Serialize the first double
    const uint8_t* double_data = reinterpret_cast<const uint8_t*>(&p.first);
    buffer.insert(buffer.end(), double_data, double_data + sizeof(double));

    // Serialize the size of the list
    size_t size = p.second.size();
    const uint8_t* size_data = reinterpret_cast<const uint8_t*>(&size);
    buffer.insert(buffer.end(), size_data, size_data + sizeof(size));

    // Serialize each double in the list
    for (double val : p.second) {
        const uint8_t* val_data = reinterpret_cast<const uint8_t*>(&val);
        buffer.insert(buffer.end(), val_data, val_data + sizeof(double));
    }

    return buffer;
}

std::pair<double, std::list<double>> deserialize(
  const std::vector<uint8_t>& buffer)
{
    size_t offset = 0;

    // Deserialize the first double
    double first;
    std::memcpy(&first, buffer.data() + offset, sizeof(double));
    offset += sizeof(double);

    // Deserialize the size of the list
    size_t size;
    std::memcpy(&size, buffer.data() + offset, sizeof(size));
    offset += sizeof(size);

    // Deserialize each double and populate the list
    std::list<double> second;
    for (size_t i = 0; i < size; ++i) {
        double val;
        std::memcpy(&val, buffer.data() + offset, sizeof(double));
        offset += sizeof(double);
        second.push_back(val);
    }

    return std::move(std::make_pair(first, second));
}

int main(int argc, char* argv[])
{
    int windowlength = 100;
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();
    // <partitioned Attribute, <msgIdx, inputTuple>>
    auto todoKeysMap =
      faasm::generateTodoKeysMap(inputMap, "partitionedAttribute");

    auto initState =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> std::pair<double, std::list<double>> {
        if (partitionedState.find(key) != partitionedState.end()) {
            return deserialize(partitionedState.at(key));
        }
        return { 0.0, std::list<double>() };
    };

    auto processOperator =
      [windowlength](
        const std::string& key,
        const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
        std::pair<double, std::list<double>>& state) {
          size_t idx = std::get<0>(todoData);
          auto tupleMap = std::get<1>(todoData);
          if (tupleMap.find("temperature") == tupleMap.end()) {
              throw std::runtime_error("Temperature not found in input map");
          }
          double todoValue = std::stod(tupleMap["temperature"]);

          // If the window is full, remove the oldest value.
          if (state.second.size() >= windowlength) {
              state.first -= state.second.front();
              state.second.pop_front();
          }
          // Add the new value.
          state.first += todoValue;
          state.second.push_back(todoValue);

          // Calculate the moving average.
          double avg = state.first / state.second.size();

          // Build the chained input.
          std::map<std::string, std::string> chainedInput;
          chainedInput["movingAverage"] = std::to_string(avg);
          chainedInput["temperature"] = std::to_string(todoValue);

          faasm::chainCallNamedId("sd_spike_detect", chainedInput, idx);
      };

    // uint64_t start = faasmGetMicros();

    faasm::processTodoMap<std::pair<double, std::list<double>>>(
      todoKeysMap, processOperator, initState, serialize);

    // uint64_t end = faasmGetMicros();
    // uint64_t diff = end - start;
    // Print start, end, and duration in microseconds

    // int inputSize = inputMap.size();
    // std::string output =
    //   "mo_moving_avg_input_size: " + std::to_string(inputSize) +
    //   " and duration:" + std::to_string(diff);
    // for (size_t i = 0; i < inputMap.size(); i++) {
    //     faasmSetOutputId(output.c_str(), output.size(), i);
    // }

    faasmChainInvoke();

    return 0;
}