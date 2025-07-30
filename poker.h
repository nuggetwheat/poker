#ifndef POKER_H
#define POKER_H

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <random>
#include <vector>

#include "cards.h"
#include "cards.pb.h"
#include "poker.pb.h"

template<>
struct std::hash<poker::Hand>
{
  std::size_t operator()(const poker::Hand& hand) const noexcept {
    return static_cast<size_t>(hand.sort_code());
  }
};

namespace poker {

inline bool operator==(const Hand& lhs, const Hand& rhs)
{
  return lhs.sort_code() == rhs.sort_code();
}

std::ostream& operator<<(std::ostream& os, const HandType& type);
std::ostream& operator<<(std::ostream& os, const Hand& hand);

int32_t HandToSortCode(Hand& hand);
Hand SortCodeToHand(int32_t sort_code);

Hand HoleHand(Card card1, Card card2);
Hand HoleHand(Rank rank1, Rank rank2, HandType hand_type);

class HandEvaluator {
public:
  void Reset(const std::vector<Card>& community);
  Hand Evaluate(const std::vector<Card>& hole);
private:
  static const int MAX_RANK = static_cast<int>(Rank_ARRAYSIZE);
  static const int MAX_SUIT = static_cast<int>(Suit_ARRAYSIZE);
  int rank_reset_limit_[MAX_RANK];
  int suit_reset_limit_[MAX_SUIT];
  std::vector<Suit> rank_[MAX_RANK];
  std::vector<Rank> suit_[MAX_SUIT];
};

class Table {
public:
  const std::vector<const Player*>& players() const { return players_; }
  void set_players(std::vector<const Player*> players) { players_ = players; }

  int button() const { return button_; }
  void set_button(int button) { button_ = button; }

  const std::vector<Card>& community_cards() const { return community_cards_; }
  void add_community_card(Card card) { community_cards_.push_back(card); }
  void clear_community_cards() { community_cards_.clear(); }

private:
  std::vector<const Player*> players_;
  int button_;
  std::vector<Card> community_cards_;
};

template <typename RNG>
class Game {
public:
  Game(Table& table, std::vector<Player>& players, RNG& rng)
    : rng_(rng), table_(table), players_(players) {
    std::vector<const Player*> table_players;
    table_players.reserve(players.size());
    for (const Player& player : players) {
      table_players.push_back(&player);
    }
    table_.set_players(table_players);
    std::uniform_int_distribution<int> di(0, players_.size()-1);
    table_.set_button(di(rng_));
  }

  void ResetForNextHand() {
    std::for_each(players_.begin(), players_.end(), [](Player& player) {
      player.clear_cards();
    });
    table_.clear_community_cards();
    deck_.Shuffle(rng_);
    table_.set_button(RotatePosition(table_.button(), -1));
  }

protected:
  int RotatePosition(int position, int offset) {
    assert(position < players_.size());
    position += offset;
    if (position < 0) {
      position += players_.size();
    } else if (position >= players_.size()) {
      position -= players_.size();
    }
    return position;
  }
  Deck& deck() { return deck_; }
  std::vector<Player>& players() { return players_; };
  Table& table() { return table_; }

  RNG& rng_;
  Table& table_;
  std::vector<Player>& players_;
  Deck deck_;
};

#if 0
class Table {
public:
  Table(int nplayers) : players_(nplayers), button_(-1) { }
  template <typename URBG>
  void NewGame(URBG& rng) {
    for (Player& player : players_) {
      player.clear_cards();
    }
    if (button_ == -1) {
      std::uniform_int_distribution<int> di(0, players_.size()-1);
      button_ = di(rng);
    } else {
      if (button_ == 0) {
	button_ = players_.size() - 1;
      } else {
	button_--;
      }
    }
    SetNextPlayer(-1);
    deck_.Shuffle(rng);
  }
protected:
  void SetNextPlayer(int from_button) {
    next_player_ += from_button;
    if (next_player_ < 0) {
      next_player_ += players_.size();
    } else if (next_player_ >= players_.size()) {
      next_player_ -= players_.size();
    }
  }
  void NextPlayer() {
    if (--next_player_ < 0) {
      next_player_ += players_.size();
    }
  }
  std::vector<Player> players_;
  int button_;
  int next_player_;
  Deck deck_;
  std::vector<Card> community_cards_;
};
#endif

} // namespace poker

#endif // POKER_H
