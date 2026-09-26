#include "client.h"

#include <protected_string/protected_string.h>

#include <VMProtectSDK.h>

#include <utility>
#include <chrono>

#include "socket.h"
#include "query.h"
#include "injection_in_game_logic.h"

using namespace modification::client;

client::client(std::shared_ptr<injection_in_game_logic> injection)
    : injection_{std::move(injection)}, thread_{&client::process, this} {
}

void client::process() {
  modification::client::socket socket{scoped_protected_std_string(ARCANE_SERVER_IP),
                                      std::to_string(ARCANE_SERVER_PORT),
                                      scoped_protected_std_string(ARCANE_SERVER_KEY)};

  query::init(injection_->username_, injection_->password_, injection_->hwid_);

  bool settings_loaded = false;
  while (!injection_->has_to_break_thread_) {
    try {
      if (!settings_loaded) {
        const auto answer = query::send(socket, scoped_protected_std_string("settings"));
        auto document = nlohmann::json::parse(answer);

        packets::user_json user_json;
        from_json(document, user_json);

        using weapon_mode = psdk_utils::weapon::mode;
        for (auto i = weapon_mode::pistols; i != weapon_mode::unknown;
             ++reinterpret_cast<int&>(i)) {
          injection_->vector_aimbot.settings[i] = {};
          injection_->silent_aimbot.settings[i] = {};
        }

        if (!user_json.patterns.empty()) {
          const auto& configuration = user_json.patterns.front();

          injection_->vector_aimbot.friendly_nicknames = user_json.friendly_nicknames;
          injection_->vector_aimbot.model_groups = configuration.groups;
          injection_->silent_aimbot.friendly_nicknames = user_json.friendly_nicknames;
          injection_->silent_aimbot.model_groups = configuration.groups;

          injection_->vector_aimbot.settings[weapon_mode::pistols] =
              configuration.vector_aimbot_pistols;
          injection_->vector_aimbot.settings[weapon_mode::shotguns] =
              configuration.vector_aimbot_shotguns;
          injection_->vector_aimbot.settings[weapon_mode::semi] =
              configuration.vector_aimbot_semi;
          injection_->vector_aimbot.settings[weapon_mode::assault] =
              configuration.vector_aimbot_assault;
          injection_->vector_aimbot.settings[weapon_mode::rifles] =
              configuration.vector_aimbot_rifles;

          injection_->silent_aimbot.settings[weapon_mode::pistols] =
              configuration.silent_aimbot_pistols;
          injection_->silent_aimbot.settings[weapon_mode::shotguns] =
              configuration.silent_aimbot_shotguns;
          injection_->silent_aimbot.settings[weapon_mode::semi] =
              configuration.silent_aimbot_semi;
          injection_->silent_aimbot.settings[weapon_mode::assault] =
              configuration.silent_aimbot_assault;
          injection_->silent_aimbot.settings[weapon_mode::rifles] =
              configuration.silent_aimbot_rifles;

          // Only the no-spread setting is retained from the actor configuration.
          injection_->actor.settings.spread_control = configuration.actor.spread_control;
        }

        settings_loaded = true;
        injection_->remote_settings_loaded_.store(true, std::memory_order_release);
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));
    } catch (const socket::exception&) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    } catch (const std::exception&) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

std::thread& client::thread() {
  return thread_;
}
