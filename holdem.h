#ifndef HOLDEM_H
#define HOLDEM_H

#include <iostream>
#include <memory>
#include <stdexcept>
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

Hand HoleHand(const std::vector<Card>& cards);
Hand HoleHand(Rank rank1, Rank rank2, HandType hand_type);

template <typename RNG, typename STATS>
class Game : public poker::Game<RNG> {
public:
  using Base = poker::Game<RNG>;
  using poker::Game<RNG>::deck;
  using poker::Game<RNG>::table;

  Game(Table& table, std::vector<Player>& players,
       poker::holdem::PlayerModelVector&& player_models, STATS& stats, RNG& rng)
    : Base(table, players.size(), rng),
      players_(players),
      player_models_(std::move(player_models)),
      stats_(stats) {
    std::vector<const Player*> table_players;
    table_players.reserve(players.size());
    for (const Player& player : players) {
      table_players.push_back(&player);
    }
    table.set_players(table_players);
  }

  void Play() {
    Round round;
    NewGame();

    round = Round::INITIAL;
    Deal(round);
    BettingRound(round);
    stats_.Collect(round);

    round = Round::FLOP;
    Deal(round);
    BettingRound(round);
    stats_.Collect(round);

    round = Round::TURN;
    Deal(round);
    BettingRound(round);
    stats_.Collect(round);

    round = Round::RIVER;
    Deal(round);
    BettingRound(round);
    stats_.Collect(round);
  }
private:
  void NewGame();
  void Deal(Round round);
  void BettingRound(Round round);

  std::vector<Player>& players_;
  poker::holdem::PlayerModelVector player_models_;
  STATS& stats_;
  bool initial_bet_{};
};

template <typename RNG, typename STATS>
void Game<RNG, STATS>::NewGame() {
  std::for_each(players_.begin(), players_.end(), [](Player& player) {
    player.reset();
  });
  Base::ResetForNextHand();
  stats_.NewGame(table(), players_);
}

template <typename RNG, typename STATS>
void Game<RNG, STATS>::Deal(Round round) {
  switch (round) {
  case Round::INITIAL:
    for (Player& player : players_) {
      player.add_card(deck().DealCard());
      player.add_card(deck().DealCard());
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

template <typename RNG, typename STATS>
void Game<RNG, STATS>::BettingRound(Round round) {
  int starting_offset = (round == Round::INITIAL) ? -3 : -1;
  int next_to_act = Base::RotatePosition(table().button(), starting_offset);

  for (int remaining = players_.size(); remaining > 0; remaining--) {
    PlayerAction action = player_models_[next_to_act]->Act(table(), round, next_to_act, players_[next_to_act]);
    switch (action) {
    case PlayerAction::CHECK:
      break;
    case PlayerAction::FOLD:
      throw std::logic_error("Unimplemented action: FOLD");
    case PlayerAction::RAISE:
      throw std::logic_error("Unimplemented action: RAISE");
    case PlayerAction::RAISE_ALL_IN:
      throw std::logic_error("Unimplemented action: RAISE_ALL_IN");
    default:
      {
        std::stringstream ss;
        ss << "Invalid action value: " << static_cast<int>(action);
        throw std::logic_error(ss.str());
      }
    }
    next_to_act = Base::RotatePosition(table().button(), -1);
  }
}


#if 0
template <typename RNG, typename STATS>
void Game<RNG, STATS>::BettingRound(Round round) {
  int starting_player;
  int next_player;
  Player* player;
  switch (round) {
  case Round::INITIAL:
    for (int bettor = StartingBettor(round); !BettingFinished(); bettor = NextBettor()) {
    }
    starting_player = RotatePosition(table.button(), -2);
    player = &players[starting_player];
    if (player_models_[starting_player]->Act() == PlayerAction::FOLD) {
      player->fold();
    }
    for (next_player = RotatePosition(starting_player, -1);
         next_player != starting_player;
         next_player = RotatePosition(starting_player, -1)) {
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
#endif

} // namespace poker::holdem

#endif // HOLDEM_H
