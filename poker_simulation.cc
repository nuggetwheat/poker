#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "cards.h"
#include "holdem.h"
#include "holdem_stats.h"
#include "poker.pb.h"
#include "player_model_holdem.h"
#include "poker_simulation_args.h"
#include "poker_simulation_utils.h"

namespace {

poker::holdem::PlayerModelVector
CreatePlayerModels(const PokerSimulationArgs& args) {
  poker::holdem::PlayerModelVector player_models(args.players);
  for (int i=0; i<args.players; i++) {
    player_models[i] = poker::holdem::PlayerModelFactory::Create(args.player_model);
  }
  return player_models;
}

}

int main(int argc, char *argv[]) {
  PokerSimulationArgs args = ParseArgs(argc, argv);
  args.Display();

  poker::Table table;
  std::vector<poker::Player> players(args.players);
  poker::holdem::PlayerModelVector player_models = CreatePlayerModels(args);
  poker::holdem::Statistics stats(args);
  std::mt19937 rng(static_cast<unsigned int>(std::time(0)));
  poker::holdem::Game<std::mt19937, poker::holdem::Statistics> game(table, players, std::move(player_models), stats, rng);

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
    // std::cerr << "iteration " << i << std::endl;
    progress_bar.Update(i);
    game.Play();
  }
  progress_bar.Update(args.iterations);

  stats.Display(out);
  fout.close();

  return 0;
}
