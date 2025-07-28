#include "holdem.h"

#include <iostream>

#include "cards.pb.h"
#include "poker.h"
#include "poker.pb.h"

namespace poker::holdem {

Statistics::Statistics() {
  // Initialize hand index
  int offset{};
  Hand hand;
  for (Rank rank1 = Rank::ACE; rank1 > Rank::ONE;
       rank1 = OffsetRank(rank1, -1)) {
    hand = HoleHand(rank1, rank1, HandType::ONE_PAIR);
    //std::cout << offset << " (sort_code=" << hand.sort_code() << ") " << hand << std::endl;
    hand_index_[hand] = offset++;
    for (Rank rank2 = OffsetRank(rank1, -1); rank2 > Rank::ONE;
         rank2 = OffsetRank(rank2, -1)) {
      hand = HoleHand(rank1, rank2, HandType::FLUSH);
      //std::cout << offset << " (sort_code=" << hand.sort_code() << ") " << hand << std::endl;
      hand_index_[hand] = offset++;
      hand = HoleHand(rank1, rank2, HandType::HIGH_CARD);
      //std::cout << offset << " (sort_code=" << hand.sort_code() << ") " << hand << std::endl;
      hand_index_[hand] = offset++;
    }
  }

  // Initialize beat matrix
  beat_matrix_ = std::make_unique<std::vector<int>[]>(offset);
  for (int i=0; i<offset; i++) {
    beat_matrix_[i].resize(offset, 0);
  }
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
    for (auto& info : player_info_) {
      info.hole_hand = HoleHand(info.player->cards(0), info.player->cards(1));
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
      for (int i=0; i<player_info_.size()-1; i++) {
        for (int j=i+1; j<player_info_.size(); j++) {
          if (player_info_[i].hand.sort_code() > player_info_[j].hand.sort_code()) {
            int win_index = hand_index_.at(player_info_[i].hole_hand);
            int lose_index = hand_index_.at(player_info_[j].hole_hand);
            beat_matrix_[win_index][lose_index]++;
          }
        }
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
    if (stats.wins == 0)
      break;
    std::cout << stats.hand.rank(0) << stats.hand.rank(1);
    if (stats.hand.type() == HandType::FLUSH) {
      std::cout << "s -> ";
    } else if (stats.hand.type() == HandType::HIGH_CARD) {
      std::cout << "o -> ";
    } else {
      std::cout << "  -> ";
    }
    std::cout << stats.wins << std::endl;
  }
}


} // namespace poker::holdem
