#ifndef HOLDEM_H
#define HOLDEM_H

#include <vector>

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

  Game(std::vector<Player> players, RNG& rng) : Game(players, rng) { }

  void Play(STATS& stats) {
    Round round;
    BaseClass::ResetForNextHand();

    round = Round::INITIAL;
    Deal(round);
    stats.Collect(round, BaseClass::players(), community_cards_);

    round = Round::FLOP;
    Deal(round);
    stats.Collect(round, BaseClass::players(), community_cards_);

    round = Round::TURN;
    Deal(round);
    stats.Collect(round, BaseClass::players(), community_cards_);

    round = Round::RIVER;
    Deal(round);
    stats.Collect(round, BaseClass::players(), community_cards_);
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
        players[i].ReceiveCard(deck.DealCard());
        players[i].ReceiveCard(deck.DealCard());
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

} // namespace poker::holdem

#endif // HOLDEM_H
