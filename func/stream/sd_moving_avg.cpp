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

    auto initStateA =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> std::pair<double, std::list<double>> {
        if (partitionedState.find(key) != partitionedState.end()) {
            return deserialize(partitionedState.at(key));
        }
        return { 0.0, std::list<double>() };
    };

    auto processOperatorA =
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

            std::vector<uint8_t> chainedInputBytes;
            faasm::serializeMap(chainedInputBytes, chainedInput);
            faasmChainNamedId("sd_spike_detect",
                              chainedInputBytes.data(),
                              chainedInputBytes.size(),
                              idx);
      };

    uint64_t start = faasmGetMicros();

    faasm::processTodoMap<std::pair<double, std::list<double>>>(
      todoKeysMap, processOperatorA, initStateA, serialize);

    uint64_t end = faasmGetMicros();
    uint64_t diff = end - start;
    // Print start, end, and duration in microseconds

    int inputSize = inputMap.size();
    std::string output =
      "mo_moving_avg_input_size: " + std::to_string(inputSize) +
      " and duration:" + std::to_string(diff);
    for (size_t i = 0; i < inputMap.size(); i++) {
        faasmSetOutputId(output.c_str(), output.size(), i);
    }

    faasmChainInvoke();

    return 0;
}

// int main(int argc, char* argv[])
// {
//     int windowlength = 100;
//     // get the inputMap (inputdata)
//     auto inputMap = faasm::getInputMap();

//     // concat the input string.
//     // <partitioned Attribute, <msgIdx, inputvalue>>
//     std::map<std::string, std::vector<std::tuple<size_t, double>>>
//     todoKeysMap; for (size_t i = 0; i < inputMap.size(); i++) {
//         // get the input for this spefic function invoke.
//         std::string inputAttr =
//           inputMap[std::to_string(i)]["partitionedAttribute"];
//         double inputData =
//           std::stod(inputMap[std::to_string(i)]["temperature"]);
//         todoKeysMap[inputAttr].push_back(
//           std::tuple<size_t, double>(i, inputData));
//     }

//     uint64_t start = faasmGetMicros();

//     while (todoKeysMap.size() > 0) {
//         // Collect todoKeys to string
//         std::vector<std::string> todoKeys;
//         for (const auto& pair : todoKeysMap) {
//             todoKeys.push_back(pair.first);
//         }
//         auto lockedStates = faasm::getPartitionedStates(todoKeys);
//         const auto& lockedKeysSet = lockedStates.first;
//         auto& partitionedState = lockedStates.second;
//         // For each locked key, process the requests.
//         for (const std::string& key : lockedKeysSet) {
//             // Prepare states of partitioned attribute 'key'
//             std::pair<double, std::list<double>> statistics;
//             if (partitionedState.find(key) != partitionedState.end()) {
//                 statistics = deserialize(partitionedState.at(key));
//             } else {
//                 statistics = { 0.0, std::list<double>() };
//             }
//             double& sum = statistics.first;
//             std::list<double>& values = statistics.second;

//             // Process requests of partitioned attribute 'key'
//             for (const std::tuple<size_t, double>& todoData :
//                  todoKeysMap[key]) {
//                 size_t idx = std::get<0>(todoData);
//                 double todoValue = std::get<1>(todoData);

//                 if (values.size() >= windowlength) {
//                     sum -= values.front();
//                     values.pop_front();
//                 }
//                 sum += todoValue;
//                 values.push_back(todoValue);
//                 double avg = sum / values.size();
//                 // Chained call next function
//                 std::map<std::string, std::string> chainedInput;
//                 chainedInput["movingAverage"] = std::to_string(avg);
//                 chainedInput["temperature"] = std::to_string(todoValue);
//                 std::vector<uint8_t> chainedInputBytes;
//                 faasm::serializeMap(chainedInputBytes, chainedInput);
//                 faasmChainNamedId("sd_spike_detect",
//                                   chainedInputBytes.data(),
//                                   chainedInputBytes.size(),
//                                   idx);
//             }
//             // Print : used for testing
//             // {
//             //     std::string valuesStr = "[";
//             //     for (auto it = values.begin(); it != values.end(); ++it) {
//             //         valuesStr += std::to_string(*it);
//             //         if (std::next(it) != values.end()) {
//             //             valuesStr += ", ";
//             //         }
//             //     }
//             //     valuesStr += "]";

//             //     // Print out statistics including the values in one line
//             //     std::cout << "Key: " << key << ", Count: " <<
//             values.size()
//             //               << ", Sum: " << sum
//             //               << ", Average: " << (sum / values.size())
//             //               << ", Values: " << valuesStr << std::endl;
//             // }
//             std::pair<double, std::list<double>> newStatistics = { sum,
//                                                                    values };
//             std::vector<uint8_t> newStatisticsBytes =
//             serialize(newStatistics); partitionedState[key] =
//             newStatisticsBytes; todoKeysMap.erase(key);
//         }
//         std::vector<uint8_t> partitionedStateBytes =
//           faasm::serializeParState(partitionedState);
//         faasmWriteIndivFunctionStateUnlock(partitionedStateBytes.data(),
//                                            partitionedStateBytes.size());
//     }

//     uint64_t end = faasmGetMicros();
//     uint64_t diff = end - start;
//     // Print start, end, and duration in microseconds

//     int inputSize = inputMap.size();
//     std::string output =
//       "mo_moving_avg_input_size: " + std::to_string(inputSize) + " and
//       duration:" + std::to_string(diff);
//     for (size_t i = 0; i < inputMap.size(); i++) {
//         faasmSetOutputId(output.c_str(), output.size(), i);
//     }

//     faasmChainInvoke();

//     return 0;
// }
