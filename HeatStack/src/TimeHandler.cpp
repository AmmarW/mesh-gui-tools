#include "TimeHandler.h"

TimeHandler::TimeHandler() : timeStep(0.01), adaptive(false) {}

TimeHandler::~TimeHandler() {}

void TimeHandler::setTimeStep(double dt) {
    timeStep = dt;
}

void TimeHandler::setAdaptiveTimeStep(bool adaptiveFlag) {
    adaptive = adaptiveFlag;
}

double TimeHandler::getTimeStep() const {
    return timeStep;
}
