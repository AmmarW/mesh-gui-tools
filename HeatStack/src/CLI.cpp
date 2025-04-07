#include "CLI.h"
#include <iostream>

CLI::CLI() {}

CLI::~CLI() {}

void CLI::showHelp() const {
    std::cout << "HeatStack Application CLI" << std::endl;
    std::cout << "Usage: HeatStack [options]" << std::endl;
    // Additional help information can be added here.
}

void CLI::run() {
    std::cout << "Running HeatStack simulation..." << std::endl;
    showHelp();
    
    // Placeholder: Parse command-line arguments and run the simulation.
}
