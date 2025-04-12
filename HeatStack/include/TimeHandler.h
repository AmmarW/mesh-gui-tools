#pragma once

class TimeHandler {
public:
    TimeHandler(double totalTime, double initialTimeStep, bool adaptive = false);

    // Advance the simulation time
    void advance();

    // Accessors
    double getCurrentTime() const;
    double getTimeStep() const;
    double getTotalTime() const;
    int getStepCount() const;

    // Adaptive timestep control (stub for now)
    void adjustTimeStep(double newTimeStep); // Can be used by solver based on stability
    bool isAdaptive() const;

    // Check if simulation is complete
    bool isFinished() const;

private:
    double totalTime;
    double dt;
    double currentTime;
    bool adaptive;
    int stepCount;
};
