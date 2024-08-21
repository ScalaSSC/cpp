#ifndef FAASM_INPUT_H
#define FAASM_INPUT_H

#include "faasm/core.h"
#include <map>
#include <set>
#include <string>
#include <vector>

namespace faasm {
const char* getStringInput(const char* defaultValue);

const std::vector<uint8_t> getInputVec();

const std::map<std::string, std::map<std::string, std::string>> getInputMap();

void setStringOutput(const char* val);

int getIntInput();

int* parseStringToIntArray(const char* inStr, int expected);

// We use "|" to concat string, please make use the input partitioned keys do
// not contain "|"
const std::string concatInput(const std::vector<std::string>& input);

std::set<std::string> splitStringToSet(const std::string& str,
                                       const std::string& delimiter);
}

#endif
