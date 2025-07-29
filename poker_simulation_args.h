#ifndef POKER_SIMULATION_ARGS_H
#define POKER_SIMULATION_ARGS_H

#include <string>

//
// This file depends on github.com/p-ranav/argparse
//

enum class PokerGameType {
  UNSPECIFIED = 0,
  HOLDEM = 1,
};

struct PokerSimulationArgs {
  PokerGameType game_type = PokerGameType::UNSPECIFIED;
  int players = 10;
  int iterations = 100'000'000;
  std::string output_file;
  bool stats_winning_hand = false;
  bool stats_hole_cards = false;
  void Display() const;
};

PokerSimulationArgs ParseArgs(int argc, char *argv[]);

#endif // POKER_SIMULATION_ARGS_H
