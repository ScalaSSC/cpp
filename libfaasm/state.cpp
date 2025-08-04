#include "faasm/state.h"
#include "faasm/core.h"
#include "faasm/input.h"
#include "faasm/serialization.h"

#include <algorithm>

namespace faasm {
void maskDouble(unsigned int* maskArray, unsigned long idx)
{
    // NOTE - we assume int is half size of double
    unsigned long intIdx = 2 * idx;
    maskArray[intIdx] |= BIT_MASK_32;
    maskArray[intIdx + 1] |= BIT_MASK_32;
}

void zeroState(const char* key, size_t stateLen)
{
    auto arr = new uint8_t[stateLen];
    std::fill(arr, arr + stateLen, 0);
    faasmWriteState(key, arr, stateLen);
    faasmPushState(key);
}

static uint32_t readBe32(const uint8_t* p)
{
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) << 8) | (uint32_t(p[3]));
}

std::pair<std::set<std::string>, std::map<std::string, std::vector<uint8_t>>>
getPartitionedStates(const std::vector<std::string>& todoKeys)
{
    std::string todoKeysStr = faasm::concatInput(todoKeys);

    // Prepare the memory space for locked keys
    int lockedKeysSize = todoKeysStr.size() + 1;
    auto lockedKeys = new uint8_t[lockedKeysSize];

    int32_t ptrOffset = faasmReadIndivFunctionStatePtr(todoKeysStr.c_str());

    auto* base = reinterpret_cast<const uint8_t*>(uintptr_t(ptrOffset));

    // First 4 bytes are the length of the data
    uint32_t dataLen = readBe32(base);

    // Get the serialized data
    const uint8_t* payload = base + sizeof(uint32_t);
    std::vector<uint8_t> serialized(payload, payload + dataLen);
    auto partitionedState = deserializeFuncState(serialized);
    // Get the locked keys
    std::set<std::string> lockedKeysSet;
    for (auto& kv : partitionedState) {
        lockedKeysSet.insert(kv.first);
    }

    // Return both the locked keys string and the partitioned state using
    // std::move to avoid reconstruction
    return std::make_pair(std::move(lockedKeysSet),
                          std::move(partitionedState));
}

std::string getPersistentState(const std::string& key)
{
    int32_t str_offset = faasmReadPersistentState(key.c_str());

    char* str = (char*)str_offset;
    return str;
}

void setPersistentState(const std::string& key, const std::string& value)
{
    // Convert the value to a C-style string
    const char* valueCStr = value.c_str();
    faasmWritePersistentState(key.c_str(), valueCStr);
}

std::string getPersistentStateRemote(const std::string& key)
{
    int32_t str_offset = faasmReadPersistentStateRemote(key.c_str());

    char* str = (char*)str_offset;
    return str;
}

void setPersistentStateRemote(const std::string& key, const std::string& value)
{
    // Convert the value to a C-style string
    const char* valueCStr = value.c_str();
    faasmWritePersistentStateRemote(key.c_str(), valueCStr);
}

std::vector<uint8_t> getFunctionStateLock()
{
    // Get and initialize the Function
    auto ptr = faasmReadFunctionStateLockPtr();

    if (ptr == 0) {
        return {};
    }

    // ptr is an offset in linear memory; treat it as a byte pointer
    uint8_t* base = reinterpret_cast<uint8_t*>(ptr);

    // decode 4-byte big-endian length
    uint32_t len = (uint32_t(base[0]) << 24) | (uint32_t(base[1]) << 16) |
                   (uint32_t(base[2]) << 8) | uint32_t(base[3]);

    // copy payload into a vector and return
    std::vector<uint8_t> result(len);
    std::memcpy(result.data(), base + 4, len);
    return result;
}

void setFunctionStateUnlock(const std::vector<uint8_t>& state)
{
    faasmWriteFunctionStateUnlock(state.data(), state.size());
}

}
