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

class Player {
public:
  const std::vector<Card>& cards() const { return cards_; }
  void add_card(Card card) { cards_.push_back(card); }
  void clear_cards() { cards_.clear(); }

  Hand preflop_hand() { return preflop_hand_; }
  void set_preflop_hand(Hand hand) { preflop_hand_ = hand; }

  Hand flop_hand() { return flop_hand_; }
  void set_flop_hand(Hand hand) { flop_hand_ = hand; }

  Hand turn_hand() { return turn_hand_; }
  void set_turn_hand(Hand hand) { turn_hand_ = hand; }

  Hand river_hand() { return river_hand_; }
  void set_river_hand(Hand hand) { river_hand_ = hand; }

private:
  std::vector<Card> cards_;
  Hand preflop_hand_;
  Hand flop_hand_;
  Hand turn_hand_;
  Hand river_hand_;
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
  Game(Table& table, int player_count, RNG& rng)
    : rng_(rng), table_(table), player_count_(player_count)  {
    std::uniform_int_distribution<int> di(0, player_count-1);
    table_.set_button(di(rng_));
  }

  void ResetForNextHand() {
    table_.clear_community_cards();
    deck_.Shuffle(rng_);
    table_.set_button(RotatePosition(table_.button(), -1));
  }

protected:
  int RotatePosition(int position, int offset) {
    assert(position < player_count_);
    position += offset;
    if (position < 0) {
      position += player_count_;
    } else if (position >= player_count_) {
      position -= player_count_;
    }
    return position;
  }
  Deck& deck() { return deck_; }
  Table& table() { return table_; }

  RNG& rng_;
  Table& table_;
  int player_count_;
  Deck deck_;
};

} // namespace poker

#endif // POKER_H
