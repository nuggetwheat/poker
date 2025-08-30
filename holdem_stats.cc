#include "holdem_stats.h"

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
  hand_win_count_flop_ = std::vector<int32_t>(hand.sort_code()+1, 0);
  hand_win_count_turn_ = std::vector<int32_t>(hand.sort_code()+1, 0);
  hand_win_count_river_ = std::vector<int32_t>(hand.sort_code()+1, 0);

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
  std::vector<Player*> players = players_;
  HandEvaluator he;

  switch (round) {
  case Round::INITIAL:
    for (Player* player : players) {
      player->set_preflop_hand(HoleHand(player->cards()));
    }
    break;
  case Round::FLOP:
    if (args_.stats_winning_hand) {
      for (Player* player : players) {
        he.Reset(table_->community_cards());
        player->set_flop_hand(he.Evaluate(player->cards()));
      }
      std::sort(players.begin(), players.end(),
                [](Player* lhs, Player* rhs) {
                  return lhs->flop_hand().sort_code() > rhs->flop_hand().sort_code();
                });
      hand_win_count_flop_[players[0]->flop_hand().sort_code()]++;
    }
    break;
  case Round::TURN:
    if (args_.stats_winning_hand) {
      for (Player* player : players) {
        he.Reset(table_->community_cards());
        player->set_turn_hand(he.Evaluate(player->cards()));
      }
      std::sort(players.begin(), players.end(),
                [](Player* lhs, Player* rhs) {
                  return lhs->turn_hand().sort_code() > rhs->turn_hand().sort_code();
                });
      hand_win_count_turn_[players[0]->turn_hand().sort_code()]++;
    }
    break;
  case Round::RIVER:
    {
      for (Player* player : players) {
        he.Reset(table_->community_cards());
        player->set_river_hand(he.Evaluate(player->cards()));
      }
      std::sort(players.begin(), players.end(),
                [](Player* lhs, Player* rhs) {
                  return lhs->river_hand().sort_code() > rhs->river_hand().sort_code();
                });
      if (args_.stats_winning_hand) {
        hand_win_count_river_[players[0]->river_hand().sort_code()]++;
      }
      if (args_.stats_hole_cards) {
        int32_t prev_sort_code = 0;
        for (int i=0; i<players.size()-1; i++) {
          prev_sort_code = players[i]->river_hand().sort_code();
          for (int j=i+1; j<players.size(); j++) {
            // Only increment win count for distinct hands and for each distinct
            // pair of hands, only increment win count once. On other words,
            // given the following four hands (AA, AA, KTo, KTo), we should not
            // increment [AA][AA] or [KTo][KTo] and we should only increment
            // [AA][KTo] once.
            if (players[j]->river_hand().sort_code() != prev_sort_code) {
              int win_index = hand_index_[players[i]->preflop_hand()];
              int lose_index = hand_index_[players[j]->preflop_hand()];
              beat_matrix_[win_index][lose_index]++;
            }
            prev_sort_code = players[i]->river_hand().sort_code();
          }
        }
        for (Player* player : players) {
          hole_hand_appearance_[hand_index_[player->preflop_hand()]]++;
        }
        hole_hand_win_[hand_index_[players[0]->preflop_hand()]]++;
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

void Statistics::Display() {
  std::filesystem::path output_file;
  std::ofstream fout;
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

    output_file = std::filesystem::path(args_.output_dir);
    output_file.append("hole-cards-win-rank.csv");
    fout = std::ofstream(output_file);
    auto output_wins_fn = [&](int i, int j, int stripe_size, int column_size) {
      if (j>0) {
        fout << ",";
      }
      int offset = i + (j * stripe_size);
      if (offset < win_stats.size()) {
        WinStatsT& stats = win_stats[offset];
        fout << HoleHandToString(stats.hand) << "," << stats.hand_wins;
      } else {
        fout << ",";
      }
      if (j == column_size-1) {
        fout << "\n";
      }
    };
    ColumnStripeOrderApply(hand_index_.size(), 10, output_wins_fn);
    fout.close();

    //
    // Win percentage
    //
    output_file = std::filesystem::path(args_.output_dir);
    output_file.append("hole-cards-vs-other-hole-cards-win-pct.csv");
    fout = std::ofstream(output_file);
    fout << std::fixed << std::setprecision(2);
    for (WinStatsT& stats : win_stats) {
      fout << HoleHandToString(stats.hand) << std::endl;
      // TODO: just set this to win_stats;
      std::vector<WinStatsT> win_stats_sorted(hand_index_.size());
      for (auto const& [hand, index] : hand_index_) {
        win_stats_sorted[index].index = index;
        win_stats_sorted[index].hand = hand;
        win_stats_sorted[index].hand_wins = 0;
        win_stats_sorted[index].win_percentage = win_percentage[stats.index][index];
      }
      std::sort(win_stats_sorted.begin(), win_stats_sorted.end(),
                [](const WinStatsT& lhs, const WinStatsT& rhs) {
                  return lhs.win_percentage < rhs.win_percentage;
                });

      auto output_win_pct_fn = [&](int i, int j, int stripe_size, int column_size) {
        if (j>0) {
          fout << ",";
        }
        int offset = i + (j * stripe_size);
        if (offset < win_stats_sorted.size()) {
          fout << HoleHandToString(win_stats_sorted[offset].hand) << ","
             << win_stats_sorted[offset].win_percentage;
        }
        if (j == column_size - 1) {
          fout << "\n";
        }
      };
      ColumnStripeOrderApply(hand_index_.size(), 10, output_win_pct_fn);
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
    for (WinStatsT& stats : win_stats) {
      stats.win_percentage = (100.0 * hole_hand_win_[stats.index]) / (double)hole_hand_appearance_[stats.index];
    }
    std::sort(win_stats.begin(), win_stats.end(),
              [](const WinStatsT& lhs, const WinStatsT& rhs) {
                return lhs.win_percentage > rhs.win_percentage;
              });
    auto output_win_pct_showdown_fn = [&](int i, int j, int stripe_size, int column_size) {
      if (j>0) {
        fout << ",";
      }
      int offset = i + (j * stripe_size);
      if (offset < win_stats.size()) {
        WinStatsT& stats = win_stats[offset];
        fout << HoleHandToString(stats.hand) << "," << stats.win_percentage;
      } else {
        fout << ",";
      }
      if (j == column_size-1) {
        fout << "\n";
      }
    };
    ColumnStripeOrderApply(win_stats.size(), 10, output_win_pct_showdown_fn);
    fout.close();
  }
  if (args_.stats_winning_hand) {
    std::vector<int> winning_hand_type_flop(10, 0);
    std::vector<int> winning_hand_type_turn(10, 0);
    std::vector<int> winning_hand_type_river(10, 0);
    int median = args_.iterations / 2;
    int count_flop = 0;
    int count_turn = 0;
    int count_river = 0;
    Hand hand;
    uint32_t median_offset_flop = 0;
    uint32_t median_offset_turn = 0;
    uint32_t median_offset_river = 0;
    for (uint32_t i=0; i<hand_win_count_river_.size(); i++) {
      if (hand_win_count_flop_[i] != 0 || hand_win_count_turn_[i] != 0 || hand_win_count_river_[i] != 0) {
        hand = SortCodeToHand(i);
      }
      if (hand_win_count_flop_[i] != 0) {
        winning_hand_type_flop[static_cast<int>(hand.type())] += hand_win_count_flop_[i];
        count_flop += hand_win_count_flop_[i];
        if (median_offset_flop == 0 && count_flop >= median) {
          median_offset_flop = i;
        }
      }
      if (hand_win_count_turn_[i] != 0) {
        winning_hand_type_turn[static_cast<int>(hand.type())] += hand_win_count_turn_[i];
        count_turn += hand_win_count_turn_[i];
        if (median_offset_turn == 0 && count_turn >= median) {
          median_offset_turn = i;
        }
      }
      if (hand_win_count_river_[i] != 0) {
        winning_hand_type_river[static_cast<int>(hand.type())] += hand_win_count_river_[i];
        count_river += hand_win_count_river_[i];
        if (median_offset_river == 0 && count_river >= median) {
          median_offset_river = i;
        }
      }
    }

    std::string flush_suffix;

    // Winning hand distribution (flop)
    output_file = std::filesystem::path(args_.output_dir);
    output_file.append("winning-hand-distribution-flop.csv");
    if (args_.append_output) {
      fout = std::ofstream(output_file, std::ios::app);
    } else {
      fout = std::ofstream(output_file);
      fout << "Players,High Card,One Pair,Two Pair,Three Of A Kind,Straight,Flush,Full House,Four Of A Kind,Straight Flush,Median\n";
    }
    fout << table_->players().size() << ",";
    fout << std::fixed << std::setprecision(3);
    for (int i=1; i<10; i++) {
      double percentage = (winning_hand_type_flop[i] * 100.0) / args_.iterations;
      fout << percentage << ",";
    }
    hand = SortCodeToHand(median_offset_flop);
    flush_suffix = FlushSuffix(hand);
    hand.set_type(HandType::HANDTYPE_UNSPECIFIED);
    fout << hand << flush_suffix << std::endl;
    fout.close();

    // Winning hand distribution (turn)
    output_file = std::filesystem::path(args_.output_dir);
    output_file.append("winning-hand-distribution-turn.csv");
    if (args_.append_output) {
      fout = std::ofstream(output_file, std::ios::app);
    } else {
      fout = std::ofstream(output_file);
      fout << "Players,High Card,One Pair,Two Pair,Three Of A Kind,Straight,Flush,Full House,Four Of A Kind,Straight Flush,Median\n";
    }
    fout << table_->players().size() << ",";
    fout << std::fixed << std::setprecision(3);
    for (int i=1; i<10; i++) {
      double percentage = (winning_hand_type_turn[i] * 100.0) / args_.iterations;
      fout << percentage << ",";
    }
    hand = SortCodeToHand(median_offset_turn);
    flush_suffix = FlushSuffix(hand);
    hand.set_type(HandType::HANDTYPE_UNSPECIFIED);
    fout << hand << flush_suffix << std::endl;
    fout.close();

    // Winning hand distribution (river)
    output_file = std::filesystem::path(args_.output_dir);
    output_file.append("winning-hand-distribution-river.csv");
    if (args_.append_output) {
      fout = std::ofstream(output_file, std::ios::app);
    } else {
      fout = std::ofstream(output_file);
      fout << "Players,High Card,One Pair,Two Pair,Three Of A Kind,Straight,Flush,Full House,Four Of A Kind,Straight Flush,Median\n";
    }
    fout << table_->players().size() << ",";
    fout << std::fixed << std::setprecision(3);
    for (int i=1; i<10; i++) {
      double percentage = (winning_hand_type_river[i] * 100.0) / args_.iterations;
      fout << percentage << "%,";
    }
    hand = SortCodeToHand(median_offset_river);
    flush_suffix = FlushSuffix(hand);
    hand.set_type(HandType::HANDTYPE_UNSPECIFIED);
    fout << hand << flush_suffix << std::endl;
    fout.close();

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
