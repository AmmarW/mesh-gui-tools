#pragma once
#include <chrono>

class Timer {
public:
    Timer() : start(std::chrono::high_resolution_clock::now()) {}

    void reset() {
        start = std::chrono::high_resolution_clock::now();
    }

    double elapsed() const {
        return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
    }

private:
    std::chrono::high_resolution_clock::time_point start;
};