#include "poker.h"

#include "poker.pb.h"

namespace poker {

std::ostream& operator<<(std::ostream& os, const Hand& hand) {
  switch (hand.type()) {
  case HandType::STRAIGHT_FLUSH:
    os << "straight-flush ";
    break;
  case HandType::FOUR_OF_A_KIND:
    os << "four-of-a-kind ";
    break;
  case HandType::FULL_HOUSE:
    os << "full-house ";
    break;
  case HandType::FLUSH:
    os << "flush ";
    break;
  case HandType::STRAIGHT:
    os << "straight ";
    break;
  case HandType::THREE_OF_A_KIND:
    os << "three-of-a-kind ";
    break;
  case HandType::TWO_PAIR:
    os << "two-pair ";
    break;
  case HandType::ONE_PAIR:
    os << "one-pair ";
    break;
  case HandType::HIGH_CARD:
    os << "high-card ";
    break;
  default:
    os << "?";
  }
  for (int rank : hand.rank()) {
    os << static_cast<Rank>(rank);
  }
  return os;
}

void HandEvaluator::Reset(const std::vector<Card>& community) {
  // Clear state
  memset(rank_reset_limit_, 0, sizeof(rank_reset_limit_));
  memset(suit_reset_limit_, 0, sizeof(suit_reset_limit_));
  for (int i=0; i<MAX_RANK; i++) {
    rank_[i].clear();
  }
  for (int i=0; i<MAX_SUIT; i++) {
    suit_[i].clear();
  }

  // Add community cards
  for (Card card : community) {
    int ranki = static_cast<int>(card.rank());
    rank_[ranki].push_back(card.suit());
    rank_reset_limit_[ranki] = rank_[ranki].size();
    int suiti = static_cast<int>(card.suit());
    suit_[suiti].push_back(card.rank());
    suit_reset_limit_[suiti] = suit_[suiti].size();
  }
}
  
Hand HandEvaluator::Evaluate(const std::vector<Card>& hole) {
  for (Card card : hole) {
    rank_[static_cast<int>(card.rank())].push_back(card.suit());
    suit_[static_cast<int>(card.suit())].push_back(card.rank());
  }
  // Copy ACE to ONE in rank array for the purpose of straight calculation
  rank_[1] = rank_[static_cast<int>(Rank::ACE)];

  // Best hand tracking
  HandType best_hand_type = HandType::HANDTYPE_UNSPECIFIED;
  auto set_best_hand = [&best_hand_type](HandType hand_type) {
    if (hand_type > best_hand_type) best_hand_type = hand_type;
  };

  int rank_count[MAX_RANK];
  memset(rank_count, 0, sizeof(rank_count));

  Rank straight_high = Rank::RANK_UNSPECIFIED;
  int straight_length = 0;
  Rank four_of_a_kind = Rank::RANK_UNSPECIFIED;
  Rank trips = Rank::RANK_UNSPECIFIED;
  Rank pair_high = Rank::RANK_UNSPECIFIED;
  Rank pair_low = Rank::RANK_UNSPECIFIED;
  
  for (int i=static_cast<int>(Rank::ACE); i>0; i--) {
    rank_count[i] = rank_[i].size();

    // Straight
    if (best_hand_type < HandType::STRAIGHT) {
      if (!rank_[i].empty()) {
        if (straight_high == Rank::RANK_UNSPECIFIED) {
          straight_high = static_cast<Rank>(i);
          straight_length = 1;
        } else {
          straight_length++;
          if (straight_length == 5) {
            set_best_hand(HandType::STRAIGHT);
          }
        }
      } else {
        straight_high = Rank::RANK_UNSPECIFIED;
        straight_length = 0;
      }
    }

    // Four-of-a-kind
    if (rank_[i].size() == 4 && best_hand_type < HandType::FOUR_OF_A_KIND) {
      set_best_hand(HandType::FOUR_OF_A_KIND);
      four_of_a_kind = static_cast<Rank>(i);
    }

    // Three-of-a-kind
    if (rank_[i].size() == 3) {
      if (trips == Rank::RANK_UNSPECIFIED) {
        if (pair_high != Rank::RANK_UNSPECIFIED) {
          set_best_hand(HandType::FULL_HOUSE);
        } else {
          set_best_hand(HandType::THREE_OF_A_KIND);
        }
        trips = static_cast<Rank>(i);
      } else {
        if (pair_high == Rank::RANK_UNSPECIFIED) {
          set_best_hand(HandType::ONE_PAIR);
          pair_high = static_cast<Rank>(i);
        } else if (pair_low == Rank::RANK_UNSPECIFIED) {
          set_best_hand(HandType::TWO_PAIR);
          pair_low = static_cast<Rank>(i);
        }
      }
    }

    // Pair
    if (rank_[i].size() == 2) {
      if (pair_high == Rank::RANK_UNSPECIFIED) {
        set_best_hand(HandType::ONE_PAIR);
        pair_high = static_cast<Rank>(i);
      } else if (pair_low == Rank::RANK_UNSPECIFIED) {
        set_best_hand(HandType::TWO_PAIR);
        pair_low = static_cast<Rank>(i);
      }
      if (trips != Rank::RANK_UNSPECIFIED) {
        set_best_hand(HandType::FULL_HOUSE);
      }
    }
  }

  Hand hand;

  // Check for flush
  for (int i=0; i<MAX_SUIT; i++) {
    if (suit_[i].size() >= 5) {
      std::vector<Rank> flush = suit_[i];
      std::sort(flush.begin(), flush.end(), [](Rank lhs, Rank rhs) {
        return lhs > rhs;
      });
      // check for straight flush
      if (straight_high != Rank::RANK_UNSPECIFIED) {
        Rank straight_flush_high = Rank::RANK_UNSPECIFIED;
        int prev_rank = 0;
        int run_length = 0;
        for (Rank rank : flush) {
          if (straight_flush_high == Rank::RANK_UNSPECIFIED ||
              static_cast<int>(rank) != prev_rank - 1) {
            if (rank <= Rank::FOUR)
              break;
            straight_flush_high = rank;
            run_length = 1;
          } else {
            run_length++;
            if (run_length == 5 ||
                (run_length == 4 && rank == Rank::TWO && flush[0] == Rank::ACE)) {
              set_best_hand(HandType::STRAIGHT_FLUSH);
              hand.set_type(HandType::STRAIGHT_FLUSH);
              rank = straight_flush_high;
              for (int i=0; i<4; i++) {
                hand.add_rank(rank);
                rank = static_cast<Rank>(static_cast<int>(rank)-1);
              }
              if (rank == Rank::ONE) {
                hand.add_rank(Rank::ACE);
              } else {
                hand.add_rank(rank);
              }
              // Assuming no multiple straight flushes
              break;
            }
          }
          prev_rank = static_cast<int>(rank);
        }
      }
      if (best_hand_type != HandType::STRAIGHT_FLUSH) {
        set_best_hand(HandType::FLUSH);
        hand.set_type(HandType::FLUSH);
        for (int i=0; i<5; i++) {
          hand.add_rank(flush[i]);
        }
        // Assuming no multiple flushes
        break;
      }
    }
  }

  if (best_hand_type != HandType::FLUSH &&
      best_hand_type != HandType::STRAIGHT_FLUSH) {
    int ki{}; // kicker index
    if (best_hand_type == HandType::FOUR_OF_A_KIND) {
      hand.set_type(HandType::FOUR_OF_A_KIND);
      for (; ki < 4; ki++) {
        hand.add_rank(four_of_a_kind);
      }
      rank_count[static_cast<int>(four_of_a_kind)] -= 4;
    } else if (best_hand_type == HandType::FULL_HOUSE) {
      hand.set_type(HandType::FULL_HOUSE);
      for (; ki < 3; ki++) {
        hand.add_rank(trips);
      }
      for (; ki < 5; ki++) {
        hand.add_rank(pair_high);
      }
    } else if (best_hand_type == HandType::STRAIGHT) {
      hand.set_type(HandType::STRAIGHT);
      std::cerr << "Straight to the " << straight_high << std::endl;
      Rank rank = straight_high;
      for (; ki<4; ki++) {
        hand.add_rank(rank);
        rank = static_cast<Rank>(static_cast<int>(rank)-1);
      }
      if (rank == Rank::ONE) {
        hand.add_rank(Rank::ACE);
      } else {
        hand.add_rank(rank);
      }
      ki++;
    } else if (best_hand_type == HandType::THREE_OF_A_KIND) {
      hand.set_type(HandType::THREE_OF_A_KIND);
      for (; ki < 3; ki++) {
        hand.add_rank(trips);
      }
      rank_count[static_cast<int>(trips)] -= 3;
    } else if (best_hand_type == HandType::TWO_PAIR) {
      hand.set_type(HandType::TWO_PAIR);
      for (; ki < 2; ki++) {
        hand.add_rank(pair_high);
      }
      for (; ki < 4; ki++) {
        hand.add_rank(pair_low);
      }
      rank_count[static_cast<int>(pair_high)] -= 2;
      rank_count[static_cast<int>(pair_low)] -= 2;
    } else if (best_hand_type == HandType::ONE_PAIR) {
      hand.set_type(HandType::ONE_PAIR);
      for (; ki < 2; ki++) {
        hand.add_rank(pair_high);
      }
      rank_count[static_cast<int>(pair_high)] -= 2;
    } else {
      hand.set_type(HandType::HIGH_CARD);
    }
    // Add the kickers
    for (int i=static_cast<int>(Rank::ACE); ki<5; i--) {
      while (rank_count[i] > 0) {
        hand.add_rank(static_cast<Rank>(i));
        if (++ki == 5)
          break;
        rank_count[i]--;
      }
    }
  }

  // Reset community counters
  for (Card card : hole) {
    int ranki = static_cast<int>(card.rank());
    rank_[ranki].resize(rank_reset_limit_[ranki]);
    int suiti = static_cast<int>(card.suit());
    suit_[suiti].resize(suit_reset_limit_[suiti]);
  }
  rank_[1].clear();

  return hand;
}  

} // namespace poker
