#include "faasm/input.h"
#include <faasm/faasm.h>
#include <faasm/serialization.h>
#include <faasm/state.h>

#include <cctype>
#include <iostream>
#include <sstream>

// They must match the bashes and bits in the data generator.
const size_t m_hashes = 3;
const size_t m_bits = 50;

std::vector<bool> decode(const std::string& encoded_string)
{
    // Initialize an empty vector to store the boolean values.
    std::vector<bool> decoded_vector;

    // Reserve memory for the vector to avoid reallocations, which is a
    // good practice for performance if the size is known.
    decoded_vector.reserve(encoded_string.length());

    // Iterate over each character in the input string.
    for (char c : encoded_string) {
        // The expression (c == '1') evaluates to true if the character is '1'
        // and false otherwise. We push this boolean result directly into the
        // vector.
        decoded_vector.push_back(c == '1');
    }

    return decoded_vector;
}

std::vector<uint8_t> serialize(const std::vector<bool>& vec)
{
    std::vector<uint8_t> buffer;
    uint64_t size = vec.size();

    // 1. Store the 64-bit size at the beginning of the buffer.
    buffer.resize(sizeof(uint64_t));
    std::copy(reinterpret_cast<const uint8_t*>(&size),
              reinterpret_cast<const uint8_t*>(&size) + sizeof(uint64_t),
              buffer.begin());

    // 2. Pack the boolean values into bytes.
    if (size > 0) {
        uint8_t current_byte = 0;
        int bit_count = 0;
        for (bool b : vec) {
            // Set the bit in the current byte.
            if (b) {
                current_byte |= (1 << bit_count);
            }
            bit_count++;
            // When the byte is full, push it to the buffer and reset.
            if (bit_count == 8) {
                buffer.push_back(current_byte);
                current_byte = 0;
                bit_count = 0;
            }
        }
        // Push the last byte if it's not empty.
        if (bit_count > 0) {
            buffer.push_back(current_byte);
        }
    }

    return buffer;
}

/**
 * @brief Deserializes a byte vector back into a vector of booleans.
 *
 * @param buf The byte vector created by the serialize function.
 * @return The deserialized std::vector<bool>.
 */
std::vector<bool> deserialize(const std::vector<uint8_t>& buf)
{
    if (buf.size() < sizeof(uint64_t)) {
        return {}; // Not enough data to even read the size.
    }

    // 1. Read the size from the beginning of the buffer.
    uint64_t size = 0;
    std::copy(buf.begin(),
              buf.begin() + sizeof(uint64_t),
              reinterpret_cast<uint8_t*>(&size));

    std::vector<bool> result;
    result.reserve(size);

    // 2. Unpack the bytes back into boolean values.
    for (size_t i = sizeof(uint64_t); i < buf.size() && result.size() < size;
         ++i) {
        uint8_t current_byte = buf[i];
        for (int j = 0; j < 8 && result.size() < size; ++j) {
            result.push_back((current_byte >> j) & 1);
        }
    }

    return result;
}

std::vector<size_t> hashes(const std::string& s)
{
    std::vector<size_t> result;
    std::hash<std::string> hasher;
    size_t h1 = hasher(s);
    // Second hash: hash of reversed string
    std::string rev(s.rbegin(), s.rend());
    size_t h2 = hasher(rev);
    for (size_t i = 0; i < m_hashes; ++i) {
        result.push_back(h1 + i * h2);
    }
    return result;
}

bool possiblyContains(const std::string& s, const std::vector<bool>& bits)
{
    // Add a check to prevent crashing if the bit vector is empty
    if (bits.empty()) {
        return false;
    }

    auto hs = hashes(s);
    for (auto h : hs) {
        if (!bits[h % m_bits])
            return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();
    // <partitioned Attribute, <msgIdx, inputTuple>>
    auto todoKeysMap = faasm::generateTodoKeysMap(inputMap, "obs_type");

    auto initState =
      [](const std::string& key,
         const std::map<std::string, std::vector<uint8_t>>& partitionedState)
      -> std::vector<bool> {
        // First, check for state passed from a chained call
        if (partitionedState.count(key) > 0 &&
            partitionedState.at(key).size() > 0) {
            return deserialize(partitionedState.at(key));
        }

        // Otherwise, load from persistent storage
        std::string hashBitsStr =
          faasm::getPersistentState("etl_filter_bloom_" + key);

        std::vector<bool> hashBitArr = decode(hashBitsStr);

        // Now, use the consistent deserialize function
        return hashBitArr;
    };

    auto processOperator =
      [](const std::string& key,
         const std::tuple<size_t, std::map<std::string, std::string>>& todoData,
         std::vector<bool>& state) {
          //   std::ostringstream oss;
          //   oss << "deserialized state: ";
          //   for (bool val : state) {
          //       oss << (val ? '1' : '0');
          //   }
          //   std::cout << oss.str() << std::endl;

          size_t idx = std::get<0>(todoData);
          auto inputTuple = std::get<1>(todoData);

          std::string obsVal = inputTuple["obs_val"];

          bool mightContain = possiblyContains(obsVal, state);

          // This log should now appear
          //   std::ostringstream oss1;
          //   oss1 << "Bloom Filter processing obs_val: " << obsVal
          //       << ", mightContain: " << mightContain << " for obs_type: " <<
          //       key;
          //   std::cout << oss1.str() << std::endl;

          if (!mightContain) {
              obsVal = "0";
          }

          std::map<std::string, std::string> chainedInput =
            std::move(inputTuple);
          chainedInput["obs_val"] = obsVal;
          chainedInput["obs_type_sensor_id"] =
            chainedInput["obs_type"] + "_" + chainedInput["sensor_id"];

          faasm::chainCallNamedId("etl_interpolation", chainedInput, idx);
      };

    faasm::processTodoMap<std::vector<bool>>(
      todoKeysMap, processOperator, initState, serialize);

    faasmChainInvoke();

    return 0;
}
