#include "notes_tabs.h"

#include <windows.h>

#include <functional>

#include "profiles.h"
#include "tiling.h"

namespace {

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

HWND notepad_showing( const wstring &name ) {
    for ( HWND hwnd : top_windows() ) {
        wstring title = title_of( hwnd );
        if ( contains_ci( title, L"Notepad" ) && ( name.empty() || contains_ci( title, name ) ) ) return hwnd;
    }
    return nullptr;
}

bool wait_for( const std::function<bool()> &done, int milliseconds ) {
    for ( int waited = 0; waited < milliseconds; waited += 100 ) {
        if ( done() ) return true;
        Sleep( 100 );
    }
    return done();
}

// Each step checks the window before the next key press, so a surprise leaves the tab open.
bool save_and_close( const wstring &path ) {
    wstring name = file_name( path );
    start_process( L"notepad.exe " + quoted( path ) );   // brings that file's tab to the front

    HWND hwnd = nullptr;
    if ( !wait_for( [&] { return ( hwnd = notepad_showing( name ) ) != nullptr; }, 3000 ) ) return false;
    if ( !bring_to_front( hwnd ) ) return false;

    press_keys( { VK_CONTROL, 'S' } );
    // Notepad starts the title with * while the file has unsaved changes
    if ( !wait_for( [&] { return !starts_with( title_of( hwnd ), L"*" ); }, 2000 ) ) return false;
    if ( GetForegroundWindow() != hwnd || !contains_ci( title_of( hwnd ), name ) ) return false;

    press_keys( { VK_CONTROL, 'W' } );
    return wait_for( [&] { return !IsWindow( hwnd ) || !contains_ci( title_of( hwnd ), name ); }, 2000 );
}

}  // namespace

void remember_notes( const wstring &path ) {
    vector<wstring> paths = read_list();
    if ( listed( paths, path ) ) return;
    paths.push_back( path );
    write_list( paths );
}

void close_old_notes( const vector<wstring> &keep ) {
    vector<wstring> paths = read_list();
    if ( paths.empty() ) return;
    if ( !notepad_showing( L"" ) ) {   // Notepad is closed, so no tabs are left to close
        write_list( {} );
        return;
    }
    vector<wstring> still_open;
    for ( const wstring &path : paths ) {
        if ( listed( keep, path ) ) still_open.push_back( path );
        else if ( is_file( path ) && !save_and_close( path ) ) still_open.push_back( path );
    }
    write_list( still_open );
}
