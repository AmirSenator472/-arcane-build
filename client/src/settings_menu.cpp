#include "settings_menu.h"

#include <algorithm>
#include <cmath>
#include <array>
#include <fstream>
#include <atomic>
#include <iomanip>
#include <cstdio>
#include <sstream>

#include <psdk_utils/key.h>
#include <psdk_utils/psdk_utils.h>
#include <psdk_utils/weapon.h>

using namespace modification::client;

namespace {
struct mode_entry {
  psdk_utils::weapon::mode mode;
  const char* name;
};

constexpr std::array<mode_entry, 5> kModes{{
    {psdk_utils::weapon::mode::pistols, "Pistols"},
    {psdk_utils::weapon::mode::shotguns, "Shotguns"},
    {psdk_utils::weapon::mode::semi, "SMG"},
    {psdk_utils::weapon::mode::assault, "Assault"},
    {psdk_utils::weapon::mode::rifles, "Rifles"},
}};

std::string read_line_value(const std::filesystem::path& path, const std::string& key,
                            const std::string& fallback) {
  std::ifstream file(path);
  if (!file) return fallback;

  std::string line;
  const std::string prefix = key + "=";
  while (std::getline(file, line)) {
    if (line.rfind(prefix, 0) == 0) return line.substr(prefix.size());
  }
  return fallback;
}

bool parse_bool(const std::string& value, bool fallback) {
  if (value == "1" || value == "true") return true;
  if (value == "0" || value == "false") return false;
  return fallback;
}

float parse_float(const std::string& value, float fallback) {
  try {
    return std::stof(value);
  } catch (...) {
    return fallback;
  }
}

unsigned int parse_uint(const std::string& value, unsigned int fallback) {
  try {
    const auto result = std::stoul(value);
    return result <= 255 ? static_cast<unsigned int>(result) : fallback;
  } catch (...) {
    return fallback;
  }
}
}  // namespace

settings_menu::settings_menu(shooting::vector_aimbot& vector_aimbot,
                             shooting::silent_aimbot& silent_aimbot,
                             actor::actor& actor,
                             std::atomic_bool& remote_settings_loaded)
    : vector_aimbot_{vector_aimbot},
      silent_aimbot_{silent_aimbot},
      actor_{actor},
      remote_settings_loaded_{remote_settings_loaded} {
}

void settings_menu::initialize() {
  if (initialized_) return;

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;

  load_preset_from_disk();
  apply_preset(settings_);
  initialized_ = true;
}

void settings_menu::process() {
  if (!remote_settings_loaded_.load(std::memory_order_acquire)) return;
  if (!initialized_) initialize();

  update_hotkeys();
  apply_preset(settings_);
  draw_fov();
  if (!open_) return;

  draw_menu();
}

void settings_menu::toggle() {
  open_ = !open_;
}

void settings_menu::update_hotkeys() {
  if (waiting_for_preset_key_) return;

  if (psdk_utils::key::pressed(VK_F12)) {
    open_ = !open_;
    return;
  }

  if (preset_key_ != VK_F12 && psdk_utils::key::pressed(preset_key_)) {
    apply_preset(saved_preset_);
  }
}

void settings_menu::draw_fov() {
  if (!settings_.silent_aim && !settings_.smooth_aim) return;

  const auto resolution = psdk_utils::resolution();
  const ImVec2 center{resolution.x() * 0.5f, resolution.y() * 0.5f};
  const auto radians = settings_.fov * 3.14159265358979323846f / 180.0f;
  const auto radius = std::tan(radians * 0.5f) * resolution.y() * 0.5f;

  ImGui::GetForegroundDrawList()->AddCircle(center, std::max(2.0f, radius),
                                             IM_COL32(80, 220, 170, 180), 96, 1.5f);
}

