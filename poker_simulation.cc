#include <iostream>
#include <random>
#include <ctime>

#include "holdem.h"

int main() {
  std::mt19937 rng(static_cast<unsigned int>(std::time(0)));
  Deck deck;
  deck.Shuffle(rng);
  std::wcout << deck << std::endl;
  deck.Shuffle(rng);
  std::wcout << deck << std::endl;
  return 0;
}
