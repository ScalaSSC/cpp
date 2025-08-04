#ifndef FAASM_STATE_H
#define FAASM_STATE_H

#include <map>
#include <set>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>

#define BIT_MASK_8 0b11111111
#define BIT_MASK_32 0b11111111111111111111111111111111

namespace faasm {
void maskDouble(unsigned int* maskArray, unsigned long idx);

void zeroState(const char* key, size_t stateLen);

std::vector<uint8_t> getFunctionStateLock();

void setFunctionStateUnlock(const std::vector<uint8_t>& state);

// rerturn <lockedKeys , partitionedState>
std::pair<std::set<std::string>, std::map<std::string, std::vector<uint8_t>>>
getPartitionedStates(const std::vector<std::string>& todoKeys);

std::string getPersistentState(const std::string& key);

void setPersistentState(const std::string& key, const std::string& value);

std::string getPersistentStateRemote(const std::string& key);

void setPersistentStateRemote(const std::string& key, const std::string& value);

} // namespace faasm

#endif
