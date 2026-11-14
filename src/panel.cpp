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

const int panel_width = 420;
const int margin = 18;
const int row_height = 24;
const int tile_height = 58;
const int tile_gap = 8;
const int icon_size = 18;
const int icon_gap = 3;
const int max_icons = 8;
const int button_height = 32;

enum class button { long_pomodoro, short_pomodoro, setups };
const wchar_t *button_labels[] = { L"Pomodoro 3 h", L"Pomodoro 1.5 h", L"Setups\x2026" };

struct panel_layout {
    int rule_after_header;
    int today_heading;
    vector<RECT> rows;
    RECT message;
    int rule_after_today;
    int desktops_heading;
    vector<RECT> tiles;
    int rule_after_tiles;
    RECT buttons[3];
    int height;
};

panel_layout lay_out_panel( const today_section &today, const vector<desktop_tile> &tiles, int width, UINT dpi ) {
    auto px = [dpi]( int value ) { return scaled( value, dpi ); };
    panel_layout layout{};
    int left = px( margin ), right = width - px( margin );

    layout.rule_after_header = px( 54 );
    layout.today_heading = layout.rule_after_header + px( 12 );
    int y = layout.today_heading + px( 28 );
    for ( size_t i = 0; i < today.rows.size(); ++i ) {
        layout.rows.push_back( { 0, y, width, y + px( row_height ) } );
        y += px( row_height );
    }
    layout.message = { left, y, right, y };
    if ( !today.message.empty() ) {
        layout.message.bottom = y + px( row_height );
        y += px( row_height );
    }

    layout.rule_after_today = y + px( 12 );
    layout.desktops_heading = layout.rule_after_today + px( 12 );
    y = layout.desktops_heading + px( 30 );
    int tile_width = ( right - left - px( tile_gap ) ) / 2;
    for ( size_t i = 0; i < tiles.size(); ++i ) {
        int column = int( i % 2 ), line = int( i / 2 );
        int tile_left = left + column * ( tile_width + px( tile_gap ) );
        int tile_top = y + line * ( px( tile_height ) + px( tile_gap ) );
        layout.tiles.push_back( { tile_left, tile_top, tile_left + tile_width, tile_top + px( tile_height ) } );
    }
    int tile_lines = int( ( tiles.size() + 1 ) / 2 );
    if ( tile_lines > 0 ) y += tile_lines * px( tile_height ) + ( tile_lines - 1 ) * px( tile_gap );

    layout.rule_after_tiles = y + px( 16 );
    y = layout.rule_after_tiles + px( 16 );
    int button_width = ( right - left - 2 * px( tile_gap ) ) / 3;
    for ( int i = 0; i < 3; ++i ) {
        int button_left = left + i * ( button_width + px( tile_gap ) );
        layout.buttons[i] = { button_left, y, button_left + button_width, y + px( button_height ) };
    }
    layout.height = y + px( button_height ) + px( 18 );
    return layout;
}

HFONT panel_font( UINT dpi, const wchar_t *face, int points, int weight = FW_NORMAL ) {
    LOGFONTW font{};
    font.lfHeight = -MulDiv( points, static_cast<int>( dpi ), 72 );
    font.lfWeight = weight;
    font.lfQuality = CLEARTYPE_QUALITY;
    lstrcpynW( font.lfFaceName, face, LF_FACESIZE );
    return CreateFontIndirectW( &font );
}

struct panel_fonts {
    HFONT date, heading, text, strong;

    explicit panel_fonts( UINT dpi )
        : date( panel_font( dpi, L"Georgia", 16 ) ), heading( panel_font( dpi, L"Georgia", 12 ) ),
          text( panel_font( dpi, L"Segoe UI", 10 ) ), strong( panel_font( dpi, L"Segoe UI", 10, FW_SEMIBOLD ) ) {}
    ~panel_fonts() {
        for ( HFONT font : { date, heading, text, strong } ) DeleteObject( font );
    }
};

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

void draw_row( HDC dc, const today_row &row, const RECT &box, int width, const panel_fonts &fonts, UINT dpi ) {
    auto px = [dpi]( int value ) { return scaled( value, dpi ); };
    COLORREF colour = row.marked ? red : row.known ? ink : grey;
    UINT format = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;
    int left = px( margin );
    if ( row.marked ) fill( dc, { 0, box.top + px( 4 ), px( 5 ), box.bottom - px( 4 ) }, red );

    const class_meeting &meeting = row.meeting;
    wstring times = clock_text( meeting.start_minute ) + L"\x2013" + clock_text( meeting.end_minute );
    write( dc, times, { left, box.top, left + px( 88 ), box.bottom }, fonts.text, colour, format );
    write( dc, meeting.course, { left + px( 94 ), box.top, left + px( 166 ), box.bottom },
           row.marked ? fonts.strong : fonts.text, colour, format );
    write( dc, row.kind_name, { left + px( 170 ), box.top, left + px( 238 ), box.bottom }, fonts.text, colour, format );
    write( dc, meeting.room, { left + px( 242 ), box.top, left + px( 312 ), box.bottom }, fonts.text, colour, format );
    write( dc, row.note, { left + px( 312 ), box.top, width - px( margin ), box.bottom }, fonts.text, colour,
           format | DT_RIGHT );
}

