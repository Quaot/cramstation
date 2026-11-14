#include "settings.h"

// Placeholder until settings are built from tests\specs\settings.md

crammy_settings default_settings() { return {}; }
crammy_settings read_settings( const wstring &, vector<wstring> & ) { return {}; }
wstring write_settings( const crammy_settings & ) { return L""; }
bool parse_shortcut( const wstring &, UINT &, UINT & ) { return false; }
wstring shortcut_text( UINT, UINT ) { return L""; }
bool youtonomous_command( const crammy_settings &, bool, wstring &, wstring & ) { return false; }
wstring settings_path() { return L""; }
