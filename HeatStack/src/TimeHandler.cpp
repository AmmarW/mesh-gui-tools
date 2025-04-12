#include "TimeHandler.h"

TimeHandler::TimeHandler(double totalTime_, double initialTimeStep, bool adaptive_)
    : totalTime(totalTime_), dt(initialTimeStep), currentTime(0.0), adaptive(adaptive_), stepCount(0) {}

void TimeHandler::advance() {
    currentTime += dt;
    stepCount++;
}

double TimeHandler::getCurrentTime() const {
    return currentTime;
}

double TimeHandler::getTimeStep() const {
    return dt;
}

double TimeHandler::getTotalTime() const {
    return totalTime;
}

int TimeHandler::getStepCount() const {
    return stepCount;
}

void TimeHandler::adjustTimeStep(double newTimeStep) {
    if (adaptive) {
        dt = newTimeStep;
        // Optional: add logging or safety checks
    }
}

bool TimeHandler::isAdaptive() const {
    return adaptive;
}

bool TimeHandler::isFinished() const {
    return currentTime >= totalTime;
}
