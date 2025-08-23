#ifndef PLAYER_MODEL_H
#define PLAYER_MODEL_H

namespace poker {

enum class PlayerAction {
  UNSPECIFIED = 0,
  FOLD,
  CHECK,
  RAISE,
  RAISE_ALL_IN
};

} // namespace poker

#endif // PLAYER_MODEL_H
