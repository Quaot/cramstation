#pragma once

#include <windows.h>

#include "profiles.h"

// The picture of the screen in the main window. Files and folders dropped on it
// become windows of the setup, and its tiles can be dragged between sides.

const UINT canvas_changed_message = WM_APP + 2;   // sent to the parent after every edit
const UINT canvas_hint_message = WM_APP + 3;      // lparam is a const wchar_t * to show as a hint

HWND create_canvas( HWND parent, HFONT font, UINT dpi );

// items may be null when no setup is selected; root is where relative paths start
void show_items( vector<written_item> *items, const wstring &root );
void add_notes_tile();

// What the canvas registers with Windows for drag and drop. The tests drop files on it directly.
struct IDropTarget *canvas_drop_target();
