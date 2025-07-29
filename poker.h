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

template <typename RNG>
class Game {
public:
  Game(std::vector<Player>& players, RNG& rng)
    : rng_(rng), players_(players), button_(-1) { }

  void ResetForNextHand() {
    std::for_each(players_.begin(), players_.end(), [](Player& player) {
      player.clear_cards();
    });
    deck_.Shuffle(rng_);
    if (button_ == -1) {
      std::uniform_int_distribution<int> di(0, players_.size()-1);
      button_ = di(rng_);
    } else {
      button_ = RotatePosition(button_, -1);
    }
  }
  int button() { return button_; }

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

  RNG& rng_;
  Deck deck_;
  std::vector<Player> players_;
  int button_;
};

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

} // namespace poker

#endif // POKER_H
