#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <iostream>
#include <list>
#include <sstream>

// We must register the function_state in scheduler!

/**
 * @brief Serializes a list of integers into a vector of bytes.
 * * This function converts each integer in the input list into a sequence of
 * 4 bytes (assuming a 32-bit integer) in big-endian order and appends
 * them to a byte vector. Big-endian means the most significant byte comes
 * first.
 * * @param data A constant reference to a list of integers to be serialized.
 * @return A std::vector<uint8_t> containing the serialized byte data.
 */
std::vector<uint8_t> serialize(const std::list<int>& data)
{
    // Note: For true cross-platform compatibility, it's safer to use
    // fixed-width integers like int32_t instead of int, as the size of int
    // can vary between platforms.

    std::vector<uint8_t> buffer;
    // Reserve memory in advance to improve performance by avoiding multiple
    // reallocations.
    buffer.reserve(data.size() * sizeof(int));

    for (int value : data) {
        // Deconstruct the integer into 4 bytes (big-endian order).
        // (value >> 24) gets the most significant byte.
        // (value >> 16) gets the second most significant byte.
        // (value >> 8) gets the third most significant byte.
        // (value) gets the least significant byte.
        // The `& 0xFF` mask ensures we only get the lowest 8 bits of the
        // shifted result.
        buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    }

    return buffer;
}

/**
 * @brief Deserializes a vector of bytes back into a list of integers.
 * * This function reconstructs integers from a byte vector. It assumes the
 * bytes are arranged in 4-byte, big-endian sequences. It will throw an
 * exception if the buffer size is not a multiple of 4.
 * * @param buffer A constant reference to a vector of bytes to be deserialized.
 * @return A std::list<int> containing the reconstructed integers.
 * @throws std::runtime_error if the buffer size is not a multiple of
 * sizeof(int).
 */
std::list<int> deserialize(const std::vector<uint8_t>& buffer)
{
    // Check if the buffer is valid. Each integer requires sizeof(int) bytes.
    if (buffer.size() % sizeof(int) != 0) {
        std::cerr << "Invalid buffer size for deserialization. "
                     "Size must be a multiple of " +
                       std::to_string(sizeof(int))
                  << std::endl;
        return {};
    }

    std::list<int> data;
    for (size_t i = 0; i < buffer.size(); i += sizeof(int)) {
        // Reconstruct the integer from 4 bytes (big-endian order).
        // The bytes are shifted to their correct positions and combined using
        // bitwise OR. static_cast<int> is used to ensure the byte is promoted
        // to an integer before the bitwise shift operation.
        int value = 0;
        value |= static_cast<int>(buffer[i]) << 24;
        value |= static_cast<int>(buffer[i + 1]) << 16;
        value |= static_cast<int>(buffer[i + 2]) << 8;
        value |= static_cast<int>(buffer[i + 3]);

        data.push_back(value);
    }

    return data;
}

int vectorLength = 5;
int defaultValue = 50;

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();
    // <partitioned Attribute, <msgIdx, inputTuple>>
    auto todoKeysMap =
      faasm::generateTodoKeysMap(inputMap, "obs_type_sensor_id");

    auto initState =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> std::list<int> {
        if (partitionedState.count(key) >= 1 &&
            partitionedState.at(key).size() > 0) {
            return deserialize(partitionedState.at(key));
        }
        return {};
    };

    auto processOperator =
      [](const std::string& key,
         const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
         std::list<int>& state) {
          size_t idx = std::get<0>(todoData);
          auto inputTuple = std::get<1>(todoData);

          int obsVal = std::stoi(inputTuple["obs_val"]);

          // If obsVal is not valid, we have to interpolate it.
          if (obsVal == 0) {
              if (state.size() == 0) {
                  obsVal = defaultValue;
              } else {
                  int sum = 0;
                  for (const auto& val : state) {
                      sum += val;
                  }
                  obsVal = sum / state.size();
              }
          }
          if (state.size() >= vectorLength) {
              state.pop_front();
          }
          state.push_back(obsVal);

        //   std::ostringstream oss;
        //   oss << "Processing obs_val: " << obsVal
        //       << " for obs_type_sensor_id: " << key
        //       << ", current state size: " << state.size() << ":";
        //   for (const auto& val : state) {
        //       oss << val << " ";
        //   }
        //   std::cout << oss.str() << std::endl;

          // Build the chained input.
          std::map<std::string, std::string> chainedInput =
            std::move(inputTuple);
          chainedInput["obs_val"] = std::to_string(obsVal);
          chainedInput.erase("obs_type_sensor_id");

          faasm::chainCallNamedId("etl_join", chainedInput, idx);
      };

    faasm::processTodoMap<std::list<int>>(
      todoKeysMap, processOperator, initState, serialize);

    faasmChainInvoke();

    return 0;
}