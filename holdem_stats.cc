#include "holdem_stats.h"

#include <iostream>
#include <vector>

#include "cards.pb.h"
#include "holdem.h"
#include "poker.h"
#include "poker.pb.h"
#include "poker_simulation_args.h"

namespace poker::holdem {

Statistics::Statistics(PokerSimulationArgs& args) : args_(args) {
  // Initialize hand index
  int offset{};
  Hand hand;
  for (Rank rank1 = Rank::ACE; rank1 > Rank::ONE;
       rank1 = OffsetRank(rank1, -1)) {
    hand = HoleHand(rank1, rank1, HandType::ONE_PAIR);
    hand_index_[hand] = offset++;
    for (Rank rank2 = OffsetRank(rank1, -1); rank2 > Rank::ONE;
         rank2 = OffsetRank(rank2, -1)) {
      hand = HoleHand(rank1, rank2, HandType::FLUSH);
      hand_index_[hand] = offset++;
      hand = HoleHand(rank1, rank2, HandType::HIGH_CARD);
      hand_index_[hand] = offset++;
    }
  }

  // Initialize beat matrix
  beat_matrix_ = std::make_unique<std::vector<int>[]>(offset);
  for (int i=0; i<offset; i++) {
    beat_matrix_[i].resize(offset, 0);
  }

  // Initialize hand win count vector
  // Maximum sort code is straight flush (enum value 9) and all aces
  // (enum value 14)
  hand.set_type(HandType::STRAIGHT_FLUSH);
  hand.clear_rank();
  for (int i=0; i<5; i++) {
    hand.add_rank(Rank::ACE);
  }
  hand.set_sort_code(HandToSortCode(hand));
  hand_win_count_ = std::vector<int32_t>(hand.sort_code()+1, 0);
}

void Statistics::NewGame(const poker::Table& table,
                         std::vector<Player>& players) {
  table_ = &table;
  players_.resize(players.size());
  for (int i=0; i<players.size(); i++) {
    players_[i] = &players[i];
  }
}

void Statistics::Collect(Round round) {
  switch (round) {
  case Round::INITIAL:
    if (args_.stats_hole_cards) {
      for (Player* player : players_) {
        player->set_preflop_hand(HoleHand(player->cards()));
      }
    }
    break;
  case Round::RIVER:
    {
      HandEvaluator he;
      he.Reset(table_->community_cards());
      for (Player* player : players_) {
        player->set_river_hand(he.Evaluate(player->cards()));
      }
      std::sort(players_.begin(), players_.end(),
                [](Player* lhs, Player* rhs) {
                  return lhs->river_hand().sort_code() > rhs->river_hand().sort_code();
                });

      int32_t prev_sort_code = 0;
      if (args_.stats_hole_cards) {
        for (int i=0; i<players_.size()-1; i++) {
          prev_sort_code = players_[i]->river_hand().sort_code();
          for (int j=i+1; j<players_.size(); j++) {
            // Only increment win count for distinct hands and for each distinct
            // pair of hands, only increment win count once. On other words,
            // given the following four hands (AA, AA, KTo, KTo), we should not
            // increment [AA][AA] or [KTo][KTo] and we should only increment
            // [AA][KTo] once.
            if (players_[j]->river_hand().sort_code() != prev_sort_code) {
              int win_index = hand_index_[players_[i]->preflop_hand()];
              int lose_index = hand_index_[players_[j]->preflop_hand()];
              beat_matrix_[win_index][lose_index]++;
            }
            prev_sort_code = players_[i]->river_hand().sort_code();
          }
        }
      }
      if (args_.stats_winning_hand) {
        hand_win_count_[players_[0]->river_hand().sort_code()]++;
      }
    }
    break;
  default:
    break;
  }
}

namespace {
struct WinStatsT {
  int index;
  Hand hand;
  int wins;
};
std::string FlushSuffix(Hand& hand) {
  if (hand.type() == HandType::FLUSH || hand.type() == HandType::STRAIGHT_FLUSH)
    return "s";
  return "";
}
}

void Statistics::Display(std::ostream& os) {
  if (args_.stats_hole_cards) {
    std::vector<WinStatsT> win_stats(hand_index_.size());
    for (auto const& [hand, index] : hand_index_) {
      win_stats[index].index = index;
      win_stats[index].hand = hand;
      win_stats[index].wins = 0;
    }
    for (int i=0; i<hand_index_.size(); i++) {
      for (int j=0; j<hand_index_.size(); j++) {
        if (beat_matrix_[i][j] > beat_matrix_[j][i]) {
          win_stats[i].wins++;
        }
      }
    }
    std::sort(win_stats.begin(), win_stats.end(),
              [](const WinStatsT& lhs, const WinStatsT& rhs) {
                return lhs.wins > rhs.wins;
              });
    for (const WinStatsT& stats : win_stats) {
      os << stats.hand.rank(0) << stats.hand.rank(1);
      if (stats.hand.type() == HandType::FLUSH) {
        os << "s,";
      } else if (stats.hand.type() == HandType::HIGH_CARD) {
        os << "o,";
      } else {
        os << ",";
      }
      os << stats.wins << std::endl;
    }
  }
  if (args_.stats_winning_hand) {
    std::vector<int> winning_hand_type_count(10, 0);
    int median = args_.iterations / 2;
    int count = 0;
    Hand hand;
    uint32_t median_offset = 0;
    for (uint32_t i=0; i<hand_win_count_.size(); i++) {
      if (hand_win_count_[i] == 0)
        continue;
      hand = SortCodeToHand(i);
      winning_hand_type_count[static_cast<int>(hand.type())] += hand_win_count_[i];
      if (median_offset == 0 && count + hand_win_count_[i] >= median) {
        median_offset = i;
      }
      count += hand_win_count_[i];
    }

#if 0
    // Compute standard deviation
    uint64_t deviation = 0;
    for (uint32_t i=0; i<hand_win_count_.size(); i++) {
      if (hand_win_count_[i] == 0)
        continue;
      if (i < median_offset) {
        deviation += (median_offset - i) * hand_win_count_[i];
      } else if (i > median_offset) {
        deviation += (i - median_offset) * hand_win_count_[i];
      }
    }
    uint32_t stddev = deviation / args_.iterations;
    uint32_t minus_stddev_offset = median_offset - stddev;
    while (hand_win_count_[minus_stddev_offset] == 0) {
      minus_stddev_offset--;
    }
    uint32_t plus_stddev_offset = median_offset + stddev;
    while (hand_win_count_[plus_stddev_offset] == 0) {
      plus_stddev_offset++;
    }
#endif

    if (!args_.append_output) {
      os << "Players,High Card,One Pair,Two Pair,Three Of A Kind,Straight,Flush,Full House,Four Of A Kind,Straight Flush,Median\n";
    }
    std::string flush_suffix;
    os << table_->players().size() << ",";

    os << std::fixed << std::setprecision(3);
    for (int i=1; i<10; i++) {
      double percentage = (winning_hand_type_count[i] * 100.0) / args_.iterations;
      os << percentage << "%,";
    }
    hand = SortCodeToHand(median_offset);
    flush_suffix = FlushSuffix(hand);
    hand.set_type(HandType::HANDTYPE_UNSPECIFIED);
    os << hand << flush_suffix << std::endl;

#if 0
    hand = SortCodeToHand(minus_stddev_offset);
    flush_suffix = FlushSuffix(hand);
    hand.set_type(HandType::HANDTYPE_UNSPECIFIED);
    os << "," << hand << flush_suffix;

    hand = SortCodeToHand(plus_stddev_offset);
    flush_suffix = FlushSuffix(hand);
    hand.set_type(HandType::HANDTYPE_UNSPECIFIED);
    os << "," << hand << flush_suffix << std::endl;
#endif

    os << std::flush;
  }
}

} // namespace poker::holdem
