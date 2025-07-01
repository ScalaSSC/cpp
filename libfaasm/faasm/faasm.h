#ifndef _FAASM_H
#define _FAASM_H

#include "faasm/core.h"
#include "faasm/serialization.h"
#include "faasm/state.h"

#include <map>
#include <string>
#include <tuple>
#include <vector>

// Use appropriate standard library headers.
namespace faasm {

// Helper types.
using InputMapType = std::map<std::string, std::map<std::string, std::string>>;
using TodoKeyTuple = std::tuple<size_t, std::map<std::string, std::string>>;
using TodoKeysMap = std::map<std::string, std::vector<TodoKeyTuple>>;

// Function declarations.
// void setOutputIdx(const std::string& outputString, int idx);

void chainCallNamedId(const std::string& name,
                      const std::map<std::string, std::string>& chainedInput,
                      size_t idx);

void chainCallNamedId(const std::string& name,
                      const std::map<std::string, std::string>& chainedInput,
                      int idx);

void setOutputId(std::string& output, size_t idx);
void setOutputId(std::string& output, int idx);

TodoKeysMap generateTodoKeysMap(InputMapType& inputMap,
                                const std::string& partitionedAttributeName);

// Template function declaration and definition
// [] means mutable variable.
// typename [State] : The type of state acted upon by each tuple.
// typename ProcessFunc : How to process each tuple.
//                        Input is key, todoData, and [state].
// typename InitFunc : How to initialize the state for each input key.
//                   Input is key and partitionedState. Output is [state].
// typename SerializeFunc : How to serialize each [state] to vector<uint8_t>.
template<typename State,
         typename ProcessFunc,
         typename InitFunc,
         typename SerializeFunc>
inline void processTodoMap(TodoKeysMap& todoKeysMap,
                           ProcessFunc process,
                           InitFunc initState,
                           SerializeFunc serializeState)
{
    while (!todoKeysMap.empty()) {
        // Collect all keys from the todo map.
        std::vector<std::string> keys;
        for (const auto& [key, _] : todoKeysMap) {
            keys.push_back(key);
        }

        // Lock the partitioned state for these keys.
        auto [lockedKeys, partitionedState] = faasm::getPartitionedStates(keys);

        // Process each locked key.
        for (const auto& key : lockedKeys) {
            // Initialize the state using operator-specific logic.
            State state = initState(key, partitionedState);

            // Process each todoData item using operator-specific logic.
            for (const auto& todoData : todoKeysMap[key]) {
                process(key, todoData, state);
            }

            // Serialize and update the partitioned state.
            partitionedState[key] = serializeState(state);
            todoKeysMap.erase(key);
        }

        // Unlock the updated partitioned state.
        std::vector<uint8_t> stateBytes =
          faasm::serializeParState(partitionedState);
        faasmWriteIndivFunctionStateUnlock(stateBytes.data(),
                                           stateBytes.size());
    }
}

} // namespace faasm

#endif
