#pragma once

#include <windows.h>

#include "text.h"

const UINT profiles_changed_message = WM_APP + 1;
const int save_shortcut = 0x9000;   // sent as WM_COMMAND when Ctrl+S is pressed

int scaled( int value, UINT dpi );
HFONT make_font( UINT dpi, const wchar_t *face, int points );
HWND add_control( HWND parent, const wchar_t *class_name, const wchar_t *text, DWORD style, int id, HFONT font,
                  DWORD ex_style = 0 );
RECT centred_on_screen( int width, int height );
HWND make_window( const wchar_t *class_name, WNDPROC procedure, const wchar_t *title, int width, int height, HWND owner,
                  DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN );
void run_message_loop();
void run_modal( HWND window, HWND owner );   // returns once window is destroyed

void show_main_window( const wstring &start_setup );
void show_editor( HWND owner );   // owner gets profiles_changed_message after a save
