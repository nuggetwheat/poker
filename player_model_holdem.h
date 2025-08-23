#ifndef PLAYER_MODEL_HOLDEM_H
#define PLAYER_MODEL_HOLDEM_H

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "holdem.h"
#include "player_model.h"
#include "poker.h"

namespace poker::holdem {

class PlayerModelShowdown : public PlayerModel {
 public:
  virtual ~PlayerModelShowdown() override = default;
  static const std::string name() { return "showdown"; }
  PlayerAction Act(const Table &table, Round round, int position,
                   Player &player) override;
};

class PlayerModelMillerTight : public PlayerModel {
 public:
  virtual ~PlayerModelMillerTight() override = default;
  static const std::string name() { return "miller_tight"; }
  PlayerAction Act(const Table &table, Round round, int position,
                   Player &player) override;
};

class PlayerModelFactory {
 public:
  static std::unique_ptr<PlayerModel> Create(std::string_view name) {
    if (name == PlayerModelShowdown::name()) {
      return std::make_unique<PlayerModelShowdown>();
    } else if (name == PlayerModelMillerTight::name()) {
      return std::make_unique<PlayerModelMillerTight>();
    } else {
      std::stringstream ss;
      ss << "Unrecognized player model: " << name;
      throw std::invalid_argument(ss.str());
    }
  }
};

} // namespace poker::holdem

#endif  // PLAYER_MODEL_HOLDEM_H
