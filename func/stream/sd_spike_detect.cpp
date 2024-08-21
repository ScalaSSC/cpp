#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

#include <cmath>
#include <iostream>
#include <map>
#include <string>

int main(int argc, char* argv[])
{
    double spikeThreshold = 0.03; // Replace with actual value
    // get the inputMap
    auto inputMap = faasm::getInputMap();

    // Iterate over the input data and split each sentence
    for (int i = 0; i < inputMap.size(); i++) {
        double movingAverage =
          std::stod(inputMap[std::to_string(i)]["movingAverage"]);
        double temperature =
          std::stod(inputMap[std::to_string(i)]["temperature"]);
        if (std::abs(temperature - movingAverage) >
            spikeThreshold * movingAverage) {
            std::string output = "detected spike";
            faasmSetOutputId(output.c_str(), output.size(), i);
            std::cout << "Input " << i << ": Moving Average = " << movingAverage
                      << ", Temperature = " << temperature
                      << ", Status = Detected" << std::endl;
        } else {
            std::string output = "no spike";
            faasmSetOutputId(output.c_str(), output.size(), i);
            std::cout << "Input " << i << ": Moving Average = " << movingAverage
                      << ", Temperature = " << temperature
                      << ", Status = Undetected" << std::endl;
        }
    }

    return 0;
}