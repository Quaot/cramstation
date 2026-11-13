#include "tiling.h"

#include <windows.h>
#include <dwmapi.h>

#include <cstdlib>
#include <set>

namespace {

const wchar_t *pdfcraft_exe = L"C:\\Program Files\\PdfCraft\\pdfcraft.exe";

struct live_window {
    planned_window plan;
    RECT target{};
    int rows = 1;               // windows sharing this side
    DWORD process_id = 0;
    HWND hwnd = nullptr;
};

DWORD start_process( wstring command_line ) {
    STARTUPINFOW startup{ sizeof( startup ) };
    PROCESS_INFORMATION info{};
    if ( !CreateProcessW( nullptr, command_line.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &info ) ) {
        return 0;
    }
    CloseHandle( info.hThread );
    CloseHandle( info.hProcess );
    return info.dwProcessId;
}

wstring quoted( const wstring &text ) {
    return L"\"" + text + L"\"";
}

vector<HWND> top_windows() {
    vector<HWND> windows;
    EnumWindows( []( HWND hwnd, LPARAM list ) -> BOOL {
        if ( IsWindowVisible( hwnd ) && !GetWindow( hwnd, GW_OWNER ) && GetWindowTextLengthW( hwnd ) > 0 ) {
            reinterpret_cast<vector<HWND> *>( list )->push_back( hwnd );
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>( &windows ) );
    return windows;
}

wstring title_of( HWND hwnd ) {
    wchar_t title[512];
    GetWindowTextW( hwnd, title, 512 );
    return title;
}

bool is_explorer( HWND hwnd ) {
    wchar_t class_name[64];
    GetClassNameW( hwnd, class_name, 64 );
    return wstring( class_name ) == L"CabinetWClass";
}

HWND find_window( const live_window &window, const std::set<HWND> &taken ) {
    for ( HWND hwnd : top_windows() ) {
        if ( taken.count( hwnd ) ) continue;
        if ( window.plan.app == program::pdfcraft ) {
            DWORD process_id = 0;
            GetWindowThreadProcessId( hwnd, &process_id );
            if ( process_id == window.process_id ) return hwnd;
        } else if ( window.plan.app == program::notepad ) {
            wstring title = title_of( hwnd );
            if ( contains_ci( title, file_name( window.plan.paths[0] ) ) && contains_ci( title, L"Notepad" ) ) return hwnd;
        } else if ( is_explorer( hwnd ) && contains_ci( title_of( hwnd ), file_name( window.plan.paths[0] ) ) ) {
            return hwnd;
        }
    }
    return nullptr;
}

// Windows 11 adds invisible borders around a window, so size the visible frame instead.
RECT visible_frame( HWND hwnd ) {
    RECT frame;
    if ( FAILED( DwmGetWindowAttribute( hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof( frame ) ) ) ) {
        GetWindowRect( hwnd, &frame );
    }
    return frame;
}

bool is_in_place( HWND hwnd, const RECT &target, int tolerance = 2 ) {
    RECT frame = visible_frame( hwnd );
    return std::abs( frame.left - target.left ) <= tolerance && std::abs( frame.top - target.top ) <= tolerance &&
           std::abs( frame.right - target.right ) <= tolerance && std::abs( frame.bottom - target.bottom ) <= tolerance;
}

void press_keys( std::initializer_list<WORD> keys ) {
    vector<INPUT> inputs;
    for ( WORD key : keys ) {
        INPUT down{ INPUT_KEYBOARD };
        down.ki.wVk = key;
        inputs.push_back( down );
    }
    for ( auto key = keys.end(); key != keys.begin(); ) {
        INPUT up{ INPUT_KEYBOARD };
        up.ki.wVk = *--key;
        up.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back( up );
    }
    SendInput( static_cast<UINT>( inputs.size() ), inputs.data(), sizeof( INPUT ) );
    Sleep( 250 );
}

// Windows only lets the program that sent the last key press change which window is in front.
// F24 is on no keyboard, so pressing it changes nothing else.
bool bring_to_front( HWND hwnd ) {
    press_keys( { VK_F24 } );
    SetForegroundWindow( hwnd );
    Sleep( 100 );
    return GetForegroundWindow() == hwnd;
}

// Snapping animates, so wait until the frame has stopped changing.
void wait_until_still( HWND hwnd ) {
    RECT last = visible_frame( hwnd );
    int still_checks = 0;
    for ( int i = 0; i < 30 && still_checks < 3; ++i ) {
        Sleep( 50 );
        RECT now = visible_frame( hwnd );
        still_checks = EqualRect( &now, &last ) ? still_checks + 1 : 0;
        last = now;
    }
}

// Snaps a window that has a side to itself, using the keys a person would press,
// so Windows treats it as snapped. Stacked windows are only moved into place.
void snap( const live_window &window ) {
    if ( window.rows != 1 ) return;
    if ( window.plan.side == L"full" ) {
        ShowWindow( window.hwnd, SW_MAXIMIZE );
        return;
    }
    if ( !bring_to_front( window.hwnd ) ) return;
    ShowWindow( window.hwnd, SW_RESTORE );   // Win+Left on a window that is already snapped moves it on
    Sleep( 150 );
    press_keys( { VK_LWIN, static_cast<WORD>( window.plan.side == L"left" ? VK_LEFT : VK_RIGHT ) } );
}

void move_window( HWND hwnd, const RECT &target ) {
    if ( IsIconic( hwnd ) || IsZoomed( hwnd ) ) ShowWindow( hwnd, SW_RESTORE );
    RECT outer;
    GetWindowRect( hwnd, &outer );
    RECT frame = visible_frame( hwnd );
    int left_border = frame.left - outer.left, top_border = frame.top - outer.top;
    int right_border = outer.right - frame.right, bottom_border = outer.bottom - frame.bottom;
    SetWindowPos( hwnd, nullptr,
                  target.left - left_border, target.top - top_border,
                  target.right - target.left + left_border + right_border,
                  target.bottom - target.top + top_border + bottom_border,
                  SWP_NOZORDER | SWP_NOACTIVATE );
}

RECT side_area( const wstring &side, const RECT &screen ) {
    int middle = screen.left + ( screen.right - screen.left ) / 2;
    if ( side == L"left" ) return { screen.left, screen.top, middle, screen.bottom };
    if ( side == L"right" ) return { middle, screen.top, screen.right, screen.bottom };
    return screen;
}

}  // namespace

vector<wstring> open_windows( const vector<planned_window> &plan ) {
    RECT screen;
    SystemParametersInfoW( SPI_GETWORKAREA, 0, &screen, 0 );

    vector<live_window> windows;
    for ( const planned_window &each : plan ) windows.push_back( { each } );

    // Windows that share a side split it into rows, in the order they were written
    for ( const wchar_t *side : { L"left", L"right", L"full" } ) {
        vector<live_window *> on_side;
        for ( live_window &window : windows ) if ( window.plan.side == side ) on_side.push_back( &window );
        RECT area = side_area( side, screen );
        int row_height = on_side.empty() ? 0 : ( area.bottom - area.top ) / static_cast<int>( on_side.size() );
        for ( size_t row = 0; row < on_side.size(); ++row ) {
            int top = area.top + static_cast<int>( row ) * row_height;
            int bottom = ( row + 1 == on_side.size() ) ? area.bottom : top + row_height;
            on_side[row]->target = { area.left, top, area.right, bottom };
            on_side[row]->rows = static_cast<int>( on_side.size() );
        }
    }

    for ( live_window &window : windows ) {
        if ( window.plan.app == program::pdfcraft ) {
            wstring command = quoted( pdfcraft_exe ) + L" --new-window";
            for ( const wstring &path : window.plan.paths ) command += L" " + quoted( path );
            window.process_id = start_process( command );
        } else if ( window.plan.app == program::notepad ) {
            start_process( L"notepad.exe " + quoted( window.plan.paths[0] ) );
        } else {
            start_process( L"explorer.exe " + quoted( window.plan.paths[0] ) );
        }
    }

    // Apps can restore their own saved size just after they appear,
    // so keep checking until every window has stayed in place for a moment.
    std::set<HWND> taken;
    int quiet_checks = 0;
    DWORD started = GetTickCount();
    while ( GetTickCount() - started < 15000 && quiet_checks < 4 ) {
        bool settled = true;
        for ( live_window &window : windows ) {
            if ( !window.hwnd ) {
                window.hwnd = find_window( window, taken );
                if ( window.hwnd ) taken.insert( window.hwnd );
            }
            if ( !window.hwnd ) { settled = false; continue; }
            if ( !is_in_place( window.hwnd, window.target ) ) {
                move_window( window.hwnd, window.target );
                settled = false;
            }
        }
        quiet_checks = settled ? quiet_checks + 1 : 0;
        Sleep( 100 );
    }

    // Snap the left side first, so the right side fills the space next to it.
    // A window that did not snap into place is put there directly instead.
    for ( const wchar_t *side : { L"left", L"right", L"full" } ) {
        for ( live_window &window : windows ) {
            if ( window.plan.side != side || !window.hwnd ) continue;
            snap( window );
            wait_until_still( window.hwnd );
            if ( !is_in_place( window.hwnd, window.target, 8 ) ) {
                ShowWindow( window.hwnd, SW_RESTORE );
                move_window( window.hwnd, window.target );
            }
        }
    }

    vector<wstring> problems;
    for ( const live_window &window : windows ) {
        if ( !window.hwnd ) problems.push_back( L"No window appeared for " + file_name( window.plan.paths[0] ) );
    }
    for ( const live_window &window : windows ) {
        if ( window.hwnd && window.plan.app == program::notepad ) bring_to_front( window.hwnd );
    }
    return problems;
}
