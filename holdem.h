#ifndef HOLDEM_H
#define HOLDEM_H

#include <vector>

#include "poker.h"

class TableHoldem : public Table {
public:
private:
  std::vector<Card> community_cards;
};

#endif // HOLDEM_H
