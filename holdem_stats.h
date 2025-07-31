#ifndef HOLDEM_STATS_H
#define HOLDEM_STATS_H

#include <vector>

#include "cards.pb.h"
#include "holdem.h"
#include "poker.pb.h"
#include "poker_simulation_args.h"

namespace poker::holdem {

class Statistics {
public:
  Statistics(PokerSimulationArgs& args);
  void NewGame(const poker::Table& table, std::vector<Player>& players);
  void Collect(Round round);
  void Display(std::ostream& os);

private:
  PokerSimulationArgs args_;
  const Table* table_;
  std::vector<Player*> players_;
  std::unordered_map<Hand, int> hand_index_;
  std::unique_ptr<std::vector<int>[]> beat_matrix_;
  std::vector<int32_t> hand_win_count_;
  // Winning hole cards distribution
};

} // namespace poker::holdem

#endif // HOLDEM_STATS_H
