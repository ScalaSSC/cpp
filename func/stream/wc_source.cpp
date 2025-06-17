#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include "faasm/random.h"
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
    std::vector<uint8_t> vec = faasm::getInputVec();
    size_t index = 0;
    std::map<std::string, std::map<std::string, std::string>> inputMap =
      faasm::deserializeNestedMap(vec, index);

    // Create a list of words
    std::vector<std::string> messages = {
        "a picture is worth a thousand words but actions speak louder",
        "the grass is always greener on the other side of the fence",
        "practice makes perfect so never give up on your dreams and goals",
        "you cant judge a book by its cover looks can be deceiving",
        "great minds think alike but fools seldom differ be very discerning"
    };

    // Iterate over the input data and create a random sentence for each
    for (int i = 0; i < inputMap.size(); i++) {
        // Get a random number from 0 to size of messages
        int random_number = faasm::randomInteger(0, messages.size() - 1);
        // Return a random sentence
        std::string message = messages[random_number];

        // Prepare for chain call
        std::map<std::string, std::string> chainedInput;
        chainedInput["sentence"] = message;
        std::vector<uint8_t> chainedInputBytes;
        faasm::serializeMap(chainedInputBytes, chainedInput);
        // Chain call next function.
        faasmChainNamedId("wc_split",
                          chainedInputBytes.data(),
                          chainedInputBytes.size(),
                          i);
    }

    faasmChainInvoke();
    return 0;
}