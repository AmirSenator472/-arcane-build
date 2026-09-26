cmake_minimum_required(VERSION 3.16)

project(client)

include(rc/CMakeRC.cmake)

set(CMAKE_CXX_STANDARD 20)

add_library(client SHARED
        src/main.cpp
        src/main.h
        src/plugin.cpp
        src/plugin.h
        src/socket.cpp
        src/socket.h
        src/query.cpp
        src/query.h
        src/client.cpp
        src/client.h
        src/unload.cpp
        src/unload.h
        src/game_logic_signals.hpp
        src/injection_in_game_logic.cpp
        src/injection_in_game_logic.h
        src/shooting/enemy_finder.cpp
        src/shooting/enemy_finder.h
        src/shooting/vector_aimbot.cpp
        src/shooting/vector_aimbot.h
        src/shooting/silent_aimbot.cpp
        src/shooting/silent_aimbot.h
        src/actor/actor.cpp
        src/actor/actor.h
        src/actor/spread_control.cpp
        src/actor/spread_control.h
        src/settings_menu.cpp
        src/settings_menu.h
)

cmrc_add_resource_library(
        fonts
        rc/fonts/jetbrains_mono_bold.ttf
        rc/fonts/geforce_bold.ttf
)

target_link_libraries(client PRIVATE
        ws2_32
        wsock32
        ImGui::imgui
        nlohmann_json::nlohmann_json
        pluginsdk::gtasa
        sampapi
        kthook
        data_representation
        psdk_utils
        samp_utils
        winapi_utils
        utils
        encryption
        arcane_packets
        vmprotect-sdk
        fonts
        protected_string
        infoware
)

if (VMP_DEBUG)
    set_target_properties(client
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${ASI_PLUGIN_OUTPUT_DIR}
            LIBRARY_OUTPUT_DIRECTORY ${ASI_PLUGIN_OUTPUT_DIR}
            RUNTIME_OUTPUT_DIRECTORY ${ASI_PLUGIN_OUTPUT_DIR}
            SUFFIX ".asi"
    )
endif ()