void draw_tile( HDC dc, const desktop_tile &tile, const RECT &box, const panel_fonts &fonts, UINT dpi ) {
    auto px = [dpi]( int value ) { return scaled( value, dpi ); };
    if ( tile.current ) outline( dc, box, red, px( 2 ) );
    else outline( dc, box, ink, px( 1 ) );

    write( dc, tile.name, { box.left + px( 9 ), box.top + px( 5 ), box.right - px( 9 ), box.top + px( 25 ) },
           tile.current ? fonts.strong : fonts.text, tile.current ? red : ink, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS );

    int size = px( icon_size );
    int x = box.left + px( 9 ), y = box.bottom - px( 8 ) - size;
    for ( size_t i = 0; i < tile.program_paths.size() && int( i ) < max_icons; ++i ) {
        HICON icon = icon_of( tile.program_paths[i], size );
        if ( icon ) DrawIconEx( dc, x, y, icon, size, size, 0, nullptr, DI_NORMAL );
        x += size + px( icon_gap );
    }
}

void draw_button( HDC dc, const wchar_t *label, const RECT &box, const panel_fonts &fonts ) {
    fill( dc, box, red );
    write( dc, label, box, fonts.strong, cream, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS );
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
    ULONGLONG closed_at = 0;
};

panel_state panel;

const wchar_t *day_names[] = { L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday" };
const wchar_t *month_names[] = { L"January", L"February", L"March", L"April", L"May", L"June", L"July",
                                 L"August", L"September", L"October", L"November", L"December" };

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
    panel.date = wstring( day_names[now.wDayOfWeek] ) + L", " + month_names[now.wMonth - 1] + L" " +
                 std::to_wstring( now.wDay );
    int minute_now = now.wHour * 60 + now.wMinute;
    panel.time = clock_text( minute_now );

    vector<wstring> errors;
    vector<class_block> blocks = read_timetable( read_text_file( timetable_path() ), errors );
    calendar_date today{ now.wYear, now.wMonth, now.wDay };
    panel.today = build_today( classes_on( blocks, today ), minute_now, known_courses( blocks ), errors );

    panel.names = desktop_names();
    panel.tiles = build_tiles( panel.names, current_desktop_index(), windows_by_desktop() );

    vector<wstring> settings_errors;
    panel.settings = read_settings( read_text_file( settings_path() ), settings_errors );
}

