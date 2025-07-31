#include "holdem.h"

#include "cards.pb.h"
#include "poker.h"

namespace poker::holdem {

Hand HoleHand(const std::vector<Card>& cards) {
  Hand hand;
  if (cards[0].rank() == cards[1].rank()) {
    hand.set_type(HandType::ONE_PAIR);
  } else if (cards[0].suit() == cards[1].suit()) {
    hand.set_type(HandType::FLUSH);
  } else {
    hand.set_type(HandType::HIGH_CARD);
  }
  if (cards[0].rank() > cards[1].rank()) {
    hand.add_rank(cards[0].rank());
    hand.add_rank(cards[1].rank());
  } else {
    hand.add_rank(cards[1].rank());
    hand.add_rank(cards[0].rank());
  }
  hand.set_sort_code(HandToSortCode(hand));
  return hand;
}

Hand HoleHand(Rank rank1, Rank rank2, HandType hand_type) {
  Hand hand;
  hand.set_type(hand_type);
  if (rank1 > rank2) {
    hand.add_rank(rank1);
    hand.add_rank(rank2);
  } else {
    hand.add_rank(rank2);
    hand.add_rank(rank1);
  }
  hand.set_sort_code(HandToSortCode(hand));
  return hand;
}

} // namespace poker::holdem
