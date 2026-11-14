#pragma once

#include <windows.h>

#include "panel_model.h"

// Opens the panel above the bottom-left corner of the taskbar, or closes it when it is open.
void toggle_panel();

// Brings the main window forward, or opens it when it is not open.
void show_setups();

// The height draw_panel needs for this content.
int panel_height( const today_section &today, const vector<desktop_tile> &tiles, UINT dpi );

void draw_panel( HDC dc, int width, int height, const today_section &today, const vector<desktop_tile> &tiles,
                 const wstring &date_text, const wstring &time_text, UINT dpi );
