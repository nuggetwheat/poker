#ifndef HOLDEM_H
#define HOLDEM_H

#include <memory>
#include <vector>
#include <unordered_map>

#include "cards.h"
#include "poker.h"

namespace poker::holdem {

enum class Round {
  INITIAL,
  FLOP,
  TURN,
  RIVER
};

template <typename RNG, typename STATS>
class Game : public poker::Game<RNG> {
public:
  using BaseClass = poker::Game<RNG>;

  Game(std::vector<Player> players, RNG& rng) : BaseClass(players, rng) { }

  void Play(STATS& stats) {
    Round round;
    BaseClass::ResetForNextHand();
    community_cards_.clear();

    stats.NewGame(BaseClass::players(), community_cards_);

    round = Round::INITIAL;
    Deal(round);
    stats.Collect(round);

    round = Round::FLOP;
    Deal(round);
    stats.Collect(round);

    round = Round::TURN;
    Deal(round);
    stats.Collect(round);

    round = Round::RIVER;
    Deal(round);
    stats.Collect(round);
  }
private:
  void Deal(Round round);
  
  std::vector<Card> community_cards_;
};

template <typename RNG, typename STATS>
void Game<RNG, STATS>::Deal(Round round) {
  Deck& deck = BaseClass::deck();
  switch (round) {
  case Round::INITIAL:
    {
      auto& players = BaseClass::players();
      for (int i=0; i<players.size(); i++) {
        *players[i].add_cards() = deck.DealCard();
        *players[i].add_cards() = deck.DealCard();
      }
    }
    break;
  case Round::FLOP:
    community_cards_.push_back(deck.DealCard());
    community_cards_.push_back(deck.DealCard());
    community_cards_.push_back(deck.DealCard());
    break;
  case Round::TURN:
    community_cards_.push_back(deck.DealCard());
    break;
  case Round::RIVER:
    community_cards_.push_back(deck.DealCard());
    break;
  default:
    assert(false && "Unknown round");
  };
}

class Statistics {
public:
  Statistics();
  void NewGame(std::vector<Player>& players,
               std::vector<Card>& community_cards);
  void Collect(Round round);
  void Display();

private:
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
  // Winning hole cards distribution
  // Winning hand distribution
};

} // namespace poker::holdem

#endif // HOLDEM_H
