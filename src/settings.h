#pragma once

#include <windows.h>

#include "text.h"

struct crammy_settings {
    UINT shortcut_modifiers;    // MOD_CONTROL, MOD_ALT, MOD_SHIFT, MOD_WIN, combined with |
    UINT shortcut_key;          // a virtual-key code, such as 'C' or VK_F9
    wstring youtonomous;
    wstring pomodoro_long;      // the 3-hour video
    wstring pomodoro_short;     // the 1.5-hour video
    bool start_at_sign_in;
};

crammy_settings default_settings();
crammy_settings read_settings( const wstring &text, vector<wstring> &errors );
wstring write_settings( const crammy_settings &settings );
bool parse_shortcut( const wstring &text, UINT &modifiers, UINT &key );
wstring shortcut_text( UINT modifiers, UINT key );
bool youtonomous_command( const crammy_settings &settings, bool long_video, wstring &command, wstring &problem );
wstring settings_path();
