#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

#include <iostream>
#include <sstream>

std::vector<uint8_t> serialize(const std::vector<std::string>& vec)
{
    uint32_t count = static_cast<uint32_t>(vec.size());
    size_t totalSize = sizeof(count);
    for (const auto& s : vec) {
        totalSize += sizeof(uint32_t) + s.size();
    }

    std::vector<uint8_t> buf(totalSize);
    size_t offset = 0;

    std::memcpy(buf.data() + offset, &count, sizeof(count));
    offset += sizeof(count);

    for (const auto& s : vec) {
        uint32_t len = static_cast<uint32_t>(s.size());
        std::memcpy(buf.data() + offset, &len, sizeof(len));
        offset += sizeof(len);

        if (len > 0) {
            std::memcpy(buf.data() + offset, s.data(), len);
            offset += len;
        }
    }

    return buf;
}

// Deserialize a byte buffer back into vector<string>.
// On error: log to cerr and return {}.
std::vector<std::string> deserialize(const std::vector<uint8_t>& buf)
{
    size_t offset = 0;

    if (buf.size() < sizeof(uint32_t)) {
        std::cerr << "deserialize error: buffer too small for count prefix\n";
        return {};
    }

    uint32_t count;
    std::memcpy(&count, buf.data() + offset, sizeof(count));
    offset += sizeof(count);

    std::vector<std::string> vec;
    vec.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        if (offset + sizeof(uint32_t) > buf.size()) {
            std::cerr << "deserialize error: buffer too small for string "
                         "length of element "
                      << i << "\n";
            return {};
        }
        uint32_t len;
        std::memcpy(&len, buf.data() + offset, sizeof(len));
        offset += sizeof(len);

        if (offset + len > buf.size()) {
            std::cerr << "deserialize error: buffer too small for string data "
                         "of element "
                      << i << " (expected " << len << " bytes)\n";
            return {};
        }

        vec.emplace_back(reinterpret_cast<const char*>(buf.data() + offset),
                         len);
        offset += len;
    }

    return vec;
}

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)
    auto inputMap = faasm::getInputMap();

    std::vector<uint8_t> stateVec = faasm::getFunctionStateLock();
    auto state = deserialize(stateVec);

    // etl_azure state size: 2 contents: 64,49,50,annotation 50,28,21,annotation
    // std::ostringstream oss;
    // oss << "etl_azure state size: " << state.size() << " contents:";
    // for (size_t i = 0; i < state.size(); ++i) {
    //     oss << " " << state[i];
    // }
    // std::cout << oss.str() << std::endl;

    // Iterate over the input data and split each sentence
    for (int idx = 0; idx < inputMap.size(); idx++) {
        auto inputTuple = inputMap[std::to_string(idx)];

        state.push_back(inputTuple["obs_val"]);
        // It is one when testing the end-to-end latency 
        if (state.size() >= 3) {
            std::string azureSave;
            for (size_t i = 0; i < state.size(); ++i) {
                azureSave += state[i];
                azureSave += ",";
            }
            faasm::setPersistentState("azure_save", azureSave);
            state.clear();
        }

        std::map<std::string, std::string> chainedInput = std::move(inputTuple);
        faasm::chainCallNamedId("etl_sink", chainedInput, idx);
    }

    // Save the state back to function state.
    std::vector<uint8_t> serializedState = serialize(state);
    faasm::setFunctionStateUnlock(serializedState);

    faasmChainInvoke();
    return 0;
}