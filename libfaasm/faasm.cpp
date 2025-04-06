#include "faasm/faasm.h"
#include "faasm/serialization.h"
#include "faasm/state.h"

namespace faasm {
void setOutputIdx(const std::string& outputString, int idx)
{
    faasmSetOutputId(outputString.c_str(), outputString.length(), idx);
}

TodoKeysMap generateTodoKeysMap(InputMapType& inputMap,
                                const std::string& partitionedAttributeKey)
{
    TodoKeysMap todoKeysMap;

    for (size_t i = 0; i < inputMap.size(); i++) {
        auto tupleIt = inputMap.find(std::to_string(i));
        if (tupleIt == inputMap.end()) {
            printf("Input %zu not found in inputMap\n", i);
            throw std::runtime_error("Input tuple not found");
        }
        auto& inputTuple = tupleIt->second;

        auto attributeIt = inputTuple.find(partitionedAttributeKey);
        if (attributeIt == inputTuple.end()) {
            printf("Partitioned attribute %s not found in inputMap\n",
                   partitionedAttributeKey.c_str());
            throw std::runtime_error("Partitioned attribute not found");
        }
        std::string partitionKey = attributeIt->second;

        // Create a tuple with the message id and the full attribute map.
        TodoKeyTuple todoTuple = std::make_tuple(i, std::move(inputTuple));

        // Insert the tuple into the map under the corresponding partition key.
        todoKeysMap[partitionKey].push_back(std::move(todoTuple));
    }

    return todoKeysMap;
}
}
