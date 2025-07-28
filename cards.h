#ifndef CARDS_H
#define CARDS_H

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

#include "cards.pb.h"

std::ostream& operator<<(std::ostream& os, const Rank& rank);
std::wostream& operator<<(std::wostream& os, const Rank& rank);
std::wostream& operator<<(std::wostream& os, const Suit& suit);
std::wostream& operator<<(std::wostream& os, const Card& card);

inline Rank OffsetRank(Rank rank, int offset) {
  return static_cast<Rank>(static_cast<int>(rank) + offset);
}

inline int ByRankAceHighSortFn(Card lhs, Card rhs) {
  if (lhs.rank() == rhs.rank())
    return lhs.suit() < rhs.suit();
  return lhs.rank() < rhs.rank();
}

inline int ByRankAceLowSortFn(Card lhs, Card rhs) {
  if (lhs.rank() == rhs.rank())
    return lhs.suit() < rhs.suit();
  if (lhs.rank() == Rank::ACE) {
    return true;
  } else if (rhs.rank() == Rank::ACE) {
    return false;
  }
  return lhs.rank() < rhs.rank();
}

inline int BySuitAceHighSortFn(Card lhs, Card rhs) {
  if (lhs.suit() == rhs.suit())
    return lhs.rank() < rhs.rank();
  return lhs.suit() < rhs.suit();
}

inline int BySuitAceLowSortFn(Card lhs, Card rhs) {
  if (lhs.suit() == rhs.suit()) {
    if (lhs.rank() == Rank::ACE) {
      return true;
    } else if (rhs.rank() == Rank::ACE) {
      return false;
    }
    return lhs.rank() < rhs.rank();
  }
  return lhs.suit() < rhs.suit();
}

class Deck {
public:
  Deck() : next_(0) {
    for (Suit suit : {Suit::SPADES, Suit::CLUBS, Suit::HEARTS, Suit::DIAMONDS}) {
      for (Rank rank : {Rank::ACE, Rank::TWO, Rank::THREE, Rank::FOUR,
                        Rank::FIVE, Rank::SIX, Rank::SEVEN, Rank::EIGHT,
			Rank::NINE, Rank::TEN, Rank::JACK, Rank::QUEEN,
			Rank::KING}) {
        Card card;
        card.set_rank(rank);
        card.set_suit(suit);
        cards_.push_back(card);
      }
    }
  }
  template <typename RNG>
  void Shuffle(RNG& rng) {
    std::shuffle(cards_.begin(), cards_.end(), rng);
    next_ = 0;
  }
  void Sort(std::function<bool(Card,Card)> sort_fn) {
    std::sort(cards_.begin(), cards_.end(), sort_fn);
    next_ = 0;
  }
  const std::vector<Card>& Cards() const { return cards_; }
  Card DealCard() { return cards_[next_++]; }
private:
  std::vector<Card> cards_;
  int next_;
};

std::wostream& operator<<(std::wostream& os, const Deck& deck);

#endif // CARDS_H
