#include "ui.h"

#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>

#include "profiles.h"

namespace {

enum control_id { id_text = 200, id_save, id_insert_file, id_insert_folder };

struct editor_state {
    HWND window = nullptr, owner = nullptr;
    HWND text_box = nullptr, save_button = nullptr, file_button = nullptr, folder_button = nullptr, status = nullptr;
    HFONT ui_font = nullptr, text_font = nullptr;
    UINT dpi = 96;
};

editor_state editor;

// The edit box needs \r\n line ends
wstring with_crlf( const wstring &text ) {
    wstring result;
    for ( wchar_t c : text ) {
        if ( c == L'\r' ) continue;
        if ( c == L'\n' ) result += L'\r';
        result += c;
    }
    return result;
}

wstring box_text() {
    int length = GetWindowTextLengthW( editor.text_box );
    wstring text( length + 1, L'\0' );
    GetWindowTextW( editor.text_box, text.data(), length + 1 );
    text.resize( length );
    return text;
}

void insert_text( const wstring &text ) {
    SendMessageW( editor.text_box, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>( text.c_str() ) );
    SetFocus( editor.text_box );
}

void save() {
    wstring text = box_text();
    if ( !write_text_file( profiles_path(), text ) ) {
        SetWindowTextW( editor.status, L"Could not save profiles.txt." );
        return;
    }
    SendMessageW( editor.text_box, EM_SETMODIFY, FALSE, 0 );
    profile_book checked = read_book( text );
    wstring message = L"Saved. " + std::to_wstring( all_setups( checked ).size() ) + L" setups.";
    if ( !checked.errors.empty() ) {
        message += L" " + std::to_wstring( checked.errors.size() ) + L" problem(s), first: " + checked.errors[0];
    }
    SetWindowTextW( editor.status, message.c_str() );
    if ( editor.owner ) PostMessageW( editor.owner, profiles_changed_message, 0, 0 );
}

void insert_file_path() {
    wchar_t path[MAX_PATH * 4] = L"";
    OPENFILENAMEW dialog{ sizeof( dialog ) };
    dialog.hwndOwner = editor.window;
    dialog.lpstrFilter = L"PDF files\0*.pdf\0Text files\0*.txt\0All files\0*.*\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = MAX_PATH * 4;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if ( GetOpenFileNameW( &dialog ) ) insert_text( L"\"" + wstring( path ) + L"\"" );
}

void insert_folder_path() {
    BROWSEINFOW dialog{};
    dialog.hwndOwner = editor.window;
    dialog.lpszTitle = L"Pick a folder";
    dialog.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE folder = SHBrowseForFolderW( &dialog );
    if ( !folder ) return;
    wchar_t path[MAX_PATH];
    if ( SHGetPathFromIDListW( folder, path ) ) insert_text( L"\"" + wstring( path ) + L"\"" );
    CoTaskMemFree( folder );
}

// Returns false when the user cancels closing
bool ask_to_save() {
    if ( !SendMessageW( editor.text_box, EM_GETMODIFY, 0, 0 ) ) return true;
    int answer = MessageBoxW( editor.window, L"Save the changes to profiles.txt?", L"Cramstation", MB_YESNOCANCEL | MB_ICONQUESTION );
    if ( answer == IDCANCEL ) return false;
    if ( answer == IDYES ) save();
    return true;
}

void lay_out( int width, int height ) {
    UINT dpi = editor.dpi;
    int pad = scaled( 8, dpi ), row = scaled( 28, dpi );
    int bottom = height - pad - row;
    MoveWindow( editor.text_box, pad, pad, width - 2 * pad, bottom - 2 * pad, TRUE );

    HWND buttons[] = { editor.save_button, editor.file_button, editor.folder_button };
    int widths[] = { 90, 120, 130 };
    int x = pad;
    for ( int i = 0; i < 3; ++i ) {
        MoveWindow( buttons[i], x, bottom, scaled( widths[i], dpi ), row, TRUE );
        x += scaled( widths[i], dpi ) + pad;
    }
    MoveWindow( editor.status, x, bottom + scaled( 5, dpi ), width - x - pad, row, TRUE );
}

