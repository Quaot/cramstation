#include "notes_tabs.h"

#include <windows.h>

#include "profiles.h"
#include "tiling.h"

namespace {

// Commands Notepad++ accepts from other programs (from its Notepad_plus_msgs.h).
// They work on its window directly: nothing comes to the front and no keys are pressed.
const UINT npp_message = WM_USER + 1000;
const UINT npp_open_file_count = npp_message + 7;
const UINT npp_switch_to_tab = npp_message + 28;
const UINT npp_save_tab = npp_message + 38;
const UINT npp_menu_command = npp_message + 48;
const LPARAM npp_close_tab = 41003;
const WPARAM main_view = 0, second_view = 1;

wstring list_path() {
    return join_path( parent_folder( profiles_path() ), L"notes opened.txt" );
}

vector<wstring> read_list() {
    vector<wstring> paths;
    wstring text = read_text_file( list_path() );
    size_t start = 0;
    while ( start < text.size() ) {
        size_t end = text.find( L'\n', start );
        if ( end == wstring::npos ) end = text.size();
        wstring line = trim( text.substr( start, end - start ) );
        if ( !line.empty() ) paths.push_back( line );
        start = end + 1;
    }
    return paths;
}

void write_list( const vector<wstring> &paths ) {
    wstring text;
    for ( const wstring &path : paths ) text += path + L"\r\n";
    write_text_file( list_path(), text );
}

bool listed( const vector<wstring> &paths, const wstring &path ) {
    for ( const wstring &each : paths ) if ( lower( each ) == lower( path ) ) return true;
    return false;
}

// Notepad++ titles itself "*C:\folder\file.txt - Notepad++", with * while there are unsaved changes
wstring shown_file( HWND window ) {
    wstring title = title_of( window );
    if ( starts_with( title, L"*" ) ) title.erase( 0, 1 );
    size_t end = title.rfind( L" - Notepad++" );
    return end == wstring::npos ? title : title.substr( 0, end );
}

bool is_file_shown( HWND window, const wstring &path ) {
    wstring shown = shown_file( window );
    if ( shown.find( L'\\' ) == wstring::npos ) return lower( shown ) == lower( file_name( path ) );
    return lower( shown ) == lower( path );
}

}  // namespace

void remember_notes( const wstring &path ) {
    vector<wstring> paths = read_list();
    if ( listed( paths, path ) ) return;
    paths.push_back( path );
    write_list( paths );
}

void close_old_notes( const vector<wstring> &keep, HWND window ) {
    vector<wstring> paths = read_list();
    if ( !window ) window = FindWindowW( L"Notepad++", nullptr );
    if ( paths.empty() || !window ) {   // with Notepad++ closed, no tabs are left to close
        if ( !window ) write_list( {} );
        return;
    }

    wstring was_showing = shown_file( window );
    vector<wstring> still_open;
    for ( WPARAM view : { main_view, second_view } ) {
        int count = int( SendMessageW( window, npp_open_file_count, 0, view == main_view ? 1 : 2 ) );
        // Going from the last tab back means closing one does not move the ones still to check
        for ( int tab = count - 1; tab >= 0; --tab ) {
            SendMessageW( window, npp_switch_to_tab, view, tab );
            wstring path = shown_file( window );
            if ( !listed( paths, path ) || listed( keep, path ) ) continue;

            SendMessageW( window, npp_save_tab, 0, 0 );
            if ( starts_with( title_of( window ), L"*" ) || !is_file_shown( window, path ) ) {
                still_open.push_back( path );   // it did not save, so the tab stays
                continue;
            }
            SendMessageW( window, npp_menu_command, 0, npp_close_tab );
        }
    }

    write_list( still_open );

    // Put back the tab that was showing before, if it is still open
    for ( WPARAM view : { main_view, second_view } ) {
        int count = int( SendMessageW( window, npp_open_file_count, 0, view == main_view ? 1 : 2 ) );
        for ( int tab = 0; tab < count; ++tab ) {
            SendMessageW( window, npp_switch_to_tab, view, tab );
            if ( lower( shown_file( window ) ) == lower( was_showing ) ) return;
        }
    }
}
