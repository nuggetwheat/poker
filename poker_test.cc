#include <gtest/gtest.h>

#include <iostream>
#include <sstream>
#include <string>

#include "cards.h"
#include "cards.pb.h"
#include "poker.h"
#include "poker.pb.h"
#include <google/protobuf/text_format.h>

namespace {
  using ::google::protobuf::TextFormat;
}

class PokerTest : public testing::Test {
protected:
  bool CardTextProtosToVector(const std::vector<std::string>& input,
                              std::vector<Card>& output) {
    Card card;    
    output.clear();
    for (const std::string& text_proto : input) {
      if (!TextFormat::ParseFromString(text_proto, &card))
        return false;
      output.push_back(card);
    }
    return true;
  }

  bool SetCommunityCards(std::vector<std::string> input) {
    std::vector<Card> community;
    if (!CardTextProtosToVector(input, community))
      return false;
    he_.Reset(community);
    return true;
  }

  bool SetHoleCards(std::vector<std::string> input) {
    return CardTextProtosToVector(input, hole_cards_);
  }

  std::string ComputeHand() {
    std::stringstream ss;
    poker::Hand hand = he_.Evaluate(hole_cards_);
    ss << hand;
    return ss.str();
  }
  
  poker::HandEvaluator he_;
  std::vector<Card> hole_cards_;
};

TEST_F(PokerTest, HandEvaluator) {
  ASSERT_TRUE(SetCommunityCards({}));

  // High card
  ASSERT_TRUE(SetHoleCards({
        "rank: FIVE suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: JACK suit: SPADES",
        "rank: THREE suit: CLUBS" }));
  EXPECT_EQ(ComputeHand(), "high-card KJ753");

  // One pair
  ASSERT_TRUE(SetHoleCards({
        "rank: FIVE suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: JACK suit: SPADES",
        "rank: SEVEN suit: CLUBS" }));
  EXPECT_EQ(ComputeHand(), "one-pair 77KJ5");

  // Two pair
  ASSERT_TRUE(SetHoleCards({
        "rank: JACK suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: JACK suit: SPADES",
        "rank: SEVEN suit: CLUBS" }));
  EXPECT_EQ(ComputeHand(), "two-pair JJ77K");

  // Three of a kind
  ASSERT_TRUE(SetHoleCards({
        "rank: JACK suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: JACK suit: SPADES",
        "rank: JACK suit: CLUBS" }));
  EXPECT_EQ(ComputeHand(), "three-of-a-kind JJJK7");

  // Straight
  ASSERT_TRUE(SetHoleCards({
        "rank: TWO suit: HEARTS",   "rank: FOUR suit: DIAMONDS",
        "rank: ACE suit: DIAMONDS", "rank: FIVE suit: SPADES",
        "rank: THREE suit: CLUBS" }));
  EXPECT_EQ(ComputeHand(), "straight 5432A");
  ASSERT_TRUE(SetHoleCards({
        "rank: KING suit: HEARTS",  "rank: TEN suit: DIAMONDS",
        "rank: ACE suit: DIAMONDS", "rank: JACK suit: SPADES",
        "rank: QUEEN suit: CLUBS" }));
  EXPECT_EQ(ComputeHand(), "straight AKQJT");

  // Flush
  ASSERT_TRUE(SetHoleCards({
        "rank: TWO suit: HEARTS",  "rank: FOUR suit: HEARTS",
        "rank: KING suit: HEARTS", "rank: FIVE suit: HEARTS",
        "rank: THREE suit: HEARTS" }));
  EXPECT_EQ(ComputeHand(), "flush K5432");

  // Four of a kind
  ASSERT_TRUE(SetHoleCards({
        "rank: TWO suit: HEARTS", "rank: KING suit: HEARTS",
        "rank: TWO suit: CLUBS",  "rank: TWO suit: SPADES",
        "rank: TWO suit: DIAMONDS" }));
  EXPECT_EQ(ComputeHand(), "four-of-a-kind 2222K");

  // Straight flush
  ASSERT_TRUE(SetHoleCards({
        "rank: TWO suit: SPADES", "rank: FOUR suit: SPADES",
        "rank: ACE suit: SPADES", "rank: FIVE suit: SPADES",
        "rank: THREE suit: SPADES" }));
  EXPECT_EQ(ComputeHand(), "straight-flush 5432A");
  ASSERT_TRUE(SetHoleCards({
        "rank: KING suit: DIAMONDS", "rank: TEN suit: DIAMONDS",
        "rank: ACE suit: DIAMONDS",  "rank: JACK suit: DIAMONDS",
        "rank: QUEEN suit: DIAMONDS" }));
  EXPECT_EQ(ComputeHand(), "straight-flush AKQJT");
}
