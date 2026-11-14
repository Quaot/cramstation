#pragma once

#include <windows.h>

#include "text.h"

extern const wchar_t *background_class_name;    // L"CrammyBackground"

UINT show_panel_message();
bool background_is_running();
bool this_program_is_in_background();   // the background window belongs to this process
bool ask_running_crammy_to_toggle_panel();
bool ask_running_crammy_to_show_panel();    // like toggle, but an open panel stays open
bool set_start_at_sign_in( bool on );
wstring startup_shortcut_path();    // CRAMMY_STARTUP_FOLDER, when set, replaces the Startup folder
int run_background( bool show_panel_now );
