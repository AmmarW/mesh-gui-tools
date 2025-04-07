#ifndef TIME_HANDLER_H
#define TIME_HANDLER_H

class TimeHandler {
public:
    TimeHandler();
    ~TimeHandler();

    // Set fixed time step
    void setTimeStep(double dt);

    // Enable or disable adaptive time stepping
    void setAdaptiveTimeStep(bool adaptive);

    // Return the current time step value
    double getTimeStep() const;

private:
    double timeStep;
    bool adaptive;
};

#endif // TIME_HANDLER_H
