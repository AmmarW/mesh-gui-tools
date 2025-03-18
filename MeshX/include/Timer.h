/**
 * @file Timer.h
 * @brief Defines a simple timer class for measuring elapsed time.
 *
 * This file provides the Timer class, which uses the high-resolution clock
 * to measure time intervals.
 */

 #pragma once
 #include <chrono>
 
 /**
  * @class Timer
  * @brief A simple timer class to measure elapsed time.
  *
  * This class uses the high-resolution clock to measure the time elapsed
  * between the creation of the Timer object or the last reset and the current time.
  */
 class Timer {
 public:
     /**
      * @brief Constructs a Timer object and starts the timer.
      *
      * The timer starts as soon as the object is created.
      */
     Timer() : start(std::chrono::high_resolution_clock::now()) {}
 
     /**
      * @brief Resets the timer to the current time.
      *
      * This function resets the start time to the current time, effectively restarting the timer.
      */
     void reset() {
         start = std::chrono::high_resolution_clock::now();
     }
 
     /**
      * @brief Returns the elapsed time in milliseconds since the timer was started or last reset.
      *
      * @return The elapsed time in milliseconds as a double.
      */
     double elapsed() const {
         return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
     }
 
 private:
     std::chrono::high_resolution_clock::time_point start; ///< The start time of the timer.
 };