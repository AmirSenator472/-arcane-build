#ifndef ARCANE_CLIENT_SRC_SETTINGS_MENU_H
#define ARCANE_CLIENT_SRC_SETTINGS_MENU_H

#include <windows.h>

#include <atomic>
#include <filesystem>
#include <string>

#include <imgui.h>

#include "actor/actor.h"
#include "shooting/silent_aimbot.h"
#include "shooting/vector_aimbot.h"

namespace modification::client {
class settings_menu {
 public:
  settings_menu(shooting::vector_aimbot& vector_aimbot,
                shooting::silent_aimbot& silent_aimbot,
                actor::actor& actor,
                std::atomic_bool& remote_settings_loaded);

  void initialize();
  void process();
  void toggle();

 private:
  struct preset {
    bool silent_aim{true};
    bool smooth_aim{true};
    bool no_spread{true};
    float fov{10.0f};
    float smooth{50.0f};
  };

  void draw_menu();
  void draw_fov();
  void draw_key_selector(const char* label, unsigned int& key, bool& waiting);
  void update_hotkeys();
  void load_preset_from_disk();
  void save_preset_to_disk() const;
  void apply_preset(const preset& value);
  preset capture_preset() const;
  std::filesystem::path config_path() const;
  static const char* key_name(unsigned int key);
  static bool is_valid_bind_key(int key);

 private:
  shooting::vector_aimbot& vector_aimbot_;
  shooting::silent_aimbot& silent_aimbot_;
  actor::actor& actor_;
  std::atomic_bool& remote_settings_loaded_;

  bool initialized_{};
  bool open_{};
  bool waiting_for_preset_key_{};
  unsigned int preset_key_{VK_F11};
  preset settings_{};
  preset saved_preset_{};
};
}  // namespace modification::client

#endif  // ARCANE_CLIENT_SRC_SETTINGS_MENU_H