LRESULT CALLBACK editor_proc( HWND window, UINT message, WPARAM wparam, LPARAM lparam ) {
    switch ( message ) {
    case WM_CREATE:
        editor.window = window;
        editor.ui_font = make_font( editor.dpi, L"Segoe UI", 10 );
        editor.text_font = make_font( editor.dpi, L"Consolas", 11 );
        editor.text_box = add_control( window, WC_EDITW, L"",
                                       ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
                                       id_text, editor.text_font, WS_EX_CLIENTEDGE );
        editor.save_button = add_control( window, WC_BUTTONW, L"Save", WS_TABSTOP, id_save, editor.ui_font );
        editor.file_button = add_control( window, WC_BUTTONW, L"Insert file...", WS_TABSTOP, id_insert_file, editor.ui_font );
        editor.folder_button = add_control( window, WC_BUTTONW, L"Insert folder...", WS_TABSTOP, id_insert_folder, editor.ui_font );
        editor.status = add_control( window, WC_STATICW, L"Ctrl+S saves. Insert puts a quoted path where the cursor is.",
                                     SS_NOPREFIX | SS_ENDELLIPSIS, 0, editor.ui_font );
        SendMessageW( editor.text_box, EM_SETLIMITTEXT, 4 * 1024 * 1024, 0 );
        SetWindowTextW( editor.text_box, with_crlf( read_text_file( profiles_path() ) ).c_str() );
        SendMessageW( editor.text_box, EM_SETMODIFY, FALSE, 0 );
        return 0;
    case WM_SIZE:
        lay_out( LOWORD( lparam ), HIWORD( lparam ) );
        return 0;
    case WM_COMMAND:
        switch ( LOWORD( wparam ) ) {
        case id_save:
        case save_shortcut: save(); break;
        case id_insert_file: insert_file_path(); break;
        case id_insert_folder: insert_folder_path(); break;
        case IDCANCEL: SendMessageW( window, WM_CLOSE, 0, 0 ); break;
        }
        return 0;
    case WM_CLOSE:
        if ( ask_to_save() ) DestroyWindow( window );
        return 0;
    case WM_DESTROY:
        DeleteObject( editor.ui_font );
        DeleteObject( editor.text_font );
        if ( editor.owner ) {
            EnableWindow( editor.owner, TRUE );
            SetForegroundWindow( editor.owner );
        } else {
            PostQuitMessage( 0 );
        }
        return 0;
    }
    return DefWindowProcW( window, message, wparam, lparam );
}

}  // namespace

void show_editor( HWND owner ) {
    editor.owner = owner;
    editor.dpi = GetDpiForSystem();

    WNDCLASSEXW window_class{ sizeof( window_class ) };
    window_class.lpfnWndProc = editor_proc;
    window_class.hInstance = GetModuleHandleW( nullptr );
    window_class.hCursor = LoadCursorW( nullptr, IDC_ARROW );
    window_class.hbrBackground = reinterpret_cast<HBRUSH>( COLOR_BTNFACE + 1 );
    window_class.lpszClassName = L"CramstationEditor";
    window_class.hIcon = LoadIconW( window_class.hInstance, MAKEINTRESOURCEW( 1 ) );
    RegisterClassExW( &window_class );

    RECT place = centred_on_screen( scaled( 900, editor.dpi ), scaled( 640, editor.dpi ) );
    HWND window = CreateWindowExW( 0, window_class.lpszClassName, L"Cramstation: edit profiles",
                                   WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, place.left, place.top,
                                   place.right - place.left, place.bottom - place.top, owner, nullptr,
                                   window_class.hInstance, nullptr );
    if ( owner ) EnableWindow( owner, FALSE );
    ShowWindow( window, SW_SHOW );
    SetFocus( editor.text_box );
}
