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
  void NewGame(std::vector<Player>& players,
               std::vector<Card>& community_cards);
  void Collect(Round round);
  void Display(std::ostream& os);

private:
  PokerSimulationArgs args_;
  std::vector<Player>* players_;
  std::vector<Card>* community_cards_;
  std::unordered_map<Hand, int> hand_index_;
  std::unique_ptr<std::vector<int>[]> beat_matrix_;

  struct PlayerInfoT {
    Player* player;
    Hand hole_hand;
    Hand hand;
  };
  std::vector<PlayerInfoT> player_info_;
  std::vector<int32_t> hand_win_count_;
  // Winning hole cards distribution
  // Winning hand distribution
};

} // namespace poker::holdem

#endif // HOLDEM_STATS_H
