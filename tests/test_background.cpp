// Written from tests\specs\background.md.

#include <windows.h>
#include <objbase.h>
#include <shlobj.h>

#include "check.h"
#include "../src/background.h"
#include "../src/text.h"

namespace {

int toggle_count = 0;

LRESULT CALLBACK counting_proc( HWND window, UINT message, WPARAM w_param, LPARAM l_param ) {
    if ( message == show_panel_message() ) {
        toggle_count++;
        return 0;
    }
    return DefWindowProcW( window, message, w_param, l_param );
}

void pump_messages() {
    MSG message;
    while ( PeekMessageW( &message, nullptr, 0, 0, PM_REMOVE ) ) {
        TranslateMessage( &message );
        DispatchMessageW( &message );
    }
}

wstring test_program_folder() {
    wchar_t path[MAX_PATH * 4];
    DWORD length = GetModuleFileNameW( nullptr, path, MAX_PATH * 4 );
    return parent_folder( wstring( path, length ) );
}

struct link_contents {
    bool loaded = false;
    wstring target;
    wstring arguments;
};

link_contents read_link( const wstring &path ) {
    link_contents contents;
    IShellLinkW *link = nullptr;
    if ( FAILED( CoCreateInstance( CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &link ) ) ) ) return contents;
    IPersistFile *file = nullptr;
    if ( SUCCEEDED( link->QueryInterface( IID_PPV_ARGS( &file ) ) ) ) {
        if ( SUCCEEDED( file->Load( path.c_str(), STGM_READ ) ) ) {
            contents.loaded = true;
            wchar_t target[MAX_PATH * 4] = {};
            wchar_t arguments[MAX_PATH * 4] = {};
            link->GetPath( target, MAX_PATH * 4, nullptr, SLGP_RAWPATH );
            link->GetArguments( arguments, MAX_PATH * 4 );
            contents.target = target;
            contents.arguments = arguments;
        }
        file->Release();
    }
    link->Release();
    return contents;
}

int files_in_folder( const wstring &folder ) {
    WIN32_FIND_DATAW found;
    HANDLE search = FindFirstFileW( join_path( folder, L"*" ).c_str(), &found );
    if ( search == INVALID_HANDLE_VALUE ) return 0;
    int count = 0;
    do {
        if ( !( found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) count++;
    } while ( FindNextFileW( search, &found ) );
    FindClose( search );
    return count;
}

void check_sending_the_message() {
    if ( FindWindowW( L"CrammyBackground", nullptr ) ) {
        check( true, "A real Crammy is running in the background, so the message checks are skipped." );
        return;
    }

    check( !background_is_running(), "No background window is reported before one exists." );
    check( !ask_running_crammy_to_toggle_panel(), "Asking a Crammy that is not running to toggle the panel returns false." );
    check( show_panel_message() != 0, "The show panel message is not zero." );
    check( show_panel_message() == show_panel_message(), "The show panel message is the same on every call." );

    HINSTANCE instance = GetModuleHandleW( nullptr );
    WNDCLASSW window_class = {};
    window_class.lpfnWndProc = counting_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"CrammyBackground";
    RegisterClassW( &window_class );
    HWND window = CreateWindowExW( 0, L"CrammyBackground", L"", WS_OVERLAPPED, 0, 0, 100, 100, nullptr, nullptr, instance, nullptr );
    check( window != nullptr, "The test made its hidden background window." );

    toggle_count = 0;
    check( background_is_running(), "A background window is reported once one exists." );
    check( ask_running_crammy_to_toggle_panel(), "Asking a running Crammy to toggle the panel returns true." );
    pump_messages();
    check( toggle_count == 1, "The background window received the toggle message once." );
    ask_running_crammy_to_toggle_panel();
    ask_running_crammy_to_toggle_panel();
    pump_messages();
    check( toggle_count == 3, "Two more requests bring the count to three." );

    DestroyWindow( window );
    check( !background_is_running(), "No background window is reported after the test destroys it." );
    check( !ask_running_crammy_to_toggle_panel(), "Asking after the window is gone returns false." );
    UnregisterClassW( L"CrammyBackground", instance );
}

void check_start_at_sign_in() {
    wstring folder = join_path( expand_variables( L"%TEMP%" ), L"crammy startup check" );
    CreateDirectoryW( folder.c_str(), nullptr );
    wstring shortcut = join_path( folder, L"Crammy.lnk" );
    DeleteFileW( shortcut.c_str() );
    SetEnvironmentVariableW( L"CRAMMY_STARTUP_FOLDER", folder.c_str() );

    check( lower( startup_shortcut_path() ) == lower( shortcut ), "The shortcut path is Crammy.lnk inside the startup folder that was set." );
    check( !is_file( shortcut ), "The shortcut does not exist before it is asked for." );

    check( set_start_at_sign_in( true ), "Turning start at sign-in on returns true." );
    check( is_file( shortcut ), "Turning it on creates Crammy.lnk in the startup folder." );

    link_contents contents = read_link( shortcut );
    check( contents.loaded, "The file made is a shortcut that can be read back." );
    check( lower( contents.target ) == lower( join_path( test_program_folder(), L"crammy.exe" ) ),
           "The shortcut points at crammy.exe in the folder of the running program." );
    check( contents.arguments == L"--background", "The shortcut passes the argument --background." );

    check( set_start_at_sign_in( true ), "Turning it on a second time returns true." );
    check( is_file( shortcut ) && files_in_folder( folder ) == 1, "Turning it on twice leaves exactly one file." );

    check( set_start_at_sign_in( false ), "Turning start at sign-in off returns true." );
    check( !is_file( shortcut ), "Turning it off removes the shortcut." );
    check( set_start_at_sign_in( false ), "Turning it off when there is no shortcut also returns true." );
    check( !is_file( shortcut ), "There is still no shortcut after turning it off twice." );

    SetEnvironmentVariableW( L"CRAMMY_STARTUP_FOLDER", nullptr );
    check( GetEnvironmentVariableW( L"CRAMMY_STARTUP_FOLDER", nullptr, 0 ) == 0, "The startup folder variable is unset at the end." );
    RemoveDirectoryW( folder.c_str() );
}

} // namespace

void test_background() {
    HRESULT started = CoInitialize( nullptr );
    check_sending_the_message();
    check_start_at_sign_in();
    if ( SUCCEEDED( started ) ) CoUninitialize();
}
