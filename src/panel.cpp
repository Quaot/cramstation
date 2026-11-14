#include "panel.h"

#include <windowsx.h>

#include <map>

#include "dialogs.h"
#include "launch.h"
#include "profiles.h"
#include "settings.h"
#include "subjects.h"
#include "tiling.h"
#include "ui.h"

namespace {

const wchar_t *panel_class_name = L"CrammyPanel";

const COLORREF ink = RGB( 11, 11, 12 );
const COLORREF cream = RGB( 239, 233, 220 );
const COLORREF red = RGB( 183, 2, 40 );
const COLORREF grey = RGB( 150, 145, 136 );

const int panel_width = 300;
const int margin = 12;
const int header_height = 22;
const int row_height = 18;
const int chips_per_row = 4;
const int chip_height = 36;
const int chip_gap = 4;
const int icon_size = 16;
const int icon_gap = 2;
const int max_icons = 3;
const int link_height = 20;
const int link_gap = 10;

enum class link { long_pomodoro, short_pomodoro, setups, none };
const wchar_t *link_labels[] = { L"Pomodoro 3h", L"1.5h", L"Setups\x2026" };

HFONT panel_font( UINT dpi, const wchar_t *face, int points, int weight = FW_NORMAL, bool underline = false ) {
    LOGFONTW font{};
    font.lfHeight = -MulDiv( points, static_cast<int>( dpi ), 72 );
    font.lfWeight = weight;
    font.lfUnderline = underline;
    font.lfQuality = CLEARTYPE_QUALITY;
    lstrcpynW( font.lfFaceName, face, LF_FACESIZE );
    return CreateFontIndirectW( &font );
}

struct panel_fonts {
    HFONT date, text, strong, hovered;

