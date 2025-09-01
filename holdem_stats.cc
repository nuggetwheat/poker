#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "cards.pb.h"
#include "holdem.h"
#include "holdem_stats.h"
#include "poker.h"
#include "poker.pb.h"
#include "poker_simulation_args.h"

namespace poker::holdem {

Statistics::Statistics(PokerSimulationArgs &args) : args_(args) {
  // Initialize hand index
  int offset{};
  Hand hand;

  for (Rank rank1 = Rank::ACE; rank1 > Rank::ONE;
       rank1 = OffsetRank(rank1, -1)) {
    hand = HoleHand(rank1, rank1, HandType::ONE_PAIR);
    hole_hand_index_[hand] = offset++;
    for (Rank rank2 = OffsetRank(rank1, -1); rank2 > Rank::ONE;
         rank2 = OffsetRank(rank2, -1)) {
      hand = HoleHand(rank1, rank2, HandType::FLUSH);
      hole_hand_index_[hand] = offset++;
      hand = HoleHand(rank1, rank2, HandType::HIGH_CARD);
      hole_hand_index_[hand] = offset++;
    }
  }
  assert(offset == kHoleHandCount);

  // Initialize hole hand appearance vector
  hole_hand_appearance_ = std::vector<int32_t>(kHoleHandCount, 0);

  // Sanity check sort code limit (maximum hand sort code + 1). Max sort code is
  // equivalent to straight flush (enum value 9) and all aces (enum value 14).
  {
    hand.set_type(HandType::STRAIGHT_FLUSH);
    hand.clear_rank();
    for (int i = 0; i < 5; i++) {
      hand.add_rank(Rank::ACE);
    }
    hand.set_sort_code(HandToSortCode(hand));
    assert(kSortCodeLimit == hand.sort_code() + 1);
  }

  // Initialize per-round statistics variables. We don't track statistics for
  // pre-flop which is why the index starts at kRoundFlop.
  for (int r = kRoundFlop; r < kRoundMax; r++) {
    RoundStats &stats = round_stats_[r];

    // Initialize beat matrix
    stats.beat_matrix = std::vector<std::vector<int32_t>>(kHoleHandCount);
    for (int i = 0; i < kHoleHandCount; i++) {
      stats.beat_matrix[i].resize(kHoleHandCount, 0);
    }

    // Initialize hand win count vector
    stats.hand_win_count = std::vector<int32_t>(kSortCodeLimit, 0);

    // Initialize hole hand win vector
    stats.hole_hand_wins = std::vector<int32_t>(kSortCodeLimit, 0);

    // Initialize win percentage matrix
    stats.win_percentage_matrix =
        std::vector<std::vector<float>>(kHoleHandCount);
    for (int i = 0; i < kHoleHandCount; i++) {
      stats.win_percentage_matrix[i].resize(kHoleHandCount, 0.0);
    }
  }
}

void Statistics::NewGame(const poker::Table &table,
                         std::vector<Player> &players) {
  table_ = &table;
  players_.resize(players.size());
  for (int i = 0; i < players.size(); i++) {
    players_[i] = &players[i];
  }
}

void Statistics::CollectRound(Round round) {
  HandEvaluator he;
  int round_index = static_cast<int>(round);
  RoundStats &round_stats = round_stats_[round_index];

  for (Player *player : players_) {
    he.Reset(table_->community_cards());
    player->set_hand(round_index, he.Evaluate(player->cards()));
  }
  std::vector<Player *> players = players_;
  std::sort(players.begin(), players.end(), [&](Player *lhs, Player *rhs) {
    return lhs->hand(round_index).sort_code() >
           rhs->hand(round_index).sort_code();
  });
  if (args_.stats_winning_hand) {
    round_stats.hand_win_count[players[0]->hand(round_index).sort_code()]++;
  }

  if (args_.stats_hole_cards) {
    int32_t prev_sort_code = 0;
    for (int i = 0; i < players.size() - 1; i++) {
      prev_sort_code = players[i]->hand(round_index).sort_code();
      for (int j = i + 1; j < players.size(); j++) {
        // Only increment win count for distinct hands and for each distinct
        // pair of hands, only increment win count once. On other words,
        // given the following four hands (AA, AA, KTo, KTo), we should not
        // increment [AA][AA] or [KTo][KTo] and we should only increment
        // [AA][KTo] once.
        if (players[j]->hand(round_index).sort_code() != prev_sort_code) {
          int win_index = hole_hand_index_[players[i]->hand(kRoundPreflop)];
          int lose_index = hole_hand_index_[players[j]->hand(kRoundPreflop)];
          round_stats.beat_matrix[win_index][lose_index]++;
          prev_sort_code = players[j]->hand(round_index).sort_code();
        }
      }
    }
    round_stats
        .hole_hand_wins[hole_hand_index_[players[0]->hand(kRoundPreflop)]]++;
  }
}

void Statistics::Collect(Round round) {
  switch (round) {
  case Round::PREFLOP:
    for (Player *player : players_) {
      player->set_hand(kRoundPreflop, HoleHand(player->cards()));
      hole_hand_appearance_[hole_hand_index_[player->hand(kRoundPreflop)]]++;
    }
    break;
  case Round::FLOP:
  case Round::TURN:
  case Round::RIVER:
    CollectRound(round);
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
std::string FlushSuffix(Hand &hand) {
  if (hand.type() == HandType::FLUSH || hand.type() == HandType::STRAIGHT_FLUSH)
    return "s";
  return "";
}
void HoleHandStripeOrderApply(
    std::ostream &os, std::function<void(std::ostream &, int, int, int)> func) {
  constexpr int kColumnCount = 10;
  int stripe_size =
      std::ceil((double)Statistics::kHoleHandCount / (double)kColumnCount);
  for (int i = 0; i < stripe_size; i++) {
    for (int j = 0; j < kColumnCount; j++) {
      if (j > 0) {
        os << ",";
      }
      func(os, i, j, stripe_size);
      if (j == kColumnCount - 1) {
        os << "\n";
      }
    }
  }
}
} // namespace

void Statistics::Display() {
  std::filesystem::path output_file;
  std::ofstream fout;
  std::vector<WinStatsT> win_stats(kHoleHandCount);
  if (args_.stats_hole_cards) {
    for (int r = kRoundFlop; r < kRoundMax; r++) {
      for (auto const &[hand, index] : hole_hand_index_) {
        win_stats[index].index = index;
        win_stats[index].hand = hand;
        win_stats[index].hand_wins = 0;
      }
      RoundStats &round_stats = round_stats_[r];
      for (int i = 0; i < kHoleHandCount; i++) {
        for (int j = 0; j < kHoleHandCount; j++) {
          if (round_stats.beat_matrix[i][j] > round_stats.beat_matrix[j][i]) {
            win_stats[i].hand_wins++;
          }
          if ((round_stats.beat_matrix[i][j] + round_stats.beat_matrix[j][i]) ==
              0) {
            round_stats.win_percentage_matrix[i][j] = -1.0;
          } else if (i == j) {
            round_stats.win_percentage_matrix[i][j] = 0.0;
          } else {
            round_stats.win_percentage_matrix[i][j] =
                100.0 * round_stats.beat_matrix[i][j] /
                (round_stats.beat_matrix[i][j] + round_stats.beat_matrix[j][i]);
          }
        }
      }
      std::sort(win_stats.begin(), win_stats.end(),
                [](const WinStatsT &lhs, const WinStatsT &rhs) {
                  return lhs.hand_wins > rhs.hand_wins;
                });

      output_file = std::filesystem::path(args_.output_dir);
      std::stringstream ss;
      ss << "hole-cards-win-rank-" << kRound[r] << ".csv";
      output_file.append(ss.str());
      fout = std::ofstream(output_file);
      auto output_wins_fn = [&](std::ostream &os, int i, int j,
                                int stripe_size) {
        int offset = i + (j * stripe_size);
        if (offset < kHoleHandCount) {
          WinStatsT &stats = win_stats[offset];
          os << HoleHandToString(stats.hand) << "," << stats.hand_wins;
        } else {
          os << ",";
        }
      };
      HoleHandStripeOrderApply(fout, output_wins_fn);
      fout.close();
    }

    //
    // Win percentage
    //
    output_file = std::filesystem::path(args_.output_dir);
    output_file.append("hole-cards-vs-other-hole-cards-win-pct.csv");
    fout = std::ofstream(output_file);
    fout << std::fixed << std::setprecision(2);
    for (WinStatsT &stats : win_stats) {
      fout << HoleHandToString(stats.hand) << std::endl;
      // TODO: just set this to win_stats;
      std::vector<WinStatsT> win_stats_sorted(kHoleHandCount);
      for (auto const &[hand, index] : hole_hand_index_) {
        win_stats_sorted[index].index = index;
        win_stats_sorted[index].hand = hand;
        win_stats_sorted[index].hand_wins = 0;
        win_stats_sorted[index].win_percentage =
            round_stats_[kRoundRiver].win_percentage_matrix[stats.index][index];
      }
      std::sort(win_stats_sorted.begin(), win_stats_sorted.end(),
                [](const WinStatsT &lhs, const WinStatsT &rhs) {
                  return lhs.win_percentage < rhs.win_percentage;
                });

      auto output_win_pct_fn = [&](std::ostream &os, int i, int j,
                                   int stripe_size) {
        int offset = i + (j * stripe_size);
        if (offset < win_stats_sorted.size()) {
          os << HoleHandToString(win_stats_sorted[offset].hand) << ","
             << win_stats_sorted[offset].win_percentage;
        } else {
          os << ",";
        }
      };
      HoleHandStripeOrderApply(fout, output_win_pct_fn);
    }
    fout.close();

    //
    // Hole Cards With Percentage at Showdown
    //
    std::stringstream ss;
    ss << "hole-cards-win-pct-";
    ss << std::setw(2) << std::setfill('0') << args_.players << "-players.csv";
    output_file = std::filesystem::path(args_.output_dir);
    output_file.append(ss.str());
    fout = std::ofstream(output_file);
    fout << std::fixed << std::setprecision(2);
    for (WinStatsT &stats : win_stats) {
      stats.win_percentage =
          (100.0 * round_stats_[kRoundRiver].hole_hand_wins[stats.index]) /
          (double)hole_hand_appearance_[stats.index];
    }
    std::sort(win_stats.begin(), win_stats.end(),
              [](const WinStatsT &lhs, const WinStatsT &rhs) {
                return lhs.win_percentage > rhs.win_percentage;
              });
    auto output_win_pct_showdown_fn = [&](std::ostream &os, int i, int j,
                                          int stripe_size) {
      int offset = i + (j * stripe_size);
      if (offset < kHoleHandCount) {
        WinStatsT &stats = win_stats[offset];
        fout << HoleHandToString(stats.hand) << "," << stats.win_percentage;
      } else {
        fout << ",";
      }
    };
    HoleHandStripeOrderApply(fout, output_win_pct_showdown_fn);
    fout.close();
  }
  if (args_.stats_winning_hand) {
    Hand hand;
    int median = args_.iterations / 2;
    HandTypeWinStats hand_type_stats[kRoundMax];
    for (int r = 1; r < kRoundMax; r++) {
      hand_type_stats[r].wins = std::vector<int>(kHandTypeMax, 0);
    }

    for (uint32_t sort_code = 0; sort_code < kSortCodeLimit; sort_code++) {
      if (round_stats_[kRoundFlop].hand_win_count[sort_code] == 0 &&
          round_stats_[kRoundTurn].hand_win_count[sort_code] == 0 &&
          round_stats_[kRoundRiver].hand_win_count[sort_code] == 0)
        continue;
      hand = SortCodeToHand(sort_code);
      for (int r = kRoundFlop; r < kRoundMax; r++) {
        RoundStats &round_stats = round_stats_[r];
        if (round_stats.hand_win_count[sort_code] != 0) {
          hand_type_stats[r].wins[static_cast<int>(hand.type())] +=
              round_stats.hand_win_count[sort_code];
          hand_type_stats[r].count += round_stats.hand_win_count[sort_code];
          if (hand_type_stats[r].median_offset == 0 &&
              hand_type_stats[r].count >= median) {
            hand_type_stats[r].median_offset = sort_code;
          }
        }
      }
    }

    std::string flush_suffix;

    // Winning hand distribution
    for (int r = kRoundFlop; r < kRoundMax; r++) {
      output_file = std::filesystem::path(args_.output_dir);
      std::stringstream ss;
      ss << "winning-hand-distribution-" << kRound[r] << ".csv";
      output_file.append(ss.str());
      if (args_.append_output) {
        fout = std::ofstream(output_file, std::ios::app);
      } else {
        fout = std::ofstream(output_file);
        fout << "Players,High Card,One Pair,Two Pair,Three Of A "
                "Kind,Straight,Flush,Full House,Four Of A Kind,Straight "
                "Flush,Median\n";
      }
      fout << table_->players().size() << ",";
      fout << std::fixed << std::setprecision(3);
      for (int i = 1; i < 10; i++) {
        double percentage =
            (hand_type_stats[r].wins[i] * 100.0) / args_.iterations;
        fout << percentage << ",";
      }
      hand = SortCodeToHand(hand_type_stats[r].median_offset);
      flush_suffix = FlushSuffix(hand);
      hand.set_type(HandType::HANDTYPE_UNSPECIFIED);
      fout << hand << flush_suffix << std::endl;
      fout.close();
    }
    fout << std::flush;
  }
}

#if 0
    // Compute standard deviation
    uint64_t deviation = 0;
    for (uint32_t i=0; i<hand_win_count_river_.size(); i++) {
      if (hand_win_count_river_[i] == 0)
        continue;
      if (i < median_offset_river) {
        deviation += (median_offset_river - i) * hand_win_count_river_[i];
      } else if (i > median_offset_river) {
        deviation += (i - median_offset_river) * hand_win_count_river_[i];
      }
    }
    uint32_t stddev = deviation / args_.iterations;
    uint32_t minus_stddev_offset = median_offset_river - stddev;
    while (hand_win_count_river_[minus_stddev_offset] == 0) {
      minus_stddev_offset--;
    }
    uint32_t plus_stddev_offset = median_offset_river + stddev;
    while (hand_win_count_river_[plus_stddev_offset] == 0) {
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
