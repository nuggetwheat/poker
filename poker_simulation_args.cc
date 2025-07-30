#include "poker_simulation_args.h"

#include <cstdlib>
#include <iostream>

#include <argparse/argparse.hpp>

void PokerSimulationArgs::Display() const {
  std::cout << "Game type: ";
  if (game_type == PokerGameType::HOLDEM) {
    std::cout << "holdem\n";
  } else {
    std::cout << "?\n";
  }
  std::cout << "Players: " << players << std::endl;
  std::cout << "Iterations: " << iterations << std::endl;
  if (output_file.empty()) {
    std::cout << "Output file: <stdout>" << std::endl;
  } else {
    std::cout << "Output file: " << output_file << std::endl;
  }
  std::cout << "Statistics:";
  bool stats_output = false;
  if (stats_winning_hand) {
    if (stats_output)
      std::cout << ",";
    std::cout << " winning-hand";
    stats_output = true;
  }
  if (stats_hole_cards) {
    if (stats_output)
      std::cout << ",";
    std::cout << " hole-cards";
    stats_output = true;
  }
  std::cout << std::endl;
}

PokerSimulationArgs ParseArgs(int argc, char *argv[]) {
  PokerSimulationArgs args;
  argparse::ArgumentParser program("poker_simulation");

  // Positional args
  std::string game_type_str;
  program.add_argument("game-type")
    .help("Game to simulate (values: holdem)")
    .store_into(game_type_str);

  // Optional args
  program.add_argument("-p", "--players")
    .help("Number of players")
    .default_value(10)
    .store_into(args.players)
    .scan<'i', int>();
  program.add_argument("-i", "--iterations")
    .help("Number of iterations")
    .default_value(100'000'000)
    .store_into(args.iterations)
    .scan<'i', int>();
  program.add_argument("-o", "--output")
    .help("Output file name")
    .default_value(std::string())
    .store_into(args.output_file);
  program.add_argument("-a", "--append-output")
    .help("Append output to output file")
    .default_value(false)
    .implicit_value(true)
    .store_into(args.append_output);
  program.add_argument("--stats:winning-hand")
    .help("Compute winning hand statistics")
    .default_value(false)
    .store_into(args.stats_winning_hand)
    .implicit_value(true);
  program.add_argument("--stats:hole-cards")
    .help("Compute hole cards statistics")
    .default_value(false)
    .store_into(args.stats_hole_cards)
    .implicit_value(true);

  try {
    program.parse_args(argc, argv);
  }
  catch (const std::exception& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    exit(1);
  }

  if (game_type_str == "holdem") {
    args.game_type = PokerGameType::HOLDEM;
  } else {
    std::cerr << "Unrecognized game type: " << game_type_str << "\n\n";
    std::cerr << program["game-type"] << std::endl;
    exit(1);
  }

  return args;
}
