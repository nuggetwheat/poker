#include "cards.h"

std::ostream& operator<<(std::ostream& os, const Rank& rank) {
  if (rank >= Rank::ONE && rank < Rank::TEN) {
    os << static_cast<int>(rank);
  } else {
    switch (rank) {
    case Rank::TEN:
      os << "T";
      break;
    case Rank::JACK:
      os << "J";
      break;
    case Rank::QUEEN:
      os << "Q";
      break;
    case Rank::KING:
      os << "K";
      break;
    case Rank::ACE:
      os << "A";
      break;
    default:
      std::cerr << "Invalid rank: " << static_cast<int>(rank) << std::endl;
      assert(false && "Invalid rank");
      os << "?";
      break;
    }
  }
  return os;
}

std::wostream& operator<<(std::wostream& os, const Rank& rank) {
  if (rank >= Rank::ONE && rank < Rank::TEN) {
    os << static_cast<int>(rank);
  } else {
    switch (rank) {
    case Rank::ACE:
      os << L"A";
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
      assert(false && "Invalid rank");
      os << L"?";
      break;
    }
  }
  return os;
}

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

std::wostream& operator<<(std::wostream& os, const Card& card) {
  os << card.rank() << card.suit();
  return os;
}

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
