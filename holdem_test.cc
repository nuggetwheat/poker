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

TEST_F(PokerTest, HoldemHandEvaluator) {

  // High card
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: THREE suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: FOUR suit: HEARTS",
        "rank: QUEEN suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "high-card AKQ75");
  ASSERT_TRUE(SetHoleCards({
        "rank: JACK suit: HEARTS",
        "rank: TEN suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "high-card AKJT7");

  // One pair
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: FIVE suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: FOUR suit: HEARTS",
        "rank: QUEEN suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "one-pair 55AKQ");
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: FOUR suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: FIVE suit: SPADES",
        "rank: TWO suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "one-pair 55AK7");

  // Two pair  
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: FIVE suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: KING suit: HEARTS",
        "rank: QUEEN suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "two-pair KK55A");
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: FOUR suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: FIVE suit: SPADES",
        "rank: SEVEN suit: HEARTS"}));
  EXPECT_EQ(ComputeHand(), "two-pair 7755A");
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: KING suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: FIVE suit: SPADES",
        "rank: SEVEN suit: HEARTS"}));
  EXPECT_EQ(ComputeHand(), "two-pair KK77A");

  // Three-of-a-kind
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: FIVE suit: DIAMONDS",
        "rank: TWO suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: FIVE suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: EIGHT suit: HEARTS",
        "rank: QUEEN suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "three-of-a-kind 555AQ");
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: FIVE suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: FIVE suit: SPADES",
        "rank: SIX suit: HEARTS"}));
  EXPECT_EQ(ComputeHand(), "three-of-a-kind 555AK");

  // Straight
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: FOUR suit: DIAMONDS",
        "rank: TWO suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: THREE suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: EIGHT suit: HEARTS",
        "rank: QUEEN suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "straight 5432A");
  ASSERT_TRUE(SetCommunityCards({
        "rank: KING suit: HEARTS",   "rank: FOUR suit: DIAMONDS",
        "rank: TWO suit: DIAMONDS", "rank: THREE suit: SPADES",
        "rank: QUEEN suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: FIVE suit: HEARTS",
        "rank: ACE suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "straight 5432A");
  ASSERT_TRUE(SetCommunityCards({
        "rank: KING suit: HEARTS",   "rank: FOUR suit: DIAMONDS",
        "rank: TWO suit: DIAMONDS", "rank: TEN suit: SPADES",
        "rank: QUEEN suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: JACK suit: HEARTS",
        "rank: ACE suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "straight AKQJT");
  ASSERT_TRUE(SetCommunityCards({
        "rank: KING suit: HEARTS",   "rank: EIGHT suit: DIAMONDS",
        "rank: TWO suit: DIAMONDS", "rank: TEN suit: SPADES",
        "rank: SEVEN suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: NINE suit: HEARTS",
        "rank: SIX suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "straight T9876");
  ASSERT_TRUE(SetCommunityCards({
        "rank: KING suit: HEARTS",   "rank: EIGHT suit: DIAMONDS",
        "rank: NINE suit: DIAMONDS", "rank: TEN suit: SPADES",
        "rank: SEVEN suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: NINE suit: HEARTS",
        "rank: SIX suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "straight T9876");

  // Flush
  ASSERT_TRUE(SetCommunityCards({
        "rank: KING suit: HEARTS",   "rank: EIGHT suit: DIAMONDS",
        "rank: TWO suit: HEARTS", "rank: TEN suit: HEARTS",
        "rank: SEVEN suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: NINE suit: HEARTS",
        "rank: SIX suit: HEARTS"}));
  EXPECT_EQ(ComputeHand(), "flush KT962");
  ASSERT_TRUE(SetCommunityCards({
        "rank: QUEEN suit: HEARTS",   "rank: EIGHT suit: HEARTS",
        "rank: TWO suit: HEARTS", "rank: TEN suit: HEARTS",
        "rank: SEVEN suit: HEARTS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: SIX suit: HEARTS",
        "rank: JACK suit: HEARTS"}));
  EXPECT_EQ(ComputeHand(), "flush QJT87");
  ASSERT_TRUE(SetCommunityCards({
        "rank: KING suit: HEARTS",   "rank: EIGHT suit: HEARTS",
        "rank: TWO suit: HEARTS", "rank: TEN suit: HEARTS",
        "rank: SEVEN suit: HEARTS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: NINE suit: CLUBS",
        "rank: SIX suit: CLUBS"}));
  EXPECT_EQ(ComputeHand(), "flush KT872");
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: SPADES",   "rank: FOUR suit: SPADES",
        "rank: KING suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: EIGHT suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: JACK suit: SPADES",
        "rank: TWO suit: SPADES"}));
  EXPECT_EQ(ComputeHand(), "flush AJ542");

  // Four-of-a-kind
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: FIVE suit: DIAMONDS",
        "rank: TWO suit: DIAMONDS", "rank: FIVE suit: SPADES",
        "rank: FIVE suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: EIGHT suit: HEARTS",
        "rank: QUEEN suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "four-of-a-kind 5555Q");
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: HEARTS",   "rank: SEVEN suit: DIAMONDS",
        "rank: KING suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: FIVE suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: FIVE suit: SPADES",
        "rank: FIVE suit: DIAMONDS"}));
  EXPECT_EQ(ComputeHand(), "four-of-a-kind 5555A");

  // Straight flush
  ASSERT_TRUE(SetCommunityCards({
        "rank: FIVE suit: SPADES",   "rank: FOUR suit: SPADES",
        "rank: KING suit: DIAMONDS", "rank: ACE suit: SPADES",
        "rank: FIVE suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: THREE suit: SPADES",
        "rank: TWO suit: SPADES"}));
  EXPECT_EQ(ComputeHand(), "straight-flush 5432A");
  ASSERT_TRUE(SetCommunityCards({
        "rank: JACK suit: SPADES",   "rank: FOUR suit: SPADES",
        "rank: KING suit: SPADES", "rank: ACE suit: SPADES",
        "rank: FIVE suit: CLUBS" }));
  ASSERT_TRUE(SetHoleCards({
        "rank: QUEEN suit: SPADES",
        "rank: TEN suit: SPADES"}));
  EXPECT_EQ(ComputeHand(), "straight-flush AKQJT");
}
