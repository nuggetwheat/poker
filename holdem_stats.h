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
  void Display();

  static constexpr int kHoleHandCount = 169;
  static constexpr int kSortCodeLimit = 10'415'855;

private:
  PokerSimulationArgs args_;
  const Table* table_;
  std::vector<Player*> players_;
  std::unordered_map<Hand, int> hole_hand_index_;

  // Vector to hold the number of times each hole hand appeared in a game.
  std::vector<int32_t> hole_hand_appearance_;

  struct RoundStats {
    std::vector<std::vector<int32_t>> beat_matrix;
    std::vector<int32_t> hand_win_count;
    std::vector<int32_t> hole_hand_wins;
    std::vector<std::vector<float>> win_percentage_matrix;
  };

  RoundStats round_stats_[kRoundMax];

  void CollectRound(Round round);

  struct HandTypeWinStats {
    std::vector<int32_t> wins;
    uint32_t count{};
    uint32_t median_offset{};
  };
};

} // namespace poker::holdem

#endif // HOLDEM_STATS_H
