// The canvas sits in a hidden window and gets fake drops in code. The notes check
// starts its own minimized Notepad++ and closes it again. Prints one line per check
// and returns 1 if any failed.
//
//   tests\test.cmd                  runs the checks
//   tests\crammy_test.exe shot.bmp  also saves a picture of a sample setup

#include <windows.h>
#include <ole2.h>
#include <shlobj.h>

#include <cstdio>

#include "check.h"
#include "../src/canvas.h"
#include "../src/files.h"
#include "../src/notes_tabs.h"
#include "../src/tiling.h"
#include "../src/ui.h"

namespace {

const wstring course = expand_variables( L"%USERPROFILE%\\Downloads\\ECE 190" );

// A data object holding file paths, the same thing File Explorer hands over when you drag
IDataObject *files_object( const vector<wstring> &paths ) {
    wstring list;
    for ( const wstring &path : paths ) list += path + L'\0';
    list += L'\0';
    size_t bytes = sizeof( DROPFILES ) + list.size() * sizeof( wchar_t );
    HGLOBAL memory = GlobalAlloc( GHND, bytes );
    auto *drop = static_cast<DROPFILES *>( GlobalLock( memory ) );
    drop->pFiles = sizeof( DROPFILES );
    drop->fWide = TRUE;
    memcpy( reinterpret_cast<BYTE *>( drop ) + sizeof( DROPFILES ), list.data(), list.size() * sizeof( wchar_t ) );
    GlobalUnlock( memory );

    IDataObject *data = nullptr;
    SHCreateDataObject( nullptr, 0, nullptr, nullptr, IID_PPV_ARGS( &data ) );
    FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium{};
    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = memory;
    data->SetData( &format, &medium, TRUE );
    return data;
}

// Drops at a fraction across and down the canvas, as a mouse would
void drop_at( HWND canvas, const vector<wstring> &paths, double across, double down ) {
    RECT client;
    GetClientRect( canvas, &client );
    POINT point = { LONG( client.right * across ), LONG( client.bottom * down ) };
    ClientToScreen( canvas, &point );
    POINTL at = { point.x, point.y };
    IDataObject *data = files_object( paths );
    DWORD effect = DROPEFFECT_COPY;
    IDropTarget *target = canvas_drop_target();
    target->DragEnter( data, MK_LBUTTON, at, &effect );
    target->DragOver( MK_LBUTTON, at, &effect );
    target->Drop( data, MK_LBUTTON, at, &effect );
    data->Release();
}

// The test program sits in tests\, one folder below profiles.txt
wstring real_profiles() {
    return join_path( parent_folder( parent_folder( profiles_path() ) ), L"profiles.txt" );
}

void check_profiles_round_trip() {
    profile_book first = read_book( read_text_file( real_profiles() ) );
    wstring written = write_book( first );
    profile_book second = read_book( written );
    check( first.errors.empty(), "profiles.txt reads without errors" );
    check( all_setups( first ).size() == all_setups( second ).size(), "saving keeps every setup" );
    check( write_book( second ) == written, "saving twice gives the same file" );
}

void check_newest_pattern() {
    check( newest_pattern( L"ECE190-W10-Slides.pdf" ) == L"ECE190-W*-Slides.pdf", "week numbers become *" );
    check( newest_pattern( L"PDF\\MATH117-L23.pdf" ) == L"PDF\\MATH117-L*.pdf", "lecture numbers become *" );
    check( newest_pattern( L"Formula sheet.pdf" ).empty(), "names without a number have no pattern" );
}

void check_answer_keys() {
    check( is_answer_key( L"ECE150-Mock-1112a-S.pdf" ), "a name ending in -S is an answer key" );
    check( is_answer_key( L"F25-M-S.pdf" ), "a short name ending in -S is an answer key" );
    check( is_answer_key( L"F24-M-S (1).pdf" ) && is_answer_key( L"Week 3 solutions.pdf" ),
           "the older key names still count" );
    check( !is_answer_key( L"ECE190-W10-Slides.pdf" ) && !is_answer_key( L"ECE190-W10-Sum.pdf" ),
           "slides and summaries are not answer keys" );
    check( !is_answer_key( L"ECE150-Mock-1112a.pdf" ) && !is_answer_key( L"F25-M.pdf" ), "papers are not answer keys" );
}

void check_drops( HWND canvas ) {
    wstring slides = course + L"\\1 Lectures\\PDF\\ECE190-W10-Slides.pdf";
    wstring sheet = course + L"\\1 Lectures\\PDF\\ECE190-W10-Sum.pdf";
    wstring exams = course + L"\\6 Exams";

    vector<written_item> items;
    show_items( &items, course );
    drop_at( canvas, { exams }, 0.5, 0.5 );
    check( items.size() == 1 && items[0].side == L"full" && items[0].kind == item_kind::pick && items[0].paths[0] == L"6 Exams",
           "a folder dropped in the middle fills the screen and asks which PDFs" );

    drop_at( canvas, { slides }, 0.1, 0.5 );
    check( items.size() == 2 && items[1].side == L"left" && items[1].kind == item_kind::pdf, "a PDF dropped on the left goes left" );
    check( items[0].side == L"right", "the whole-screen window moves to the other half" );
    check( items[1].paths[0] == L"1 Lectures\\PDF\\ECE190-W10-Slides.pdf", "paths inside the course are stored short" );

    items.clear();
    show_items( &items, course );
    drop_at( canvas, { slides, sheet }, 0.9, 0.5 );
    check( items.size() == 1 && items[0].kind == item_kind::tabs && items[0].paths.size() == 2 && items[0].side == L"right",
           "two PDFs dropped together become tabs" );

    items.clear();
    show_items( &items, course );
    drop_at( canvas, { course + L"\\notes.docx" }, 0.5, 0.5 );
    check( items.empty(), "other file types are refused" );

    show_items( nullptr, L"" );
    drop_at( canvas, { slides }, 0.5, 0.5 );
    check( true, "dropping with no setup chosen does nothing" );
}

// A separate, minimized Notepad++ with three scratch files. Crammy remembers two of them
// and keeps one, so exactly one tab should close; the third stands for a tab you opened yourself.
void check_notes_tabs() {
    wstring folder = join_path( expand_variables( L"%TEMP%" ), L"crammy notes check" );
    CreateDirectoryW( folder.c_str(), nullptr );
    wstring old_notes = join_path( folder, L"old notes.txt" ), kept_notes = join_path( folder, L"kept notes.txt" ),
            own_file = join_path( folder, L"your own.txt" );
    for ( const wstring &path : { old_notes, kept_notes, own_file } ) write_text_file( path, L"notes\r\n" );

    wstring command = quoted( program_file( program::notepad_pp ) ) + L" -multiInst -nosession -noPlugin " +
                      quoted( old_notes ) + L" " + quoted( kept_notes ) + L" " + quoted( own_file );
    STARTUPINFOW startup{ sizeof( startup ) };
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWMINNOACTIVE;
    PROCESS_INFORMATION info{};
    if ( !CreateProcessW( nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &info ) ) {
        check( false, "Notepad++ starts for the notes check" );
        return;
    }
    WaitForInputIdle( info.hProcess, 5000 );
    HWND window = nullptr;
    for ( int wait = 0; wait < 50 && !window; ++wait ) {
        Sleep( 100 );
        for ( HWND each : top_windows() ) {
            DWORD process_id = 0;
            GetWindowThreadProcessId( each, &process_id );
            wchar_t class_name[32];
            GetClassNameW( each, class_name, 32 );
            if ( process_id == info.dwProcessId && wstring( class_name ) == L"Notepad++" ) window = each;
        }
    }
    auto tab_count = [&] { return int( SendMessageW( window, WM_USER + 1000 + 7, 0, 1 ) ); };
    check( window && tab_count() == 3, "the scratch Notepad++ opens three tabs" );

    remember_notes( old_notes );
    remember_notes( kept_notes );
    close_old_notes( { kept_notes }, window );
    check( tab_count() == 2, "only the old notes tab closes" );

    PostMessageW( window, WM_CLOSE, 0, 0 );
    if ( WaitForSingleObject( info.hProcess, 5000 ) != WAIT_OBJECT_0 ) TerminateProcess( info.hProcess, 0 );
    CloseHandle( info.hThread );
    CloseHandle( info.hProcess );
}

// Saves what the canvas looks like for a sample setup, without showing it on screen
void save_picture( HWND canvas, const wchar_t *path ) {
    static vector<written_item> items = {
        { L"left", item_kind::newest, { L"1 Lectures\\PDF\\ECE190-W*-Slides.pdf" } },
        { L"right", item_kind::newest, { L"1 Lectures\\PDF\\ECE190-W*-Sum.pdf" }, viewer::acrobat },
    };
    show_items( &items, course );
    RECT client;
    GetClientRect( canvas, &client );
    HDC screen = GetDC( nullptr );
    HDC dc = CreateCompatibleDC( screen );
    HBITMAP bitmap = CreateCompatibleBitmap( screen, client.right, client.bottom );
    SelectObject( dc, bitmap );
    SendMessageW( canvas, WM_PRINTCLIENT, reinterpret_cast<WPARAM>( dc ), PRF_CLIENT );

    BITMAPINFOHEADER header{ sizeof( header ), client.right, client.bottom, 1, 24, BI_RGB };
    int row_bytes = ( ( client.right * 3 + 3 ) / 4 ) * 4;
    vector<BYTE> pixels( row_bytes * client.bottom );
    GetDIBits( dc, bitmap, 0, client.bottom, pixels.data(), reinterpret_cast<BITMAPINFO *>( &header ), DIB_RGB_COLORS );
    BITMAPFILEHEADER file_header{ 0x4D42, DWORD( sizeof( file_header ) + sizeof( header ) + pixels.size() ), 0, 0,
                                  sizeof( file_header ) + sizeof( header ) };
    HANDLE file = CreateFileW( path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr );
    DWORD written;
    WriteFile( file, &file_header, sizeof( file_header ), &written, nullptr );
    WriteFile( file, &header, sizeof( header ), &written, nullptr );
    WriteFile( file, pixels.data(), DWORD( pixels.size() ), &written, nullptr );
    CloseHandle( file );
    DeleteObject( bitmap );
    DeleteDC( dc );
    ReleaseDC( nullptr, screen );
}

}  // namespace

int wmain( int argument_count, wchar_t **arguments ) {
    OleInitialize( nullptr );
    UINT dpi = GetDpiForSystem();
    HWND parent = CreateWindowExW( 0, L"STATIC", L"", WS_POPUP, 0, 0, scaled( 820, dpi ), scaled( 470, dpi ), nullptr,
                                   nullptr, GetModuleHandleW( nullptr ), nullptr );
    HWND canvas = create_canvas( parent, make_font( dpi, L"Segoe UI", 10 ), dpi );
    MoveWindow( canvas, 0, 0, scaled( 820, dpi ), scaled( 470, dpi ), FALSE );

    check_profiles_round_trip();
    check_newest_pattern();
    check_answer_keys();
    check_drops( canvas );
    check_notes_tabs();
    if ( argument_count > 1 ) save_picture( canvas, arguments[1] );

    std::printf( "%s\n", check_failures() ? "Some checks failed." : "All checks passed." );
    return check_failures() ? 1 : 0;
}
