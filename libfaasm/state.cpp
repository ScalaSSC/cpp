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

std::pair<std::set<std::string>, std::map<std::string, std::vector<uint8_t>>>
getPartitionedStates(const std::vector<std::string>& todoKeys)
{
    std::string todoKeysStr = faasm::concatInput(todoKeys);

    // Prepare the memory space for locked keys
    int lockedKeysSize = todoKeysStr.size() + 1;
    auto lockedKeys = new uint8_t[lockedKeysSize];

    // Read and lock the state size
    size_t readSize =
      faasmReadIndivFunctionStateSizeLock(todoKeysStr.c_str(), lockedKeys);

    // Get the Locked Keys
    std::string lockedKeysStr(reinterpret_cast<char*>(lockedKeys));
    delete[] lockedKeys; // Clean up allocated memory
    auto lockedKeysSet = faasm::splitStringToSet(lockedKeysStr, "|");

    // Initialize the partitioned state map
    std::map<std::string, std::vector<uint8_t>> partitionedState;
    if (readSize != 0) {
        // Read the state and deserialize it
        std::vector<uint8_t> stateBuffer(readSize);
        faasmReadIndivFunctionState(
          stateBuffer.data(), readSize, lockedKeysStr.c_str());
        partitionedState = faasm::deserializeParState(stateBuffer);
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

}
