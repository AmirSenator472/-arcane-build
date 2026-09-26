#include "actor.h"

using namespace modification::client::actor;

void actor::process() {
  spread_control.process(settings.spread_control);
}
