#pragma once

#include <windows.h>

#include "text.h"

// Crammy keeps a list of the notes files it has opened in Notepad++, so that
// before the next setup opens it can save each one and close its tab.
// Tabs you opened yourself are left alone.

void remember_notes( const wstring &path );

// Saves and closes the tabs of every remembered notes file except those in keep.
// A tab that does not save cleanly stays open. window is the Notepad++ to use; the tests pass their own.
void close_old_notes( const vector<wstring> &keep, HWND window = nullptr );