void settings_menu::draw_menu() {
  ImGui::SetNextWindowSize(ImVec2(430.0f, 0.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(40.0f, 80.0f), ImGuiCond_FirstUseEver);

  bool visible = open_;
  if (!ImGui::Begin("Arcane - Aim Settings", &visible,
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::End();
    open_ = visible;
    return;
  }
  open_ = visible;

  ImGui::TextUnformatted("Only the selected aim features are active.");
  ImGui::Separator();

  ImGui::Checkbox("Silent Aim", &settings_.silent_aim);
  ImGui::Checkbox("Smooth Aim", &settings_.smooth_aim);
  ImGui::Checkbox("No Spread", &settings_.no_spread);
  ImGui::SliderFloat("FOV", &settings_.fov, 1.0f, 180.0f, "%.1f");
  ImGui::SliderFloat("Smooth", &settings_.smooth, 1.0f, 200.0f, "%.1f");

  ImGui::Separator();
  ImGui::TextUnformatted("Preset");

  draw_key_selector("Preset Hotkey", preset_key_, waiting_for_preset_key_);

  if (ImGui::Button("Save Settings", ImVec2(190.0f, 0.0f))) {
    saved_preset_ = capture_preset();
    save_preset_to_disk();
  }
  ImGui::SameLine();
  if (ImGui::Button("Load Settings", ImVec2(190.0f, 0.0f))) {
    load_preset_from_disk();
    apply_preset(settings_);
  }

  ImGui::Text("Preset key: %s", key_name(preset_key_));
  ImGui::TextDisabled("F12: open/close menu");

  ImGui::End();
}

void settings_menu::draw_key_selector(const char* label, unsigned int& key, bool& waiting) {
  ImGui::TextUnformatted(label);
  ImGui::SameLine();

  const std::string button_text = waiting ? "Press a key..." : key_name(key);
  if (ImGui::Button(button_text.c_str(), ImVec2(180.0f, 0.0f))) waiting = true;

  if (!waiting) return;

  for (int candidate = 1; candidate < 256; ++candidate) {
    if (!is_valid_bind_key(candidate)) continue;
    if (GetAsyncKeyState(candidate) & 0x8000) {
      if (candidate != VK_F12) {
        key = static_cast<unsigned int>(candidate);
        waiting = false;
      }
      break;
    }
  }
}

void settings_menu::load_preset_from_disk() {
  const auto path = config_path();
  settings_.silent_aim = parse_bool(read_line_value(path, "silent_aim", "1"), true);
  settings_.smooth_aim = parse_bool(read_line_value(path, "smooth_aim", "1"), true);
  settings_.no_spread = parse_bool(read_line_value(path, "no_spread", "1"), true);
  settings_.fov = std::clamp(parse_float(read_line_value(path, "fov", "10"), 10.0f), 1.0f, 180.0f);
  settings_.smooth = std::clamp(parse_float(read_line_value(path, "smooth", "50"), 50.0f), 1.0f, 200.0f);
  preset_key_ = parse_uint(read_line_value(path, "preset_key", "122"), VK_F11);
  if (preset_key_ == VK_F12) preset_key_ = VK_F11;

  saved_preset_ = settings_;
}

void settings_menu::save_preset_to_disk() const {
  const auto path = config_path();
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream file(path, std::ios::trunc);
  if (!file) return;

  file << "silent_aim=" << (saved_preset_.silent_aim ? 1 : 0) << '\n';
  file << "smooth_aim=" << (saved_preset_.smooth_aim ? 1 : 0) << '\n';
  file << "no_spread=" << (saved_preset_.no_spread ? 1 : 0) << '\n';
  file << std::fixed << std::setprecision(2);
  file << "fov=" << saved_preset_.fov << '\n';
  file << "smooth=" << saved_preset_.smooth << '\n';
  file << "preset_key=" << preset_key_ << '\n';
}

void settings_menu::apply_preset(const preset& value) {
  settings_ = value;

  for (const auto& entry : kModes) {
    auto& vector = vector_aimbot_.settings[entry.mode];
    vector.enable = value.smooth_aim;
    vector.smooth = value.smooth;
    vector.max_angle_in_degrees = value.fov;
    vector.aim_only_when_shooting = false;
    vector.do_not_change_z_angle = false;

    auto& silent = silent_aimbot_.settings[entry.mode];
    silent.enable = value.silent_aim;
    silent.max_angle_in_degrees = value.fov;
    silent.pull_camera_toward_enemy = false;
    silent.turn_player_toward_enemy = false;
    silent.display_triangle_above_enemy = false;
  }

  actor_.settings.spread_control.enable = value.no_spread;
  actor_.settings.spread_control.spread = 0.0f;
}

settings_menu::preset settings_menu::capture_preset() const {
  preset value = settings_;
  const auto mode = psdk_utils::weapon::mode::pistols;

  if (const auto it = vector_aimbot_.settings.find(mode); it != vector_aimbot_.settings.end()) {
    value.smooth_aim = it->second.enable;
    value.smooth = it->second.smooth;
    value.fov = it->second.max_angle_in_degrees;
  }
  if (const auto it = silent_aimbot_.settings.find(mode); it != silent_aimbot_.settings.end()) {
    value.silent_aim = it->second.enable;
    value.fov = it->second.max_angle_in_degrees;
  }
  value.no_spread = actor_.settings.spread_control.enable;
  return value;
}

std::filesystem::path settings_menu::config_path() const {
  char buffer[MAX_PATH]{};
  const DWORD length = GetEnvironmentVariableA("APPDATA", buffer, MAX_PATH);
  if (length > 0 && length < MAX_PATH) {
    return std::filesystem::path(buffer) / "Arcane" / "settings.ini";
  }
  return std::filesystem::current_path() / "arcane_settings.ini";
}

const char* settings_menu::key_name(unsigned int key) {
  static std::string name;
  if (key >= VK_F1 && key <= VK_F24) {
    name = "F" + std::to_string(key - VK_F1 + 1);
    return name.c_str();
  }

  switch (key) {
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_HOME: return "Home";
    case VK_END: return "End";
    case VK_PRIOR: return "PageUp";
    case VK_NEXT: return "PageDown";
    case VK_SPACE: return "Space";
    case VK_SHIFT: return "Shift";
    case VK_CONTROL: return "Ctrl";
    case VK_MENU: return "Alt";
    case VK_LBUTTON: return "Mouse1";
    case VK_RBUTTON: return "Mouse2";
    case VK_MBUTTON: return "Mouse3";
    default: break;
  }

  if (key >= '0' && key <= '9') {
    name.assign(1, static_cast<char>(key));
    return name.c_str();
  }
  if (key >= 'A' && key <= 'Z') {
    name.assign(1, static_cast<char>(key));
    return name.c_str();
  }

  static char fallback[16];
  std::snprintf(fallback, sizeof(fallback), "VK %u", key);
  return fallback;
}

bool settings_menu::is_valid_bind_key(int key) {
  return key >= 1 && key < 256 && key != VK_F12 && key != VK_LBUTTON && key != VK_RBUTTON;
}
