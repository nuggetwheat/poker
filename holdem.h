#ifndef HOLDEM_H
#define HOLDEM_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <utility>

#include "cards.h"
#include "player_model.h"
#include "poker.h"

namespace poker::holdem {

enum class Round {
  UNSPECIFIED,
  INITIAL,
  FLOP,
  TURN,
  RIVER
};

class PlayerModel {
 public:
  virtual ~PlayerModel() = default;
  virtual PlayerAction Act(const Table &table, Round round, int position,
                           Player &player) = 0;
};

typedef std::vector<std::unique_ptr<poker::holdem::PlayerModel>>
    PlayerModelVector;

inline std::ostream& operator<<(std::ostream& os, const Round& round) {
  switch (round) {
  case Round::INITIAL:
    os << "initial";
    break;
  case Round::FLOP:
    os << "flop";
    break;
  case Round::TURN:
    os << "turn";
    break;
  case Round::RIVER:
    os << "river";
    break;
  default:
    os << "?";
  }
  return os;
}

template <typename RNG, typename STATS>
class Game : public poker::Game<RNG> {
public:
  using Base = poker::Game<RNG>;
  using poker::Game<RNG>::deck;
  using poker::Game<RNG>::players;
  using poker::Game<RNG>::table;

  Game(Table& table, std::vector<Player>& players,
       poker::holdem::PlayerModelVector&& player_models, STATS& stats, RNG& rng)
      : Base(table, players, rng),
        player_models_(std::move(player_models)),
        stats_(stats) {}

  void Play() {
    Round round;
    Base::ResetForNextHand();

    stats_.NewGame(table());

    round = Round::INITIAL;
    Deal(round);
    stats_.Collect(round);

    round = Round::FLOP;
    Deal(round);
    stats_.Collect(round);

    round = Round::TURN;
    Deal(round);
    stats_.Collect(round);

    round = Round::RIVER;
    Deal(round);
    stats_.Collect(round);
  }
private:
  void Deal(Round round);
  poker::holdem::PlayerModelVector player_models_;
  STATS& stats_;
};

template <typename RNG, typename STATS>
void Game<RNG, STATS>::Deal(Round round) {
  switch (round) {
  case Round::INITIAL:
    for (Player& player : players()) {
      *player.add_cards() = deck().DealCard();
      *player.add_cards() = deck().DealCard();
    }
    break;
  case Round::FLOP:
    table().add_community_card(deck().DealCard());
    table().add_community_card(deck().DealCard());
    table().add_community_card(deck().DealCard());
    break;
  case Round::TURN:
    table().add_community_card(deck().DealCard());
    break;
  case Round::RIVER:
    table().add_community_card(deck().DealCard());
    break;
  default:
    assert(false && "Unknown round");
  };
}

} // namespace poker::holdem

#endif // HOLDEM_H
