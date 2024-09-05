#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/input.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// We must register the function_state in scheduler!

// Function to serialize a std::list<double> to std::vector<uint8_t>
std::vector<uint8_t> serialize(const std::list<double>& data)
{
    std::vector<uint8_t> buffer;
    for (const double& value : data) {
        uint8_t bytes[sizeof(double)];
        std::memcpy(bytes, &value, sizeof(double));
        buffer.insert(buffer.end(), bytes, bytes + sizeof(double));
    }
    return buffer;
}

// Function to deserialize a std::vector<uint8_t> back to std::list<double>
std::list<double> deserialize(const std::vector<uint8_t>& buffer)
{
    std::list<double> data;
    if (buffer.size() % sizeof(double) != 0) {
        throw std::runtime_error("Invalid buffer size for deserialization.");
    }
    for (size_t i = 0; i < buffer.size(); i += sizeof(double)) {
        double value;
        std::memcpy(&value, buffer.data() + i, sizeof(double));
        data.push_back(value);
    }
    return data;
}

int main(int argc, char* argv[])
{
    int windowlength = 100;
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();

    // concat the input string.
    // <partitioned Attribute(machineId), <msgIdx, score, timestamp>>
    std::map<std::string, std::vector<std::tuple<size_t, double, long>>>
      todoKeysMap;

    for (size_t i = 0; i < inputMap.size(); i++) {
        // get the input for this spefic function invoke.
        std::string inputAttr =
          inputMap[std::to_string(i)]["partitionedAttribute"];
        double score = std::stod(inputMap[std::to_string(i)]["score"]);
        long timestamp = std::stol(inputMap[std::to_string(i)]["timestamp"]);
        todoKeysMap[inputAttr].push_back(
          std::tuple<size_t, double, long>(i, score, timestamp));
    }

    // Print the input keys (partitionedAttribute)
    // std::ostringstream oss;
    // oss << "Partitioned Attributes: ";
    // for (const auto& pair : todoKeysMap) {
    //     oss << pair.first << " ";
    // }
    // std::cout << oss.str() << std::endl;

    uint64_t start = faasmGetMicros();

    while (todoKeysMap.size() > 0) {
        // Collect todoKeys to string
        std::vector<std::string> todoKeys;
        for (const auto& pair : todoKeysMap) {
            todoKeys.push_back(pair.first);
        }
        auto lockedStates = faasm::getPartitionedStates(todoKeys);
        const auto& lockedKeysSet = lockedStates.first;
        auto& partitionedState = lockedStates.second;
        // For each locked key, process the requests.
        for (const std::string& key : lockedKeysSet) {
            // Prepare states of partitioned attribute 'key'
            std::list<double> pastScores;
            if (partitionedState.find(key) != partitionedState.end()) {
                pastScores = deserialize(partitionedState.at(key));
            }
            // Process requests of partitioned attribute 'key'
            for (const auto& todoData : todoKeysMap[key]) {
                size_t idx = std::get<0>(todoData);
                double score = std::get<1>(todoData);
                long timestamp = std::get<2>(todoData);

                pastScores.push_back(score);
                if (pastScores.size() > windowlength) {
                    pastScores.pop_front();
                }

                double sumScore = 0.0;
                for (const double& pastScore : pastScores) {
                    sumScore += pastScore;
                }
                
                // Print the partitioned attribute, sumScore and timestamp
                // std::ostringstream oss1;
                // oss1 << "Partitioned Attribute: " << key
                //      << ", Sum Score: " << sumScore
                //      << ", Timestamp: " << timestamp;
                // std::cout << oss1.str() << std::endl;

                // Chained call next function
                std::map<std::string, std::string> chainedInput;
                chainedInput["machineId"] = key;
                chainedInput["score"] = std::to_string(score);
                chainedInput["timestamp"] = std::to_string(timestamp);
                chainedInput["sumScore"] = std::to_string(sumScore);
                std::vector<uint8_t> chainedInputBytes;
                faasm::serializeMap(chainedInputBytes, chainedInput);
                faasmChainNamedId("mo_alert",
                                  chainedInputBytes.data(),
                                  chainedInputBytes.size(),
                                  idx);
            }
            std::vector<uint8_t> newPastScoresBytes = serialize(pastScores);
            partitionedState[key] = newPastScoresBytes;
            todoKeysMap.erase(key);
        }
        // For each locked key, print the partitioned attribute and past scores
        // for (const auto& pair : partitionedState) {
        //     std::ostringstream oss2;
        //     oss2 << "Partitioned Attribute: " << pair.first
        //          << ", Past Scores: ";
        //     for (const double& pastScore : deserialize(pair.second)) {
        //         oss2 << pastScore << " ";
        //     }
        //     std::cout << oss2.str() << std::endl;
        // }
        std::vector<uint8_t> partitionedStateBytes =
          faasm::serializeParState(partitionedState);
        faasmWriteIndivFunctionStateUnlock(partitionedStateBytes.data(),
                                           partitionedStateBytes.size());
    }

    uint64_t end = faasmGetMicros();
    uint64_t diff = end - start;
    // Print start, end, and duration in microseconds

    std::string output =
      "mo_score_anomaly:" + std::to_string(diff);
    for (size_t i = 0; i < inputMap.size(); i++) {
        faasmSetOutputId(output.c_str(), output.size(), 0);
    }

    faasmChainInvoke();
    return 0;
}
