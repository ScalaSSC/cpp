#include "faasm/core.h"
#include "faasm/faasm.h"
#include "faasm/input.h"
#include <faasm/serialization.h>

#include <cmath>
#include <iostream>
#include <sstream>

double spikeThreshold = 0.03; // Replace with actual value

int main(int argc, char* argv[])
{
    // get the inputMap
    auto inputMap = faasm::getInputMap();

    // Iterate over the input data and split each sentence
    for (int i = 0; i < inputMap.size(); i++) {
        auto inputTuple = inputMap[std::to_string(i)];
        double movingAverage = std::stod(inputTuple["movingAverage"]);
        double temperature = std::stod(inputTuple["temperature"]);
        if (std::abs(temperature - movingAverage) >
            spikeThreshold * movingAverage) {
            std::string output = "detected spike";
            faasm::setOutputId(output, i);

            // std::ostringstream oss;
            // oss << "Input sensor: " << inputTuple["sensor_id"]
            //     << " -> Moving Average = " << movingAverage
            //     << ", Temperature = " << temperature << ", Status = Detected"
            //     << std::endl;
            // std::cout << oss.str();
        } else {
            std::string output = "no spike";
            faasm::setOutputId(output, i);

            // std::ostringstream oss;
            // oss << "Input sensor: " << inputTuple["sensor_id"]
            //     << " -> Moving Average = " << movingAverage
            //     << ", Temperature = " << temperature << ", Status =
            //     Undetected"
            //     << std::endl;
            // std::cout << oss.str();
        }
    }

    return 0;
}