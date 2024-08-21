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
    // printf the inputMap size
    printf("inputMap size (Batch Size): %ld\n", inputMap.size());

    // Create a list of words
    std::vector<std::string> messages = {
        "cat",     "dog",      "pig",        "horse",    "cow",
        "chicken", "sheep",    "goat",       "rabbit",   "duck",
        "turkey",  "rooster",  "llama",      "alpaca",   "guinea pig",
        "hamster", "ferret",   "parrot",     "canary",   "cockatiel",
        "macaw",   "parakeet", "budgerigar", "lovebird", "african grey",
        "amazon",  "cockatoo", "conure",     "eclectus", "lorikeet",
        "pionus",  "quaker",   "ringneck",   "rosella",  "senegal",
        "caique"
    };

    for (int i = 0; i < inputMap.size(); i++) {
        // Get a random number from 0 to size of messages
        int random_number = faasm::randomInteger(0, messages.size() - 1);
        // Return a random sentence
        std::string message = messages[random_number];
        printf("WordCount source: Created Random sentence: %s\n",
               message.c_str());

        std::map<std::string, std::string> input;
        input["partitionedAttribute"] = message;
        input["key2"] = "key2value";
        std::vector<uint8_t> inputBytes;
        faasm::serializeMap(inputBytes, input);

        faasmChainNamedId(
          "function_parstate", inputBytes.data(), inputBytes.size(), i);
    }

    faasmChainInvoke();
    return 0;
}