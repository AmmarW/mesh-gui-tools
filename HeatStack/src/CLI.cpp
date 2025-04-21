#include "CLI.h"
#include <iostream>
#include <cstring>  // for strcmp
#include <cstdlib>  // for atof

CLI::CLI(int argc, char* argv[]) {
    parseArguments(argc, argv);
}

void CLI::parseArguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--mesh") == 0 && i + 1 < argc) {
            meshFile = argv[++i];
        }
        else if (std::strcmp(argv[i], "--time") == 0 && i + 1 < argc) {
            timeDuration = std::atof(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--dt") == 0 && i + 1 < argc) {
            timeStep = std::atof(argv[++i]);
        }
        else if (std::strcmp(argv[i], "--adaptive") == 0) {
            adaptiveTimeStep = true;
        }
        else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            outputFile = argv[++i];
        }
        else if (std::strcmp(argv[i], "--help") == 0) {
            helpRequested = true;
            printUsage();
            break;
        }
        else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            helpRequested = true;
            printUsage();
            break;
        }
    }
}

void CLI::printUsage() const {
    std::cout << "Usage: heatstack [options]\n"
              << "Options:\n"
              << "  --mesh <file>       Path to input mesh file\n"
              << "  --time <duration>   Total simulation time (in seconds)\n"
              << "  --dt <timestep>     Fixed timestep size (ignored if --adaptive)\n"
              << "  --adaptive          Use adaptive time stepping\n"
              << "  --output <file>     Output file for temperature results\n"
              << "  --help              Print this help message\n";
}

std::string CLI::getMeshFile() const {
    return meshFile;
}

double CLI::getTimeDuration() const {
    return timeDuration;
}

double CLI::getTimeStep() const {
    return timeStep;
}

bool CLI::useAdaptiveTimeStep() const {
    return adaptiveTimeStep;
}

std::string CLI::getOutputFile() const {
    return outputFile;
}

bool CLI::isHelpRequested() const {
    return helpRequested;
}
