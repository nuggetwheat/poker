#include <cstdlib>
#include <ctime>
#include <filesystem>
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

  if (!std::filesystem::is_directory(args.output_dir)) {
    std::cerr << "Error: Invalid output directory '" << args.output_dir << "'" << std::endl;
    exit(1);
  }

  ProgressBar progress_bar(args.iterations, 50);
  for (int i=0; i<args.iterations; i++) {
    progress_bar.Update(i);
    game.Play();
  }
  progress_bar.Update(args.iterations);

  stats.Display();

  return 0;
}