int problem_height( int width ) {
    if ( panel.problem.empty() ) return 0;
    panel_fonts fonts( panel.dpi );
    HDC dc = GetDC( nullptr );
    SelectObject( dc, fonts.text );
    RECT box{ 0, 0, width - 2 * scaled( margin, panel.dpi ), 0 };
    DrawTextW( dc, panel.problem.c_str(), int( panel.problem.size() ), &box, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX );
    ReleaseDC( nullptr, dc );
    return box.bottom + scaled( 18, panel.dpi );
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
    draw_panel( dc, client.right, top, panel.today, panel.tiles, panel.date, panel.time, panel.dpi );
    if ( !panel.problem.empty() ) {
        panel_fonts fonts( panel.dpi );
        SetBkMode( dc, TRANSPARENT );
        rule( dc, top, client.right, panel.dpi );
        RECT box{ scaled( margin, panel.dpi ), top + scaled( 9, panel.dpi ), client.right - scaled( margin, panel.dpi ),
                  client.bottom };
        write( dc, panel.problem, box, fonts.text, ink, DT_WORDBREAK );
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
        create_desktop( click.desktop, problem );
        vector<wstring> fresh = desktop_names();
        if ( !problem.empty() && !fresh.empty() ) target = fresh.back();
    }
    wstring switch_problem = switch_to_desktop( target );
    if ( problem.empty() ) problem = switch_problem;

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

void click( int x, int y ) {
    POINT point{ x, y };
    RECT client;
    GetClientRect( panel.window, &client );
    panel_layout layout = lay_out_panel( panel.today, panel.tiles, client.right, panel.dpi );

    for ( size_t i = 0; i < layout.rows.size(); ++i ) {
        if ( !PtInRect( &layout.rows[i], point ) ) continue;
        today_row row = panel.today.rows[i];
        if ( row.known ) open_class( row );
        return;
    }
    for ( size_t i = 0; i < layout.tiles.size(); ++i ) {
        if ( !PtInRect( &layout.tiles[i], point ) ) continue;
        wstring name = panel.tiles[i].name;
        close_panel();
        wstring problem = switch_to_desktop( name );
        if ( !problem.empty() ) open_panel( problem );
        return;
    }
    if ( PtInRect( &layout.buttons[int( button::long_pomodoro )], point ) ) start_pomodoro( true );
    else if ( PtInRect( &layout.buttons[int( button::short_pomodoro )], point ) ) start_pomodoro( false );
    else if ( PtInRect( &layout.buttons[int( button::setups )], point ) ) {
        close_panel();
        show_setups();
    }
}

bool over_something( HWND window ) {
    POINT point;
    GetCursorPos( &point );
    ScreenToClient( window, &point );
    RECT client;
    GetClientRect( window, &client );
    panel_layout layout = lay_out_panel( panel.today, panel.tiles, client.right, panel.dpi );
    for ( size_t i = 0; i < layout.rows.size(); ++i )
        if ( PtInRect( &layout.rows[i], point ) ) return panel.today.rows[i].known;
    for ( const RECT &box : layout.tiles )
        if ( PtInRect( &box, point ) ) return true;
    for ( const RECT &box : layout.buttons )
        if ( PtInRect( &box, point ) ) return true;
    return false;
}

LRESULT CALLBACK panel_proc( HWND window, UINT message, WPARAM wparam, LPARAM lparam ) {
    switch ( message ) {
    case WM_PAINT:
        paint( window );
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_LBUTTONUP:
        click( GET_X_LPARAM( lparam ), GET_Y_LPARAM( lparam ) );
        return 0;
    case WM_SETCURSOR:
        if ( LOWORD( lparam ) == HTCLIENT ) {
            SetCursor( LoadCursorW( nullptr, over_something( window ) ? IDC_HAND : IDC_ARROW ) );
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
        return 0;
    case WM_DESTROY:
        panel.window = nullptr;
        panel.closed_at = GetTickCount64();
        return 0;
    }
    return DefWindowProcW( window, message, wparam, lparam );
}

void open_panel( const wstring &problem ) {
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
    int gap = scaled( 12, panel.dpi );
    panel.window = CreateWindowExW( WS_EX_TOPMOST | WS_EX_TOOLWINDOW, panel_class_name, L"Crammy", WS_POPUP | WS_BORDER,
                                    work.left + gap, work.bottom - gap - height, width, height, nullptr, nullptr,
                                    instance, nullptr );
    if ( !panel.window ) return;
    ShowWindow( panel.window, SW_SHOW );
    if ( !SetForegroundWindow( panel.window ) ) bring_to_front( panel.window );
    SetFocus( panel.window );
}

}  // namespace

int panel_height( const today_section &today, const vector<desktop_tile> &tiles, UINT dpi ) {
    return lay_out_panel( today, tiles, scaled( panel_width, dpi ), dpi ).height;
}

void draw_panel( HDC dc, int width, int height, const today_section &today, const vector<desktop_tile> &tiles,
                 const wstring &date_text, const wstring &time_text, UINT dpi ) {
    auto px = [dpi]( int value ) { return scaled( value, dpi ); };
    panel_layout layout = lay_out_panel( today, tiles, width, dpi );
    panel_fonts fonts( dpi );
    int left = px( margin ), right = width - px( margin );

    fill( dc, { 0, 0, width, height }, cream );
    SetBkMode( dc, TRANSPARENT );

    RECT header{ left, px( 14 ), right, layout.rule_after_header - px( 8 ) };
    write( dc, date_text, header, fonts.date, ink, DT_SINGLELINE | DT_BOTTOM | DT_LEFT );
    write( dc, time_text, header, fonts.date, ink, DT_SINGLELINE | DT_BOTTOM | DT_RIGHT );
    rule( dc, layout.rule_after_header, width, dpi );

    write( dc, L"Today", { left, layout.today_heading, right, layout.today_heading + px( 24 ) }, fonts.heading, ink,
           DT_SINGLELINE | DT_VCENTER );
    for ( size_t i = 0; i < today.rows.size(); ++i ) draw_row( dc, today.rows[i], layout.rows[i], width, fonts, dpi );
    if ( !today.message.empty() )
        write( dc, today.message, layout.message, fonts.text, ink, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS );
    rule( dc, layout.rule_after_today, width, dpi );

    write( dc, L"Desktops", { left, layout.desktops_heading, right, layout.desktops_heading + px( 24 ) }, fonts.heading,
           ink, DT_SINGLELINE | DT_VCENTER );
    for ( size_t i = 0; i < tiles.size(); ++i ) draw_tile( dc, tiles[i], layout.tiles[i], fonts, dpi );
    rule( dc, layout.rule_after_tiles, width, dpi );

    for ( int i = 0; i < 3; ++i ) draw_button( dc, button_labels[i], layout.buttons[i], fonts );
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
