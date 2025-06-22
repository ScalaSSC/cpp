#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

// We must register the function_state in scheduler!

std::vector<uint8_t> serialize(const std::pair<long, int>& p)
{
    // Allocate exactly enough bytes for one long + one int.
    std::vector<uint8_t> buf(sizeof(long) + sizeof(int));

    // Copy the `long` (first) into the first sizeof(long) bytes.
    std::memcpy(buf.data(), &p.first, sizeof(long));

    // Copy the `int` (second) right after.
    std::memcpy(buf.data() + sizeof(long), &p.second, sizeof(int));

    return buf;
}

std::pair<long, int> deserialize(const std::vector<uint8_t>& buffer)
{
    constexpr size_t EXPECTED_SIZE = sizeof(long) + sizeof(int);
    if (buffer.size() < EXPECTED_SIZE) {
        // Buffer too small → return a default pair.
        return { 0L, 0 };
    }

    long firstValue;
    int secondValue;

    // Read the first sizeof(long) bytes into `firstValue`.
    std::memcpy(&firstValue, buffer.data(), sizeof(long));

    // Read the next sizeof(int) bytes into `secondValue`.
    std::memcpy(&secondValue, buffer.data() + sizeof(long), sizeof(int));

    return { firstValue, secondValue };
}

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();
    // <partitioned Attribute, <msgIdx, inputTuple>>
    auto todoKeysMap = faasm::generateTodoKeysMap(inputMap, "campaign_id");

    auto initState =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> std::pair<long, int> {
        if (partitionedState.find(key) != partitionedState.end() &&
            partitionedState.at(key).size() > 0) {
            return deserialize(partitionedState.at(key));
        }
        return { 0L, 0 };
    };

    auto processOperator =
      [](const std::string& key,
         const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
         std::pair<long, int>& state) {
          size_t idx = std::get<0>(todoData);
          auto tupleMap = std::get<1>(todoData);

          long eventTime = std::stol(tupleMap["event_time"]);

          long& timeBucket = state.first;
          int& count = state.second;

          // Just used for debugging
          // std::cout << "Processing key: " << key << ", idx: " << idx
          //           << ", event_time: " << eventTime
          //           << ", timeBucket: " << timeBucket << ", count: " <<
          //           count
          //           << std::endl;

          // If state is not initialized, we need to fetch it from Redis
          if (timeBucket == 0L && count == 0) {
              timeBucket = eventTime;
          }
          if (eventTime - timeBucket > 1000) {
              // Flush the state window.
              std::string storeKey = std::to_string(timeBucket) + "_" + key;
              std::string storeValue = std::to_string(count);
              faasm::setPersistentState(storeKey, storeValue);
              timeBucket = eventTime;
              count = 1;
          } else {
              // Increment the count for this time bucket.
              count++;
          }
      };

    faasm::processTodoMap<std::pair<long, int>>(
      todoKeysMap, processOperator, initState, serialize);

    return 0;
}