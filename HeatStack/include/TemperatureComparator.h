#ifndef TEMPERATURE_COMPARATOR_H
#define TEMPERATURE_COMPARATOR_H

#include <vector>
#include "MaterialProperties.h"

// Class for comparing temperature distributions to suggest TPS thickness
class TemperatureComparator {

public:
    TemperatureComparator();
    ~TemperatureComparator();

    void setTimeStep(double dt, bool adaptive);
    void setGridResolution(int pointsPerLayer);

    // Run simulation for a given TPS thickness
    std::vector<double> runSimulation(Stack stack, double duration, double theta = 0.5, double l_over_L = 0.0);

    // Suggest minimum TPS thickness to keep steel temperature below maxTemp
    double suggestTPSThickness(const Stack& stack, double maxTemp, double duration, double l_over_L, const MaterialProperties& props, double theta = 0.5);

private:
    double compDt    = 1.0;
    bool   compAdapt = false;
    int compPoints = 10;
};

#endif // TEMPERATURE_COMPARATOR_H