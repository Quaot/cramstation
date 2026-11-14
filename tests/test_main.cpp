// Runs without showing anything: the canvas sits in a hidden window and gets
// fake drops in code. Prints one line per check and returns 1 if any failed.
//
//   tests\test.cmd                  runs the checks
//   crammy_test.exe shot.bmp   also saves a picture of a sample setup

#include <windows.h>
#include <ole2.h>
#include <shlobj.h>

#include <cstdio>

#include "../src/canvas.h"
#include "../src/files.h"
#include "../src/ui.h"

namespace {

int failures = 0;

void check( bool passed, const char *what ) {
    std::printf( "%s  %s\n", passed ? "pass" : "FAIL", what );
    if ( !passed ) ++failures;
}

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
    check_drops( canvas );
    if ( argument_count > 1 ) save_picture( canvas, arguments[1] );

    std::printf( "%s\n", failures ? "Some checks failed." : "All checks passed." );
    return failures ? 1 : 0;
}
