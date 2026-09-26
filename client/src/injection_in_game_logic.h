#ifndef ARCANE_CLIENT_SRC_INJECTION_IN_GAME_LOGIC_H
#define ARCANE_CLIENT_SRC_INJECTION_IN_GAME_LOGIC_H

#include <string_view>

#include "game_logic_signals.hpp"

#include "shooting/silent_aimbot.h"
#include "shooting/vector_aimbot.h"

#include "actor/actor.h"
#include "settings_menu.h"

#include "client.h"

namespace modification::client {
class injection_in_game_logic {
  friend class client;

 public:
  injection_in_game_logic(std::string_view username, std::string_view password,
                          std::string_view hwid);
  injection_in_game_logic(const injection_in_game_logic&) = delete;
  injection_in_game_logic(injection_in_game_logic&&) = delete;

 public:
  shooting::vector_aimbot vector_aimbot;
  shooting::silent_aimbot silent_aimbot;
 public:
  actor::actor actor;

 private:
  static void load_debug_console();
  static void load_imgui_context();
  void load_anticheat_patch();
  void load_keys();
  void load_samp();
  void load_unload();
  void load_vector_aimbot();
  void load_silent_aimbot();
  void load_menu();
  void load_actor();

 private:  // details of loads
  game_logic_signals signals_;
  bool is_aiming_at_person_;
  bool was_last_compute_mouse_target_caller_local_player_;
  settings_menu menu_;
  std::atomic_bool remote_settings_loaded_{false};

 private:  // things for multithreading
  std::atomic_bool has_to_break_thread_;

 private:
  const std::string username_;
  const std::string password_;
  const std::string hwid_;
};
}  // namespace modification::client

#endif  // ARCANE_CLIENT_SRC_INJECTION_IN_GAME_LOGIC_H
