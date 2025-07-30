#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include "cards.h"
#include "holdem.h"
#include "holdem_stats.h"
#include "poker.pb.h"
#include "poker_simulation_args.h"
#include "poker_simulation_utils.h"

int main(int argc, char *argv[]) {
  PokerSimulationArgs args = ParseArgs(argc, argv);
  args.Display();

  std::mt19937 rng(static_cast<unsigned int>(std::time(0)));
  std::vector<poker::Player> players(args.players);
  poker::holdem::Game<std::mt19937, poker::holdem::Statistics> game(players, rng);
  poker::holdem::Statistics stats(args);

  // Set up output stream
  std::ofstream fout;
  if (!args.output_file.empty()) {
    if (args.append_output) {
      fout = std::ofstream(args.output_file, std::ios::app);
    } else {
      fout = std::ofstream(args.output_file);
    }
    if (!fout.is_open()) {
      std::cerr << "Error: Unable to open output file '" << args.output_file << "'" << std::endl;
      exit(1);
    }
  }
  std::ostream& out = !args.output_file.empty() ? fout : std::cout;

  ProgressBar progress_bar(args.iterations, 50);
  for (int i=0; i<args.iterations; i++) {
    progress_bar.Update(i);
    game.Play(stats);
  }
  progress_bar.Update(args.iterations);

  stats.Display(out);
  fout.close();

  return 0;
}
