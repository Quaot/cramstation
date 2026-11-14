#include "desktops.h"

#include <windows.h>
#include <objbase.h>

#include <cstdlib>
#include <cstring>

#include "tiling.h"

namespace {

const wchar_t *desktops_key = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktops";

vector<BYTE> read_binary( const wchar_t *value_name ) {
    DWORD size = 0;
    if ( RegGetValueW( HKEY_CURRENT_USER, desktops_key, value_name, RRF_RT_REG_BINARY, nullptr, nullptr, &size ) != ERROR_SUCCESS ) {
        return {};
    }
    vector<BYTE> data( size );
    RegGetValueW( HKEY_CURRENT_USER, desktops_key, value_name, RRF_RT_REG_BINARY, nullptr, data.data(), &size );
    return data;
}

wstring guid_text( const BYTE *bytes ) {
    GUID id;
    memcpy( &id, bytes, sizeof( id ) );
    wchar_t text[40];
    StringFromGUID2( id, text, 40 );
    return text;
}

// Windows keeps the desktops in order as a list of IDs, with each name stored under its ID.
vector<wstring> desktop_ids() {
    vector<BYTE> data = read_binary( L"VirtualDesktopIDs" );
    vector<wstring> ids;
    for ( size_t at = 0; at + 16 <= data.size(); at += 16 ) ids.push_back( guid_text( &data[at] ) );
    return ids;
}

wstring desktop_name( const wstring &id ) {
    wstring key = wstring( desktops_key ) + L"\\Desktops\\" + id;
    wchar_t name[256];
    DWORD size = sizeof( name );
    if ( RegGetValueW( HKEY_CURRENT_USER, key.c_str(), L"Name", RRF_RT_REG_SZ, nullptr, name, &size ) != ERROR_SUCCESS ) {
        return L"";
    }
    return name;
}

wstring current_id() {
    vector<BYTE> data = read_binary( L"CurrentVirtualDesktop" );
    return data.size() >= 16 ? guid_text( data.data() ) : L"";
}

int index_of( const vector<wstring> &ids, const wstring &id ) {
    for ( size_t i = 0; i < ids.size(); ++i ) if ( ids[i] == id ) return static_cast<int>( i );
    return -1;
}

}  // namespace

// A desktop nobody renamed is shown as "Desktop 2" and so on, so it gets that name here too
vector<wstring> desktop_names() {
    vector<wstring> names;
    vector<wstring> ids = desktop_ids();
    for ( size_t i = 0; i < ids.size(); ++i ) {
        wstring name = desktop_name( ids[i] );
        names.push_back( name.empty() ? L"Desktop " + std::to_wstring( i + 1 ) : name );
    }
    return names;
}

wstring switch_to_desktop( const wstring &name ) {
    vector<wstring> ids = desktop_ids();
    vector<wstring> names = desktop_names();
    int target = -1;
    for ( size_t i = 0; i < names.size(); ++i ) {
        if ( lower( names[i] ) == lower( name ) ) target = static_cast<int>( i );
    }
    if ( target < 0 ) return L"There is no desktop named '" + name + L"', so the windows opened on this one.";

    int current = index_of( ids, current_id() );
    if ( current < 0 ) return L"Could not tell which desktop is showing, so the windows opened on this one.";

    unsigned short arrow = target > current ? VK_RIGHT : VK_LEFT;
    for ( int step = 0; step < std::abs( target - current ); ++step ) {
        press_keys( { VK_CONTROL, VK_LWIN, arrow } );
    }

    // The switch animates, so wait until Windows says the target desktop is showing
    for ( int wait = 0; wait < 40 && current_id() != ids[target]; ++wait ) Sleep( 50 );
    Sleep( 200 );
    return current_id() == ids[target] ? L"" : L"Could not switch to the desktop named '" + name + L"'.";
}

// Placeholders until these are built from tests\specs\desktops.md
int current_desktop_index() {
    return -1;
}

vector<window_on_desktop> windows_by_desktop() {
    return {};
}

bool create_desktop( const wstring &, wstring &problem ) {
    problem = L"Creating desktops is not built yet.";
    return false;
}
