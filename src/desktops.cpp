#include "desktops.h"

#include <windows.h>
#include <objbase.h>
#include <servprov.h>
#include <shobjidl.h>
#include <winstring.h>

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

int current_desktop_index() {
    return index_of( desktop_ids(), current_id() );
}

namespace {

const CLSID clsid_desktop_manager = { 0xAA509086, 0x5CA9, 0x4C25, { 0x8F, 0x95, 0x58, 0x9D, 0x3C, 0x07, 0xB4, 0x8A } };
const IID iid_desktop_manager = { 0xA5CD92FF, 0x29BE, 0x454C, { 0x8D, 0x04, 0xD8, 0x28, 0x79, 0xFB, 0x3F, 0x1B } };
const CLSID clsid_immersive_shell = { 0xC2F03A33, 0x21F5, 0x47FA, { 0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39 } };
const GUID sid_desktop_manager_internal = { 0xC5E0CDCA, 0x7B6E, 0x41B2, { 0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B } };
const IID iid_desktop_manager_internal = { 0x53F5CA0B, 0x158F, 0x4124, { 0x90, 0x0C, 0x05, 0x71, 0x58, 0x06, 0x0B, 0x27 } };

// COM must be started on this thread; a thread that already started it in another mode still works
struct com_session {
    bool started = SUCCEEDED( CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED ) );
    ~com_session() { if ( started ) CoUninitialize(); }
};

wstring program_path( HWND window ) {
    DWORD process_id = 0;
    GetWindowThreadProcessId( window, &process_id );
    HANDLE process = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id );
    if ( !process ) return L"";
    wchar_t path[MAX_PATH * 4];
    DWORD size = MAX_PATH * 4;
    wstring result = QueryFullProcessImageNameW( process, 0, path, &size ) ? path : L"";
    CloseHandle( process );
    return result;
}

BOOL CALLBACK add_listed_window( HWND window, LPARAM list ) {
    if ( IsWindowVisible( window ) && GetWindowTextLengthW( window ) > 0 && !GetWindow( window, GW_OWNER ) ) {
        reinterpret_cast<vector<HWND> *>( list )->push_back( window );
    }
    return TRUE;
}

}  // namespace

vector<window_on_desktop> windows_by_desktop() {
    vector<window_on_desktop> result;
    vector<HWND> windows;
    EnumWindows( add_listed_window, reinterpret_cast<LPARAM>( &windows ) );

    com_session com;
    IVirtualDesktopManager *manager = nullptr;
    if ( FAILED( CoCreateInstance( clsid_desktop_manager, nullptr, CLSCTX_ALL, iid_desktop_manager,
                                   reinterpret_cast<void **>( &manager ) ) ) )
        return result;

    vector<wstring> ids = desktop_ids();
    for ( HWND window : windows ) {
        GUID id;
        if ( FAILED( manager->GetWindowDesktopId( window, &id ) ) ) continue;
        int index = index_of( ids, guid_text( reinterpret_cast<const BYTE *>( &id ) ) );
        if ( index >= 0 ) result.push_back( { window, index, program_path( window ) } );
    }
    manager->Release();
    return result;
}

// Explorer's undocumented IVirtualDesktopManagerInternal, IID 53F5CA0B-158F-4124-900C-057158060B27,
// the version for Windows 11 24H2 and later (build 26200 here). The slot order is from MScholtes'
// VirtualDesktop, VirtualDesktop11-24H2.cs (github.com/MScholtes/VirtualDesktop, version 1.21):
//   0 GetCount, 1 MoveViewToDesktop, 2 CanViewMoveDesktops, 3 GetCurrentDesktop, 4 GetDesktops,
//   5 GetAdjacentDesktop, 6 SwitchDesktop, 7 SwitchDesktopAndMoveForegroundView, 8 CreateDesktop,
//   9 MoveDesktop, 10 RemoveDesktop, 11 FindDesktop, 12 GetDesktopSwitchIncludeExcludeViews,
//   13 SetDesktopName, then wallpaper and remote desktop methods that are not used here.
// It sits outside the anonymous namespace so GCC keeps the calls through it.
struct desktop_manager_internal : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE get_count( int * ) = 0;
    virtual HRESULT STDMETHODCALLTYPE move_view_to_desktop( IUnknown *, IUnknown * ) = 0;
    virtual HRESULT STDMETHODCALLTYPE can_view_move_desktops( IUnknown *, BOOL * ) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_current_desktop( IUnknown ** ) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_desktops( IUnknown ** ) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_adjacent_desktop( IUnknown *, int, IUnknown ** ) = 0;
    virtual HRESULT STDMETHODCALLTYPE switch_desktop( IUnknown * ) = 0;
    virtual HRESULT STDMETHODCALLTYPE switch_desktop_and_move_foreground_view( IUnknown * ) = 0;
    virtual HRESULT STDMETHODCALLTYPE create_desktop( IUnknown ** ) = 0;
    virtual HRESULT STDMETHODCALLTYPE move_desktop( IUnknown *, int ) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_desktop( IUnknown *, IUnknown * ) = 0;
    virtual HRESULT STDMETHODCALLTYPE find_desktop( const GUID *, IUnknown ** ) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_desktop_switch_include_exclude_views( IUnknown *, IUnknown **, IUnknown ** ) = 0;
    virtual HRESULT STDMETHODCALLTYPE set_desktop_name( IUnknown *, HSTRING ) = 0;
};

namespace {

// 0 when the desktop was made and named, 1 when it was made but not named, 2 when it was not made
int create_named_desktop( const wstring &name ) {
    com_session com;
    IServiceProvider *shell = nullptr;
    if ( FAILED( CoCreateInstance( clsid_immersive_shell, nullptr, CLSCTX_LOCAL_SERVER, IID_IServiceProvider,
                                   reinterpret_cast<void **>( &shell ) ) ) )
        return 2;
    desktop_manager_internal *manager = nullptr;
    IUnknown *desktop = nullptr;
    HSTRING text = nullptr;
    int outcome = 2;
    if ( SUCCEEDED( shell->QueryService( sid_desktop_manager_internal, iid_desktop_manager_internal, reinterpret_cast<void **>( &manager ) ) ) &&
         SUCCEEDED( manager->create_desktop( &desktop ) ) && desktop ) {
        outcome = SUCCEEDED( WindowsCreateString( name.c_str(), static_cast<UINT32>( name.size() ), &text ) ) &&
                  SUCCEEDED( manager->set_desktop_name( desktop, text ) ) ? 0 : 1;
    }
    WindowsDeleteString( text );
    if ( desktop ) desktop->Release();
    if ( manager ) manager->Release();
    shell->Release();
    return outcome;
}

}  // namespace

// A desktop that was made but not named is left as it is, so pressing Ctrl+Win+D would make a second one
bool create_desktop( const wstring &name, wstring &problem ) {
    problem.clear();
    int outcome = create_named_desktop( name );
    if ( outcome == 0 ) return true;
    if ( outcome == 2 ) press_keys( { VK_CONTROL, VK_LWIN, 'D' } );
    problem = L"A new desktop was made, but it needs the name " + name + L". Right-click it in Task View and choose Rename.";
    return true;
}
