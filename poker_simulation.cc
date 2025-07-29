#include <ctime>
#include <iostream>
#include <random>
#include <vector>

#include "cards.h"
#include "holdem.h"
#include "holdem_stats.h"
#include "poker.pb.h"
#include "poker_simulation_args.h"

int main(int argc, char *argv[]) {
  PokerSimulationArgs args = ParseArgs(argc, argv);
  args.Display();

  std::mt19937 rng(static_cast<unsigned int>(std::time(0)));
  std::vector<poker::Player> players(args.players);
  poker::holdem::Game<std::mt19937, poker::holdem::Statistics> game(players, rng);
  poker::holdem::Statistics stats(args);

  for (int i=0; i<args.iterations; i++) {
    if (i > 0 && i % 1'000'000 == 0) {
      std::cout << "Completed " << i << " games." << std::endl;
    }
    game.Play(stats);
  }
  stats.Display();

  return 0;
}
