#pragma once

#include <windows.h>

#include "text.h"

extern const wchar_t *background_class_name;    // L"CrammyBackground"

UINT show_panel_message();
bool background_is_running();
bool ask_running_crammy_to_toggle_panel();
bool set_start_at_sign_in( bool on );
wstring startup_shortcut_path();    // CRAMMY_STARTUP_FOLDER, when set, replaces the Startup folder
int run_background( bool show_panel_now );
