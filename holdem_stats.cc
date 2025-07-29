#include "holdem_stats.h"

#include <iostream>

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

void Statistics::NewGame(std::vector<Player>& players,
                         std::vector<Card>& community_cards) {
  players_ = &players;
  community_cards_ = &community_cards;
  player_info_.resize(players.size());
  for (int i=0; i<players.size(); i++) {
    player_info_[i].player = &players[i];
  }
}

void Statistics::Collect(Round round) {
  switch (round) {
  case Round::INITIAL:
    if (args_.stats_hole_cards) {
      for (auto& info : player_info_) {
        info.hole_hand = HoleHand(info.player->cards(0), info.player->cards(1));
      }
    }
    break;
  case Round::RIVER:
    {
      HandEvaluator he;
      he.Reset(*community_cards_);
      std::vector<Card> cards(2);
      for (auto& info : player_info_) {
        cards[0] = info.player->cards(0);
        cards[1] = info.player->cards(1);
        info.hand = he.Evaluate(cards);
      }
      std::sort(player_info_.begin(), player_info_.end(),
                [](const PlayerInfoT& lhs, const PlayerInfoT& rhs) {
                  return lhs.hand.sort_code() > rhs.hand.sort_code();
                });
      if (args_.stats_hole_cards) {
        for (int i=0; i<player_info_.size()-1; i++) {
          for (int j=i+1; j<player_info_.size(); j++) {
            if (player_info_[i].hand.sort_code() > player_info_[j].hand.sort_code()) {
              int win_index = hand_index_[player_info_[i].hole_hand];
              int lose_index = hand_index_[player_info_[j].hole_hand];
              beat_matrix_[win_index][lose_index]++;
            }
          }
        }
      }
      if (args_.stats_winning_hand) {
        hand_win_count_[player_info_[0].hand.sort_code()]++;
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
}

void Statistics::Display() {
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
      std::cout << stats.hand.rank(0) << stats.hand.rank(1);
      if (stats.hand.type() == HandType::FLUSH) {
        std::cout << "s,";
      } else if (stats.hand.type() == HandType::HIGH_CARD) {
        std::cout << "o,";
      } else {
        std::cout << ",";
      }
      std::cout << stats.wins << std::endl;
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
    std::cout << "[winning hand]" << std::endl;
    std::cout << "+sigma: " << SortCodeToHand(plus_stddev_offset) << std::endl;
    std::cout << "median: " << SortCodeToHand(median_offset) << std::endl;
    std::cout << "-sigma: " << SortCodeToHand(minus_stddev_offset) << std::endl;
    std::cout << std::fixed << std::setprecision(3);
    for (int i=1; i<10; i++) {
      double percentage = (winning_hand_type_count[i] * 100.0) / args_.iterations;
      std::cout << std::setw(6) << percentage << "% " << static_cast<poker::HandType>(i) << std::endl;
    }
  }
}

} // namespace poker::holdem
