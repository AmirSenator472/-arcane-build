#ifndef ARCANE_CLIENT_SRC_ACTOR_ACTOR_H
#define ARCANE_CLIENT_SRC_ACTOR_ACTOR_H

#include <arcane_packets/configuration.hpp>

#include "spread_control.h"

namespace modification::client::actor {
class actor {
 public:
  void process();

 public:
  struct packets::configuration::actor settings;

 private:
  spread_control spread_control;
};
}  // namespace modification::client::actor

#endif  // ARCANE_CLIENT_SRC_ACTOR_ACTOR_H
