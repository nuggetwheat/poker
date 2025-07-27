#include <iostream>
#include <random>
#include <ctime>

#include "cards.h"
#include "holdem.h"

int main() {
  std::mt19937 rng(static_cast<unsigned int>(std::time(0)));
  Deck deck;
  deck.Shuffle(rng);
  std::wcout << "\n[random]\n";
  std::wcout << deck << std::endl;
  deck.Sort(ByRankAceHighSortFn);
  std::wcout << "\n[by rank ace high]\n";
  std::wcout << deck << std::endl;
  deck.Sort(ByRankAceLowSortFn);
  std::wcout << "\n[by rank ace low]\n";
  std::wcout << deck << std::endl;
  deck.Sort(BySuitAceHighSortFn);
  std::wcout << "\n[by suit ace high]\n";
  std::wcout << deck << std::endl;
  deck.Sort(BySuitAceLowSortFn);
  std::wcout << "\n[by suit ace low]\n";
  std::wcout << deck << std::endl;
  return 0;
}
