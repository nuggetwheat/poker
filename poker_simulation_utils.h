#ifndef POKER_SIMULATION_UTILS_H
#define POKER_SIMULATION_UTILS_H

#include <chrono>
#include <iostream>
#include <string>

class ProgressBar {
 public:
  ProgressBar(int total_iterations, int bar_width)
      : total_iterations_(total_iterations), bar_width_(bar_width) {
    start_time_ = std::chrono::high_resolution_clock::now();
  }

  void Update(int progress) {
    if (!complete_ && progress >= next_report_iteration_) Print(progress);
  }

  void Print(int progress) {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = now - start_time_;

    float percentage = static_cast<float>(progress) / total_iterations_;
    int filled_width_ = static_cast<int>(bar_width_ * percentage);

    std::cout << "\r[";  // Carriage return and start of bar
    for (int i = 0; i < filled_width_; ++i) {
      std::cout << "=";
    }
    for (int i = filled_width_; i < bar_width_; ++i) {
      std::cout << " ";  // Empty part of the bar
    }
    std::cout << "] " << static_cast<int>(percentage * 100) << "%";
    std::cout << " (" << std::setw(2) << std::setfill('0')
              << static_cast<int>(elapsed_time.count()) / 60;
    std::cout << ":" << std::setw(2) << std::setfill('0')
              << static_cast<int>(elapsed_time.count()) % 60 << ")";
    if (progress == total_iterations_) {
      std::cout << "\n";
      complete_ = true;
    }
    std::cout.flush();  // Ensure immediate display
    next_report_iteration_ += total_iterations_ / bar_width_;
  }

 private:
  int total_iterations_;
  int bar_width_;
  std::chrono::time_point<std::chrono::high_resolution_clock> start_time_;
  int next_report_iteration_ = 0;
  bool complete_ = false;
};

#endif  // POKER_SIMULATION_UTILS_H