    explicit panel_fonts( UINT dpi )
        : date( panel_font( dpi, L"Georgia", 11 ) ), text( panel_font( dpi, L"Segoe UI", 9 ) ),
          strong( panel_font( dpi, L"Segoe UI", 9, FW_SEMIBOLD ) ),
          hovered( panel_font( dpi, L"Segoe UI", 9, FW_NORMAL, true ) ) {}
    ~panel_fonts() {
        for ( HFONT font : { date, text, strong, hovered } ) DeleteObject( font );
    }
};

int text_width( HDC dc, HFONT font, const wstring &text ) {
    HGDIOBJ old_font = SelectObject( dc, font );
    SIZE size{};
    GetTextExtentPoint32W( dc, text.c_str(), int( text.size() ), &size );
    SelectObject( dc, old_font );
    return size.cx;
}

struct panel_layout {
    RECT header;
    vector<RECT> rows;
    RECT message;
    int rule_after_today;
    vector<RECT> chips;
    int rule_after_chips;
    RECT links[3];
    int height;
};

panel_layout lay_out_panel( const today_section &today, const vector<desktop_tile> &tiles, int width, UINT dpi ) {
    auto px = [dpi]( int value ) { return scaled( value, dpi ); };
    panel_layout layout{};
    int left = px( margin ), right = width - px( margin );

    layout.header = { left, px( 6 ), right, px( 6 ) + px( header_height ) };
    int y = layout.header.bottom + px( 4 );
    for ( size_t i = 0; i < today.rows.size(); ++i ) {
        layout.rows.push_back( { 0, y, width, y + px( row_height ) } );
        y += px( row_height );
    }
    if ( !today.message.empty() ) {
        layout.message = { left, y, right, y + px( row_height ) };
        y += px( row_height );
    }

    layout.rule_after_today = y + px( 6 );
    y = layout.rule_after_today + px( 7 );
    int chip_width = ( right - left - ( chips_per_row - 1 ) * px( chip_gap ) ) / chips_per_row;
    for ( size_t i = 0; i < tiles.size(); ++i ) {
        int column = int( i % chips_per_row ), line = int( i / chips_per_row );
        int chip_left = left + column * ( chip_width + px( chip_gap ) );
        int chip_top = y + line * ( px( chip_height ) + px( chip_gap ) );
        layout.chips.push_back( { chip_left, chip_top, chip_left + chip_width, chip_top + px( chip_height ) } );
    }
    int chip_lines = int( ( tiles.size() + chips_per_row - 1 ) / chips_per_row );
    if ( chip_lines > 0 ) y += chip_lines * px( chip_height ) + ( chip_lines - 1 ) * px( chip_gap );

    layout.rule_after_chips = y + px( 7 );
    y = layout.rule_after_chips + px( 4 );

    panel_fonts fonts( dpi );
    HDC screen = GetDC( nullptr );
    int x = left;
    for ( int i = 0; i < 2; ++i ) {
        int link_width = text_width( screen, fonts.text, link_labels[i] );
        layout.links[i] = { x, y, x + link_width, y + px( link_height ) };
        x += link_width + px( link_gap );
    }
    int setups_width = text_width( screen, fonts.text, link_labels[2] );
    layout.links[2] = { right - setups_width, y, right, y + px( link_height ) };
    ReleaseDC( nullptr, screen );

    layout.height = y + px( link_height ) + px( 5 );
    return layout;
}

HICON icon_of( const wstring &program_path, int size ) {
    static std::map<std::pair<wstring, int>, HICON> icons;
    auto found = icons.find( { program_path, size } );
    if ( found != icons.end() ) return found->second;
    HICON icon = nullptr;
    PrivateExtractIconsW( program_path.c_str(), 0, size, size, &icon, nullptr, 1, 0 );
    icons[{ program_path, size }] = icon;
    return icon;
}

void fill( HDC dc, const RECT &box, COLORREF colour ) {
    HBRUSH brush = CreateSolidBrush( colour );
    FillRect( dc, &box, brush );
    DeleteObject( brush );
}

void outline( HDC dc, const RECT &box, COLORREF colour, int thickness ) {
    fill( dc, { box.left, box.top, box.right, box.top + thickness }, colour );
    fill( dc, { box.left, box.bottom - thickness, box.right, box.bottom }, colour );
    fill( dc, { box.left, box.top, box.left + thickness, box.bottom }, colour );
    fill( dc, { box.right - thickness, box.top, box.right, box.bottom }, colour );
}

void rule( HDC dc, int y, int width, UINT dpi ) {
    fill( dc, { scaled( margin, dpi ), y, width - scaled( margin, dpi ), y + scaled( 1, dpi ) }, ink );
}

void write( HDC dc, const wstring &text, RECT box, HFONT font, COLORREF colour, UINT format ) {
    SelectObject( dc, font );
    SetTextColor( dc, colour );
    DrawTextW( dc, text.c_str(), int( text.size() ), &box, format | DT_NOPREFIX );
}

wstring clock_text( int minute ) {
    wchar_t text[8];
    swprintf( text, 8, L"%02d:%02d", minute / 60, minute % 60 );
    return text;
}

// "in 5 min" becomes "5m"; "now" stays as it is
wstring short_note( const wstring &note ) {
    if ( !starts_with( note, L"in " ) ) return note;
    wstring minutes = note.substr( 3 );
    size_t space = minutes.find( L' ' );
    return minutes.substr( 0, space ) + L"m";
}

void draw_row( HDC dc, const today_row &row, const RECT &box, int width, const panel_fonts &fonts, UINT dpi ) {
    auto px = [dpi]( int value ) { return scaled( value, dpi ); };
    COLORREF colour = row.marked ? red : row.known ? ink : grey;
    UINT format = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;
    int left = px( margin );
    if ( row.marked ) fill( dc, { 0, box.top + px( 3 ), px( 4 ), box.bottom - px( 3 ) }, red );

    const class_meeting &meeting = row.meeting;
    HFONT font = row.marked ? fonts.strong : fonts.text;
    write( dc, clock_text( meeting.start_minute ), { left, box.top, left + px( 38 ), box.bottom }, font, colour, format );
    write( dc, meeting.course, { left + px( 42 ), box.top, left + px( 104 ), box.bottom }, font, colour, format );
    write( dc, lower( meeting.kind ), { left + px( 108 ), box.top, left + px( 136 ), box.bottom }, font, colour, format );
    if ( !row.marked ) return;
    write( dc, meeting.room, { left + px( 140 ), box.top, left + px( 222 ), box.bottom }, fonts.text, colour, format );
    write( dc, short_note( row.note ), { left + px( 222 ), box.top, width - px( margin ), box.bottom }, fonts.text,
           colour, format | DT_RIGHT );
}

void draw_chip( HDC dc, const desktop_tile &tile, const RECT &box, const panel_fonts &fonts, UINT dpi ) {
    auto px = [dpi]( int value ) { return scaled( value, dpi ); };
    if ( tile.current ) outline( dc, box, red, px( 2 ) );
    else outline( dc, box, ink, px( 1 ) );

    write( dc, tile.name, { box.left + px( 5 ), box.top + px( 3 ), box.right - px( 4 ), box.top + px( 18 ) },
           tile.current ? fonts.strong : fonts.text, tile.current ? red : ink,
           DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS );

    int size = px( icon_size );
    int x = box.left + px( 5 ), y = box.bottom - px( 4 ) - size;
    for ( size_t i = 0; i < tile.program_paths.size() && int( i ) < max_icons; ++i ) {
        HICON icon = icon_of( tile.program_paths[i], size );
        if ( icon ) DrawIconEx( dc, x, y, icon, size, size, 0, nullptr, DI_NORMAL );
        x += size + px( icon_gap );
    }
}

void draw_contents( HDC dc, int width, int height, const today_section &today, const vector<desktop_tile> &tiles,
                    const wstring &date_text, const wstring &time_text, UINT dpi, link hovered ) {
    panel_layout layout = lay_out_panel( today, tiles, width, dpi );
    panel_fonts fonts( dpi );
    HGDIOBJ old_font = GetCurrentObject( dc, OBJ_FONT );

    fill( dc, { 0, 0, width, height }, cream );
    SetBkMode( dc, TRANSPARENT );

    write( dc, date_text, layout.header, fonts.date, ink, DT_SINGLELINE | DT_VCENTER | DT_LEFT );
    write( dc, time_text, layout.header, fonts.date, ink, DT_SINGLELINE | DT_VCENTER | DT_RIGHT );

    for ( size_t i = 0; i < today.rows.size(); ++i ) draw_row( dc, today.rows[i], layout.rows[i], width, fonts, dpi );
    if ( !today.message.empty() )
        write( dc, today.message, layout.message, fonts.text, ink, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS );
    rule( dc, layout.rule_after_today, width, dpi );

    for ( size_t i = 0; i < tiles.size(); ++i ) draw_chip( dc, tiles[i], layout.chips[i], fonts, dpi );
    rule( dc, layout.rule_after_chips, width, dpi );

    for ( int i = 0; i < 3; ++i )
        write( dc, link_labels[i], layout.links[i], link( i ) == hovered ? fonts.hovered : fonts.text, red,
               DT_SINGLELINE | DT_VCENTER );

    SelectObject( dc, old_font );
}

// ---------------------------------------------------------------------------
// The window
// ---------------------------------------------------------------------------

struct panel_state {
    HWND window = nullptr;
    UINT dpi = 96;
    today_section today;
    vector<desktop_tile> tiles;
    vector<wstring> names;
    crammy_settings settings;
    wstring date, time, problem;
    link hovered = link::none;
    ULONGLONG closed_at = 0;
};

panel_state panel;

const wchar_t *day_names[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
const wchar_t *month_names[] = { L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
                                 L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };

vector<wstring> known_courses( const vector<class_block> &blocks ) {
    wstring downloads = join_path( user_folder(), L"Downloads" );
    vector<wstring> courses, known;
    for ( const class_block &block : blocks ) {
        bool seen = false;
        for ( const wstring &course : courses ) seen = seen || same_course( course, block.course );
        if ( seen ) continue;
        courses.push_back( block.course );
        if ( has_course_folder( block.course, downloads ) ) known.push_back( block.course );
    }
    return known;
}

void read_panel_data() {
    SYSTEMTIME now;
    GetLocalTime( &now );
    panel.date = wstring( day_names[now.wDayOfWeek] ) + L" " + month_names[now.wMonth - 1] + L" " +
                 std::to_wstring( now.wDay );
    int minute_now = now.wHour * 60 + now.wMinute;
    panel.time = clock_text( minute_now );

    vector<wstring> errors;
    wstring timetable = timetable_path();
    vector<class_block> blocks;
    if ( is_file( timetable ) ) blocks = read_timetable( read_text_file( timetable ), errors );
    else errors.push_back( L"cannot find " + timetable );
    calendar_date today{ now.wYear, now.wMonth, now.wDay };
    panel.today = build_today( classes_on( blocks, today ), minute_now, known_courses( blocks ), errors );

    vector<wstring> settings_errors;
    panel.settings = read_settings( read_text_file( settings_path() ), settings_errors );
    // A timetable error comes first; a settings error takes the place of "No classes today".
    if ( errors.empty() && !settings_errors.empty() ) panel.today.message = L"settings.txt: " + settings_errors[0];

    panel.names = desktop_names();
    panel.tiles = build_tiles( panel.names, current_desktop_index(), windows_by_desktop() );
}

int problem_height( int width ) {
    if ( panel.problem.empty() ) return 0;
    panel_fonts fonts( panel.dpi );
    HDC dc = GetDC( nullptr );
    HGDIOBJ old_font = SelectObject( dc, fonts.text );
    RECT box{ 0, 0, width - 2 * scaled( margin, panel.dpi ), 0 };
    DrawTextW( dc, panel.problem.c_str(), int( panel.problem.size() ), &box, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX );
    SelectObject( dc, old_font );
    ReleaseDC( nullptr, dc );
    return box.bottom + scaled( 12, panel.dpi );
}

void paint( HWND window ) {
    PAINTSTRUCT paint_info;
    HDC screen = BeginPaint( window, &paint_info );
    RECT client;
    GetClientRect( window, &client );
    HDC dc = CreateCompatibleDC( screen );
    HBITMAP bitmap = CreateCompatibleBitmap( screen, client.right, client.bottom );
    HGDIOBJ old_bitmap = SelectObject( dc, bitmap );

    fill( dc, client, cream );
    int top = panel_height( panel.today, panel.tiles, panel.dpi );
    draw_contents( dc, client.right, top, panel.today, panel.tiles, panel.date, panel.time, panel.dpi, panel.hovered );
    if ( !panel.problem.empty() ) {
        panel_fonts fonts( panel.dpi );
        HGDIOBJ old_font = GetCurrentObject( dc, OBJ_FONT );
        SetBkMode( dc, TRANSPARENT );
        rule( dc, top, client.right, panel.dpi );
        RECT box{ scaled( margin, panel.dpi ), top + scaled( 6, panel.dpi ), client.right - scaled( margin, panel.dpi ),
                  client.bottom };
        write( dc, panel.problem, box, fonts.text, ink, DT_WORDBREAK );
        SelectObject( dc, old_font );
    }

    BitBlt( screen, 0, 0, client.right, client.bottom, dc, 0, 0, SRCCOPY );
    SelectObject( dc, old_bitmap );
    DeleteObject( bitmap );
    DeleteDC( dc );
    EndPaint( window, &paint_info );
}

void close_panel() {
    if ( panel.window ) DestroyWindow( panel.window );
}

void open_panel( const wstring &problem );

void open_class( const today_row &row ) {
    profile_file profiles = load_profiles();
    vector<wstring> titles;
    for ( const setup &each : profiles.setups ) titles.push_back( each.title );
    class_click click = plan_class_click( row.meeting, panel.names, panel.tiles, titles );
    close_panel();

    wstring problem, target = click.desktop;
    if ( click.create_desktop ) {
        size_t count_before = desktop_names().size();
        create_desktop( click.desktop, problem );
        // The registry can lag behind the new desktop, so wait for it as switch_to_desktop does
        vector<wstring> fresh = desktop_names();
        for ( int wait = 0; wait < 40 && fresh.size() <= count_before; ++wait ) {
            Sleep( 50 );
            fresh = desktop_names();
        }
        if ( !problem.empty() && !fresh.empty() ) target = fresh.back();
    }
    wstring switch_problem = switch_to_desktop( target );
    if ( !switch_problem.empty() ) {
        open_panel( problem.empty() ? switch_problem : problem + L" " + switch_problem );
        return;
    }

    const setup *found = click.setup_title.empty() ? nullptr : find_setup( profiles, click.setup_title );
    if ( found ) {
        setup chosen = *found;
        pick_answer picks;
        bool asked = false;
        for ( const layout_item &item : chosen.items ) asked = asked || item.kind == item_kind::pick;
        if ( asked ) picks = ask_for_picks( nullptr, chosen );
        if ( !asked || picks.accepted ) {
            vector<wstring> problems = open_setup( chosen, picks.picks, picks.separate );
            for ( const wstring &each : problems ) problem += ( problem.empty() ? L"" : L" " ) + each;
        }
    }
    if ( !problem.empty() ) open_panel( problem );
}

void start_pomodoro( bool long_video ) {
    wstring command, problem;
    crammy_settings settings = panel.settings;
    close_panel();
    if ( youtonomous_command( settings, long_video, command, problem ) && !start_process( command ) )
        problem = L"Youtonomous did not start: " + command;
    if ( !problem.empty() ) open_panel( problem );
}

panel_layout current_layout() {
    RECT client;
    GetClientRect( panel.window, &client );
    return lay_out_panel( panel.today, panel.tiles, client.right, panel.dpi );
}

link link_at( const panel_layout &layout, POINT point ) {
    for ( int i = 0; i < 3; ++i )
        if ( PtInRect( &layout.links[i], point ) ) return link( i );
    return link::none;
}

void click( POINT point ) {
    panel_layout layout = current_layout();
    for ( size_t i = 0; i < layout.rows.size(); ++i ) {
        if ( !PtInRect( &layout.rows[i], point ) ) continue;
        today_row row = panel.today.rows[i];
        if ( row.known ) open_class( row );
        return;
    }
    for ( size_t i = 0; i < layout.chips.size(); ++i ) {
        if ( !PtInRect( &layout.chips[i], point ) ) continue;
        wstring name = panel.tiles[i].name;
        close_panel();
        wstring problem = switch_to_desktop( name );
        if ( !problem.empty() ) open_panel( problem );
        return;
    }
    link chosen = link_at( layout, point );
    if ( chosen == link::long_pomodoro ) start_pomodoro( true );
    else if ( chosen == link::short_pomodoro ) start_pomodoro( false );
    else if ( chosen == link::setups ) {
        close_panel();
        show_setups();
    }
}

bool over_something( POINT point ) {
    panel_layout layout = current_layout();
    for ( size_t i = 0; i < layout.rows.size(); ++i )
        if ( PtInRect( &layout.rows[i], point ) ) return panel.today.rows[i].known;
    for ( const RECT &box : layout.chips )
        if ( PtInRect( &box, point ) ) return true;
    return link_at( layout, point ) != link::none;
}

void set_hovered( HWND window, link hovered ) {
    if ( panel.hovered == hovered ) return;
    panel.hovered = hovered;
    InvalidateRect( window, nullptr, FALSE );
}

LRESULT CALLBACK panel_proc( HWND window, UINT message, WPARAM wparam, LPARAM lparam ) {
    switch ( message ) {
    case WM_PAINT:
        paint( window );
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_LBUTTONUP:
        click( { GET_X_LPARAM( lparam ), GET_Y_LPARAM( lparam ) } );
        return 0;
    case WM_MOUSEMOVE: {
        TRACKMOUSEEVENT track{ sizeof( track ), TME_LEAVE, window, 0 };
        TrackMouseEvent( &track );
        set_hovered( window, link_at( current_layout(), { GET_X_LPARAM( lparam ), GET_Y_LPARAM( lparam ) } ) );
        return 0;
    }
    case WM_MOUSELEAVE:
        set_hovered( window, link::none );
        return 0;
    case WM_SETCURSOR:
        if ( LOWORD( lparam ) == HTCLIENT ) {
            POINT point;
            GetCursorPos( &point );
            ScreenToClient( window, &point );
            SetCursor( LoadCursorW( nullptr, over_something( point ) ? IDC_HAND : IDC_ARROW ) );
            return TRUE;
        }
        break;
    case WM_KEYDOWN:
        if ( wparam == VK_ESCAPE ) close_panel();
        return 0;
    case WM_COMMAND:
        if ( LOWORD( wparam ) == IDCANCEL ) close_panel();   // Esc arrives this way through IsDialogMessage
        return 0;
    case WM_ACTIVATE:
        if ( LOWORD( wparam ) == WA_INACTIVE ) PostMessageW( window, WM_CLOSE, 0, 0 );
        break;
    case WM_DESTROY:
        panel.window = nullptr;
        panel.hovered = link::none;
        panel.closed_at = GetTickCount64();
        return 0;
    }
    return DefWindowProcW( window, message, wparam, lparam );
}

void open_panel( const wstring &problem ) {
    close_panel();
    static bool registered = false;
    HINSTANCE instance = GetModuleHandleW( nullptr );
    if ( !registered ) {
        WNDCLASSEXW window_class{ sizeof( window_class ) };
        window_class.lpfnWndProc = panel_proc;
        window_class.hInstance = instance;
        window_class.hCursor = LoadCursorW( nullptr, IDC_ARROW );
        window_class.lpszClassName = panel_class_name;
        registered = RegisterClassExW( &window_class ) != 0;
    }

    panel.dpi = GetDpiForSystem();
    panel.problem = problem;
    read_panel_data();

    int width = scaled( panel_width, panel.dpi );
    int height = panel_height( panel.today, panel.tiles, panel.dpi ) + problem_height( width );
    RECT work;
    SystemParametersInfoW( SPI_GETWORKAREA, 0, &work, 0 );
    int gap = scaled( 8, panel.dpi );
    panel.window = CreateWindowExW( WS_EX_TOPMOST | WS_EX_TOOLWINDOW, panel_class_name, L"Crammy", WS_POPUP | WS_BORDER,
                                    work.left + gap, work.bottom - gap - height, width, height, nullptr, nullptr,
                                    instance, nullptr );
    if ( !panel.window ) return;
    ShowWindow( panel.window, SW_SHOW );
    if ( !SetForegroundWindow( panel.window ) ) bring_to_front( panel.window );
    // A panel without focus would never get the focus loss that closes it.
    if ( GetForegroundWindow() != panel.window ) {
        close_panel();
        return;
    }
    SetFocus( panel.window );
}

}  // namespace

int panel_height( const today_section &today, const vector<desktop_tile> &tiles, UINT dpi ) {
    return lay_out_panel( today, tiles, scaled( panel_width, dpi ), dpi ).height;
}

void draw_panel( HDC dc, int width, int height, const today_section &today, const vector<desktop_tile> &tiles,
                 const wstring &date_text, const wstring &time_text, UINT dpi ) {
    draw_contents( dc, width, height, today, tiles, date_text, time_text, dpi, link::none );
}

void show_setups() {
    HWND open_window = FindWindowW( L"CrammyMain", nullptr );
    if ( !open_window ) {
        show_main_window( L"" );
        return;
    }
    if ( IsIconic( open_window ) ) ShowWindow( open_window, SW_RESTORE );
    SetForegroundWindow( open_window );
}

void toggle_panel() {
    if ( panel.window ) {
        close_panel();
        return;
    }
    // A click on the tray icon or taskbar button first takes focus away, which closes the panel; that click
    // should not open it again.
    if ( GetTickCount64() - panel.closed_at < 300 ) return;
    open_panel( L"" );
}

void show_panel() {
    if ( !panel.window ) open_panel( L"" );
}
