#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>
#include <iostream>
#include <map>
#include <random>
#include <stdio.h>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
    // get the inputMap (inputdata)

    auto inputMap = faasm::getInputMap();

    std::string test = faasm::getPersistentStateRemote("test");

    // std::cout << "Persistent state test: " << test << std::endl;

    faasm::setPersistentStateRemote("test", "new_value");

    return 0;
}