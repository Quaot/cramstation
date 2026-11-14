// Written from tests\specs\desktops.md.

#include <windows.h>

#include "check.h"
#include "../src/desktops.h"

namespace {

const wchar_t *window_class_name = L"CrammyDesktopsTestWindow";

void register_window_class() {
    WNDCLASSW window_class = {};
    window_class.lpfnWndProc = DefWindowProcW;
    window_class.hInstance = GetModuleHandleW( nullptr );
    window_class.lpszClassName = window_class_name;
    RegisterClassW( &window_class );
}

// Made off screen so it never appears on the owner's screen.
HWND make_window( const wstring &title, bool shown ) {
    HWND window = CreateWindowExW( WS_EX_APPWINDOW, window_class_name, title.c_str(),
                                   WS_OVERLAPPEDWINDOW, -32000, -32000, 200, 100,
                                   nullptr, nullptr, GetModuleHandleW( nullptr ), nullptr );
    if ( window && shown )
        ShowWindow( window, SW_SHOWNOACTIVATE );
    return window;
}

const window_on_desktop *find_window( const vector<window_on_desktop> &list, HWND window ) {
    for ( const window_on_desktop &entry : list )
        if ( entry.window == window )
            return &entry;
    return nullptr;
}

wstring own_program_path() {
    wchar_t buffer[MAX_PATH * 4] = {};
    GetModuleFileNameW( nullptr, buffer, MAX_PATH * 4 );
    return buffer;
}

bool same_path_ci( const wstring &first, const wstring &second ) {
    return lower( first ) == lower( second );
}

void check_names() {
    vector<wstring> names = desktop_names();
    check( !names.empty(), "There is at least one desktop name." );
    bool all_named = true;
    for ( const wstring &name : names )
        if ( name.empty() )
            all_named = false;
    check( all_named, "No desktop name is empty." );
}

void check_current_index() {
    int count = (int)desktop_names().size();
    int index = current_desktop_index();
    check( index >= 0 && index < count, "The current desktop index is a valid index into the names." );
}

void check_visible_window() {
    wstring title = L"Crammy desktops check " + std::to_wstring( GetCurrentProcessId() ) + L" " +
                    std::to_wstring( GetTickCount64() );
    HWND window = make_window( title, true );
    check( window != nullptr, "The off-screen test window was made." );
    vector<window_on_desktop> list = windows_by_desktop();
    const window_on_desktop *entry = find_window( list, window );
    int current = current_desktop_index();
    wstring own_path = own_program_path();
    DestroyWindow( window );

    check( entry != nullptr, "A visible titled window is in the list of windows." );
    check( entry && entry->desktop_index == current, "The test window is on the current desktop." );
    check( entry && same_path_ci( entry->program_path, own_path ),
           "The test window's program path is the test program's own path." );
}

void check_hidden_window() {
    HWND window = make_window( L"Crammy hidden desktops check", false );
    vector<window_on_desktop> list = windows_by_desktop();
    bool found = find_window( list, window ) != nullptr;
    DestroyWindow( window );
    check( !found, "A window that was never shown is not in the list of windows." );
}

void check_all_indexes() {
    int count = (int)desktop_names().size();
    bool all_valid = true;
    for ( const window_on_desktop &entry : windows_by_desktop() )
        if ( entry.desktop_index < 0 || entry.desktop_index >= count )
            all_valid = false;
    check( all_valid, "Every listed window has a valid desktop index." );
}

} // namespace

void test_desktops() {
    register_window_class();
    check_names();
    check_current_index();
    check_visible_window();
    check_hidden_window();
    check_all_indexes();
}
