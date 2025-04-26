#pragma once
#include <string>

class CLI {
public:
    CLI(int argc, char* argv[]);

    std::string getMeshFile() const;
    double getTimeDuration() const;
    double getTimeStep() const;
    bool useAdaptiveTimeStep() const;
    std::string getOutputFile() const;
    bool isHelpRequested() const;

    // New getters:
    std::string getInitFile() const;
    int         getNumSlices() const;
    int         getPointsPerLayer() const;
    double      getTheta() const;


private:
    void parseArguments(int argc, char* argv[]);
    void printUsage() const;

    std::string meshFile;
    double timeDuration = 0.0;
    double timeStep = 0.0;
    bool adaptiveTimeStep = false;
    std::string outputFile = "output.txt";
    bool helpRequested = false;


    // Backing fields for the new flags
    std::string initFile;
    int         numSlices       = 10;
    int         pointsPerLayer  = 10;
    double      theta           = 1.0;
};
