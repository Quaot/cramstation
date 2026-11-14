#pragma once

#include <windows.h>

#include "text.h"

// Switches to the virtual desktop with this name, as Ctrl+Win+Left/Right would.
// Returns an empty string on success, or what went wrong.
wstring switch_to_desktop( const wstring &name );

// Names of the virtual desktops, in the order Task View shows them.
vector<wstring> desktop_names();

// Index into desktop_names() of the desktop showing, or -1.
int current_desktop_index();

struct window_on_desktop {
    HWND window;
    int desktop_index;      // index into desktop_names()
    wstring program_path;   // the program that owns the window
};

// Visible, titled, unowned top-level windows, each with its desktop.
vector<window_on_desktop> windows_by_desktop();

// Makes a new desktop with this name at the end of the list. problem says what went wrong, or
// what still needs doing when the name could not be set.
bool create_desktop( const wstring &name, wstring &problem );
