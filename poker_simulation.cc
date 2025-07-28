#include <ctime>
#include <iostream>
#include <random>
#include <vector>

#include "poker.pb.h"
#include "cards.h"
#include "holdem.h"

int main() {
  std::mt19937 rng(static_cast<unsigned int>(std::time(0)));
  std::vector<poker::Player> players(10);
  poker::holdem::Game<std::mt19937, poker::holdem::Statistics> game(players, rng);
  poker::holdem::Statistics stats;

  for (int i=0; i<100'000'000; i++) {
    game.Play(stats);
  }
  stats.Display();

  return 0;
}
