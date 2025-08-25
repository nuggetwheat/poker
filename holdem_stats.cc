#include "holdem_stats.h"

#include <functional>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <sstream>
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
  pocket_aces_ = HoleHand(Rank::ACE, Rank::ACE, HandType::ONE_PAIR);
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

  // Initialize river hand win count vector
  // Maximum sort code is straight flush (enum value 9) and all aces
  // (enum value 14)
  hand.set_type(HandType::STRAIGHT_FLUSH);
  hand.clear_rank();
  for (int i=0; i<5; i++) {
    hand.add_rank(Rank::ACE);
  }
  hand.set_sort_code(HandToSortCode(hand));
  river_hand_win_count_ = std::vector<int32_t>(hand.sort_code()+1, 0);

  // Initialize hole hand appearence and win vectors
  hole_hand_appearance_ = std::vector<int32_t>(hand_index_.size(), 0);
  hole_hand_win_ = std::vector<int32_t>(hand_index_.size(), 0);
}

void Statistics::NewGame(const poker::Table& table,
                         std::vector<Player>& players) {
  table_ = &table;
  players_.resize(players.size());
  for (int i=0; i<players.size(); i++) {
    players_[i] = &players[i];
  }
  total_games_++;
}

void Statistics::Collect(Round round) {
  switch (round) {
  case Round::INITIAL:
    for (Player* player : players_) {
      player->set_preflop_hand(HoleHand(player->cards()));
    }
    break;
  case Round::RIVER:
    {
      HandEvaluator he;
      for (Player* player : players_) {
        he.Reset(table_->community_cards());
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
        for (Player* player : players_) {
          hole_hand_appearance_[hand_index_[player->preflop_hand()]]++;
        }
        hole_hand_win_[hand_index_[players_[0]->preflop_hand()]]++;
        river_hand_win_count_[players_[0]->river_hand().sort_code()]++;
      }
    }
    break;
  default:
    break;
  }
}

namespace {
struct WinStatsT {
  int index{};
  Hand hand{};
  int hand_wins{};
  double win_percentage{};
};
std::string FlushSuffix(Hand& hand) {
  if (hand.type() == HandType::FLUSH || hand.type() == HandType::STRAIGHT_FLUSH)
    return "s";
  return "";
}
void ColumnStripeOrderApply(int total_size, int column_size, std::function<void(int,int,int,int)> func) {
  int stripe_size = std::ceil((double)total_size / (double)column_size);
  for (int i=0; i<stripe_size; i++) {
    for (int j=0; j<column_size; j++) {
      func(i, j, stripe_size, column_size);
    }
  }
}
}

