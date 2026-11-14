#include "background.h"

#include <shlobj.h>
#include <objbase.h>
#include <shellapi.h>

#include "panel.h"
#include "settings.h"
#include "tiling.h"
#include "ui.h"

const wchar_t *background_class_name = L"CrammyBackground";

namespace {

const UINT tray_message = WM_APP + 20;
const int hotkey_id = 1;
const UINT tray_id = 1;
const UINT menu_setups = 1;
const UINT menu_timetable = 2;
const UINT menu_start_at_sign_in = 3;
const UINT menu_quit = 4;

UINT taskbar_created_message = 0;

HWND find_background_window() {
    return FindWindowW( background_class_name, nullptr );
}

wstring program_path() {
    wchar_t path[MAX_PATH * 4];
    DWORD length = GetModuleFileNameW( nullptr, path, MAX_PATH * 4 );
    return wstring( path, length );
}

bool shortcut_exists() {
    return is_file( startup_shortcut_path() );
}

bool make_shortcut( const wstring &path ) {
    HRESULT started = CoInitialize( nullptr );
    bool made = false;
    IShellLinkW *link = nullptr;
    if ( SUCCEEDED( CoCreateInstance( CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &link ) ) ) ) {
        wstring exe = join_path( parent_folder( program_path() ), L"crammy.exe" );
        link->SetPath( exe.c_str() );
        link->SetArguments( L"--background" );
        link->SetWorkingDirectory( parent_folder( exe ).c_str() );
        IPersistFile *file = nullptr;
        if ( SUCCEEDED( link->QueryInterface( IID_PPV_ARGS( &file ) ) ) ) {
            made = SUCCEEDED( file->Save( path.c_str(), TRUE ) );
            file->Release();
        }
        link->Release();
    }
    if ( SUCCEEDED( started ) ) CoUninitialize();
    return made;
}

void shortcut_keys( UINT &modifiers, UINT &key ) {
    vector<wstring> errors;
    crammy_settings settings = read_settings( read_text_file( settings_path() ), errors );
    modifiers = settings.shortcut_modifiers;
    key = settings.shortcut_key;
    if ( key == 0 ) {
        modifiers = MOD_CONTROL | MOD_ALT;
        key = 'C';
    }
}

NOTIFYICONDATAW tray_data( HWND window ) {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof( data );
    data.hWnd = window;
    data.uID = tray_id;
    return data;
}

void add_tray_icon( HWND window ) {
    NOTIFYICONDATAW data = tray_data( window );
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = tray_message;
    data.hIcon = LoadIconW( GetModuleHandleW( nullptr ), MAKEINTRESOURCEW( 1 ) );
    lstrcpyW( data.szTip, L"Crammy" );
    Shell_NotifyIconW( NIM_ADD, &data );
}

void remove_tray_icon( HWND window ) {
    NOTIFYICONDATAW data = tray_data( window );
    Shell_NotifyIconW( NIM_DELETE, &data );
}

void edit_timetable() {
    wstring timetable = join_path( parent_folder( program_path() ), L"timetable.txt" );
    start_process( quoted( program_file( program::notepad_pp ) ) + L" " + quoted( timetable ) );
}

void show_tray_menu( HWND window ) {
    HMENU menu = CreatePopupMenu();
    AppendMenuW( menu, MF_STRING, menu_setups, L"Setups\x2026" );
    AppendMenuW( menu, MF_STRING, menu_timetable, L"Edit timetable" );
    AppendMenuW( menu, MF_STRING | ( shortcut_exists() ? MF_CHECKED : 0 ), menu_start_at_sign_in, L"Start at sign-in" );
    AppendMenuW( menu, MF_SEPARATOR, 0, nullptr );
    AppendMenuW( menu, MF_STRING, menu_quit, L"Quit" );

    POINT cursor;
    GetCursorPos( &cursor );
    SetForegroundWindow( window );   // without this the menu stays open after a click elsewhere
    UINT chosen = TrackPopupMenu( menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, window, nullptr );
    DestroyMenu( menu );

    if ( chosen == menu_setups ) toggle_panel();
    else if ( chosen == menu_timetable ) edit_timetable();
    else if ( chosen == menu_start_at_sign_in ) set_start_at_sign_in( !shortcut_exists() );
    else if ( chosen == menu_quit ) DestroyWindow( window );
}

LRESULT CALLBACK background_proc( HWND window, UINT message, WPARAM wparam, LPARAM lparam ) {
    if ( message == show_panel_message() ) {
        toggle_panel();
        return 0;
    }
    if ( message == taskbar_created_message ) {
        add_tray_icon( window );
        return 0;
    }
    switch ( message ) {
    case tray_message:
        if ( LOWORD( lparam ) == WM_LBUTTONUP ) toggle_panel();
        else if ( LOWORD( lparam ) == WM_RBUTTONUP ) show_tray_menu( window );
        return 0;
    case WM_HOTKEY:
        if ( wparam == hotkey_id ) toggle_panel();
        return 0;
    case WM_DESTROY:
        UnregisterHotKey( window, hotkey_id );
        remove_tray_icon( window );
        PostQuitMessage( 0 );
        return 0;
    }
    return DefWindowProcW( window, message, wparam, lparam );
}

}  // namespace

UINT show_panel_message() {
    static UINT message = RegisterWindowMessageW( L"CrammyShowPanel" );
    return message;
}

bool background_is_running() {
    return find_background_window() != nullptr;
}

bool this_program_is_in_background() {
    DWORD process_id = 0;
    HWND window = find_background_window();
    if ( window ) GetWindowThreadProcessId( window, &process_id );
    return process_id == GetCurrentProcessId();
}

bool ask_running_crammy_to_toggle_panel() {
    HWND window = find_background_window();
    return window && PostMessageW( window, show_panel_message(), 0, 0 );
}

wstring startup_shortcut_path() {
    wchar_t override_folder[MAX_PATH * 4];
    DWORD length = GetEnvironmentVariableW( L"CRAMMY_STARTUP_FOLDER", override_folder, MAX_PATH * 4 );
    if ( length > 0 && length < MAX_PATH * 4 ) return join_path( wstring( override_folder, length ), L"Crammy.lnk" );

    PWSTR folder = nullptr;
    wstring path;
    if ( SUCCEEDED( SHGetKnownFolderPath( FOLDERID_Startup, 0, nullptr, &folder ) ) ) path = folder;
    CoTaskMemFree( folder );
    return path.empty() ? L"" : join_path( path, L"Crammy.lnk" );
}

bool set_start_at_sign_in( bool on ) {
    wstring path = startup_shortcut_path();
    if ( path.empty() ) return false;
    if ( !on ) return DeleteFileW( path.c_str() ) || GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND;
    CreateDirectoryW( parent_folder( path ).c_str(), nullptr );
    return make_shortcut( path );
}

int run_background( bool show_panel_now ) {
    HINSTANCE instance = GetModuleHandleW( nullptr );
    WNDCLASSW window_class = {};
    window_class.lpfnWndProc = background_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = background_class_name;
    RegisterClassW( &window_class );
    HWND window = CreateWindowExW( 0, background_class_name, L"Crammy", WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, instance, nullptr );
    if ( !window ) return 1;

    taskbar_created_message = RegisterWindowMessageW( L"TaskbarCreated" );
    add_tray_icon( window );

    UINT modifiers, key;
    shortcut_keys( modifiers, key );
    RegisterHotKey( window, hotkey_id, modifiers | MOD_NOREPEAT, key );

    if ( show_panel_now ) toggle_panel();
    run_message_loop();
    return 0;
}
