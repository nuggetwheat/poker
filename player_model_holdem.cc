#include "player_model_holdem.h"

#include "holdem.h"
#include "player_model.h"
#include "poker.h"

namespace poker::holdem {

PlayerAction PlayerModelShowdown::Act(const Table& table, Round round,
                                      int position, Player& player) {
  return PlayerAction::CHECK;
}

PlayerAction PlayerModelMillerTight::Act(const Table& table, Round round,
                                         int position, Player& player) {
  return PlayerAction::CHECK;
}

} // namespace poker::holdem