void Statistics::Display(std::ostream& os) {
  std::vector<WinStatsT> win_stats(hand_index_.size());
  for (auto const& [hand, index] : hand_index_) {
    win_stats[index].index = index;
    win_stats[index].hand = hand;
    win_stats[index].hand_wins = 0;
  }
  if (args_.stats_hole_cards) {
    std::vector<std::vector<float>> win_percentage(hand_index_.size());
    for (int i=0; i<hand_index_.size(); i++) {
      win_percentage[i] = std::vector<float>(hand_index_.size());
    }
    for (int i=0; i<hand_index_.size(); i++) {
      for (int j=0; j<hand_index_.size(); j++) {
        if (beat_matrix_[i][j] > beat_matrix_[j][i]) {
          win_stats[i].hand_wins++;
        }
        if ((beat_matrix_[i][j] + beat_matrix_[j][i]) == 0) {
          win_percentage[i][j] = -1.0;
        } else if (i == j) {
          win_percentage[i][j] = 0.0;
        } else {
          win_percentage[i][j] = 100.0 * beat_matrix_[i][j] / (beat_matrix_[i][j] + beat_matrix_[j][i]);
        }
      }
    }
    std::sort(win_stats.begin(), win_stats.end(),
              [](const WinStatsT& lhs, const WinStatsT& rhs) {
                return lhs.hand_wins > rhs.hand_wins;
              });

    auto output_wins_fn = [&](int i, int j, int stripe_size, int column_size) {
      if (j>0) {
        os << ",";
      }
      int offset = i + (j * stripe_size);
      if (offset < win_stats.size()) {
        WinStatsT& stats = win_stats[offset];
        os << HoleHandToString(stats.hand) << "," << stats.hand_wins;
      } else {
        os << ",";
      }
      if (j == column_size-1) {
        os << "\n";
      }
    };
    ColumnStripeOrderApply(hand_index_.size(), 10, output_wins_fn);

    //
    // Win percentage
    //
    os << std::fixed << std::setprecision(2);
    for (WinStatsT& stats : win_stats) {
      os << HoleHandToString(stats.hand) << std::endl;
      auto output_win_pct_fn = [&](int i, int j, int stripe_size, int column_size) {
        if (j>0) {
          os << ",";
        }
        int offset = i + (j * stripe_size);
        if (offset < win_stats.size()) {
          os << HoleHandToString(win_stats[offset].hand) << ","
             << win_percentage[stats.index][win_stats[offset].index];
        }
        if (j == column_size - 1) {
          os << "\n";
        }
      };
      ColumnStripeOrderApply(hand_index_.size(), 10, output_win_pct_fn);
    }
  }
  if (args_.stats_winning_hand) {
    if (total_games_ != args_.iterations) {
      std::cerr << "total_games (" << total_games_ << ") != iterations (" << args_.iterations << ")\n";
      exit(1);
    }
    std::vector<int> winning_hand_type_count(10, 0);
    int median = args_.iterations / 2;
    int count = 0;
    Hand hand;
    uint32_t median_offset = 0;
    for (uint32_t i=0; i<river_hand_win_count_.size(); i++) {
      if (river_hand_win_count_[i] == 0)
        continue;
      hand = SortCodeToHand(i);
      winning_hand_type_count[static_cast<int>(hand.type())] += river_hand_win_count_[i];
      count += river_hand_win_count_[i];
      if (median_offset == 0 && count >= median) {
        median_offset = i;
      }
    }

    //
    // Hole Cards With Percentage at Showdown
    //
    os << "Hole Cards Win Percentage at Showdown (" << args_.players << " players)\n";
    os << std::fixed << std::setprecision(2);
    for (WinStatsT& stats : win_stats) {
      stats.win_percentage = (100.0 * hole_hand_win_[stats.index]) / (double)hole_hand_appearance_[stats.index];
    }
    std::sort(win_stats.begin(), win_stats.end(),
              [](const WinStatsT& lhs, const WinStatsT& rhs) {
                return lhs.win_percentage > rhs.win_percentage;
              });
    auto output_win_pct_showdown_fn = [&](int i, int j, int stripe_size, int column_size) {
      if (j>0) {
        os << ",";
      }
      int offset = i + (j * stripe_size);
      if (offset < win_stats.size()) {
        WinStatsT& stats = win_stats[offset];
        os << HoleHandToString(stats.hand) << "," << stats.win_percentage;
      } else {
        os << ",";
      }
      if (j == column_size-1) {
        os << "\n";
      }
    };
    ColumnStripeOrderApply(win_stats.size(), 10, output_win_pct_showdown_fn);

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
    os << std::flush;
  }
}

#if 0
    // Compute standard deviation
    uint64_t deviation = 0;
    for (uint32_t i=0; i<river_hand_win_count_.size(); i++) {
      if (river_hand_win_count_[i] == 0)
        continue;
      if (i < median_offset) {
        deviation += (median_offset - i) * river_hand_win_count_[i];
      } else if (i > median_offset) {
        deviation += (i - median_offset) * river_hand_win_count_[i];
      }
    }
    uint32_t stddev = deviation / args_.iterations;
    uint32_t minus_stddev_offset = median_offset - stddev;
    while (river_hand_win_count_[minus_stddev_offset] == 0) {
      minus_stddev_offset--;
    }
    uint32_t plus_stddev_offset = median_offset + stddev;
    while (river_hand_win_count_[plus_stddev_offset] == 0) {
      plus_stddev_offset++;
    }

    hand = SortCodeToHand(minus_stddev_offset);
    flush_suffix = FlushSuffix(hand);
    hand.set_type(HandType::HANDTYPE_UNSPECIFIED);
    os << "," << hand << flush_suffix;

    hand = SortCodeToHand(plus_stddev_offset);
    flush_suffix = FlushSuffix(hand);
    hand.set_type(HandType::HANDTYPE_UNSPECIFIED);
    os << "," << hand << flush_suffix << std::endl;
#endif

} // namespace poker::holdem
