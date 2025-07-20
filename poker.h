#ifndef POKER_H
#define POKER_H

#include <vector>

enum class Rank {
  ACE = 1,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  SEVEN,
  EIGHT,
  NINE,
  TEN,
  JACK,
  QUEEN,
  KING,
};

std::wostream& operator<<(std::wostream& os, const Rank& rank) {
  switch (rank) {
  case Rank::ACE:
    os << L"A";
    break;
  case Rank::TWO:
    os << L"2";
    break;
  case Rank::THREE:
    os << L"3";
    break;
  case Rank::FOUR:
    os << L"4";
    break;
  case Rank::FIVE:
    os << L"5";
    break;
  case Rank::SIX:
    os << L"6";
    break;
  case Rank::SEVEN:
    os << L"7";
    break;
  case Rank::EIGHT:
    os << L"8";
    break;
  case Rank::NINE:
    os << L"9";
    break;
  case Rank::TEN:
    os << L"T";
    break;
  case Rank::JACK:
    os << L"J";
    break;
  case Rank::QUEEN:
    os << L"Q";
    break;
  case Rank::KING:
    os << L"K";
    break;
  default:
    os << L"?";
    break;
  }
  return os;
}

enum class Suit {
  SPADES,
  CLUBS,
  HEARTS,
  DIAMONDS,
};

std::wostream& operator<<(std::wostream& os, const Suit& suit) {
  switch (suit) {
  case Suit::SPADES:
    os << L"\xE2\x99\xA0";
    break;
  case Suit::CLUBS:
    os << L"\xE2\x99\xA3";
    break;
  case Suit::HEARTS:
    os << L"\xE2\x99\xA5";
    break;
  case Suit::DIAMONDS:
    os << L"\xE2\x99\xA6";
    break;
  default:
    os << L"?";
    break;
  }
  return os;
}

class Card {
public:
  Card(enum Rank rank, enum Suit suit) : rank_(rank), suit_(suit) {}
  Rank Rank() const { return rank_; }
  Suit Suit() const { return suit_; }
private:
  enum Rank rank_;
  enum Suit suit_;
};

std::wostream& operator<<(std::wostream& os, const Card& card) {
  os << card.Rank() << card.Suit();
  return os;
}

class Deck {
public:
  Deck() {
    for (Suit suit : {Suit::SPADES, Suit::CLUBS, Suit::HEARTS, Suit::DIAMONDS}) {
      for (Rank rank : {Rank::ACE, Rank::TWO, Rank::THREE, Rank::FOUR, Rank::FIVE, Rank::SIX, Rank::SEVEN, Rank::EIGHT, Rank::NINE, Rank::TEN, Rank::JACK, Rank::QUEEN, Rank::KING}) {
	cards.push_back(Card(rank, suit));
      }
    }
  }
  const std::vector<Card>& Cards() const { return cards; }
private:
  std::vector<Card> cards;
};

std::wostream& operator<<(std::wostream& os, const Deck& deck) {
  bool after_first{};
  for (Card card : deck.Cards()) {
    if (after_first) {
      os << L" " << card;
    } else {
      os << card;
      after_first = true;
    }
  }
  return os;
}

class Player {
public:
private:
  std::string name;
  std::vector<Card> hand;
};

class Table {
public:
protected:
  std::vector<Player> players;
  Deck deck;
};

#endif // POKER_H
