#include "ui.h"

#include <commctrl.h>

#include <algorithm>
#include <set>

#include "launch.h"

namespace {

enum control_id { id_setups = 100, id_edit, id_filter, id_keys, id_list, id_separate };

struct pick_row {
    size_t item_index;      // which pick line of the setup it came from
    pdf_file pdf;
};

struct launcher_state {
    HWND window = nullptr;
    HWND setups_box = nullptr, edit_button = nullptr;
    HWND filter_label = nullptr, filter_box = nullptr, keys_check = nullptr, summary = nullptr, list = nullptr;
    HWND open_button = nullptr, separate_check = nullptr, status = nullptr;
    HFONT font = nullptr;
    UINT dpi = 96;
    profile_file profiles;
    vector<pick_row> rows;      // every PDF the setup lets you pick
    vector<size_t> shown;       // rows currently in the list
    std::set<wstring> ticked;   // kept while the filter hides rows
};

launcher_state ui;

wstring last_setup_path() {
    return join_path( parent_folder( profiles_path() ), L"last setup.txt" );
}

wstring row_key( const pick_row &row ) {
    return std::to_wstring( row.item_index ) + L"|" + row.pdf.path;
}

const setup *current_setup() {
    LRESULT index = SendMessageW( ui.setups_box, LB_GETCURSEL, 0, 0 );
    if ( index == LB_ERR || index >= static_cast<LRESULT>( ui.profiles.setups.size() ) ) return nullptr;
    return &ui.profiles.setups[index];
}

void set_status( const wstring &text ) {
    SetWindowTextW( ui.status, text.c_str() );
}

void remember_ticks() {
    int count = ListView_GetItemCount( ui.list );
    for ( int i = 0; i < count; ++i ) {
        wstring key = row_key( ui.rows[ ui.shown[i] ] );
        if ( ListView_GetCheckState( ui.list, i ) ) ui.ticked.insert( key );
        else ui.ticked.erase( key );
    }
}

void show_rows() {
    remember_ticks();
    wchar_t filter[256];
    GetWindowTextW( ui.filter_box, filter, 256 );
    bool show_keys = SendMessageW( ui.keys_check, BM_GETCHECK, 0, 0 ) == BST_CHECKED;
    const setup *chosen = current_setup();

    SendMessageW( ui.list, WM_SETREDRAW, FALSE, 0 );
    ListView_DeleteAllItems( ui.list );
    ui.shown.clear();
    for ( size_t i = 0; i < ui.rows.size(); ++i ) {
        const pick_row &row = ui.rows[i];
        if ( !show_keys && is_answer_key( row.pdf.name ) ) continue;
        if ( filter[0] && !contains_ci( row.pdf.path, filter ) ) continue;

        int line = static_cast<int>( ui.shown.size() );
        ui.shown.push_back( i );
        wstring name = stem( row.pdf.name );
        wstring side = chosen->items[ row.item_index ].side;
        wstring date = date_text( row.pdf.modified );

        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = line;
        item.pszText = name.data();
        ListView_InsertItem( ui.list, &item );
        ListView_SetItemText( ui.list, line, 1, const_cast<wchar_t *>( row.pdf.folder.c_str() ) );
        ListView_SetItemText( ui.list, line, 2, side.data() );
        ListView_SetItemText( ui.list, line, 3, date.data() );
        if ( ui.ticked.count( row_key( row ) ) ) ListView_SetCheckState( ui.list, line, TRUE );
    }
    SendMessageW( ui.list, WM_SETREDRAW, TRUE, 0 );
    InvalidateRect( ui.list, nullptr, TRUE );
}

void load_setup() {
    ui.rows.clear();
    ui.shown.clear();
    ui.ticked.clear();
    ListView_DeleteAllItems( ui.list );

    const setup *chosen = current_setup();
    if ( !chosen ) {
        SetWindowTextW( ui.summary, L"" );
        return;
    }
    for ( size_t i = 0; i < chosen->items.size(); ++i ) {
        if ( chosen->items[i].kind != item_kind::pick ) continue;
        for ( const pdf_file &pdf : find_pdfs( chosen->items[i].paths, true ) ) ui.rows.push_back( { i, pdf } );
    }
    SetWindowTextW( ui.summary, describe( *chosen ).c_str() );
    show_rows();

    bool has_pick = false;
    for ( const layout_item &item : chosen->items ) if ( item.kind == item_kind::pick ) has_pick = true;
    if ( !has_pick ) set_status( L"Nothing to tick here. Open shows this setup as written." );
    else if ( ui.rows.empty() ) set_status( L"No PDFs found in the pick folders." );
    else set_status( L"Tick the PDFs to open, then press Open." );
}

void load_profiles_into_list( const wstring &select_title ) {
    ui.profiles = load_profiles();
    SendMessageW( ui.setups_box, LB_RESETCONTENT, 0, 0 );
    int select = 0;
    for ( size_t i = 0; i < ui.profiles.setups.size(); ++i ) {
        const wstring &title = ui.profiles.setups[i].title;
        SendMessageW( ui.setups_box, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>( title.c_str() ) );
        if ( lower( title ) == lower( select_title ) ) select = static_cast<int>( i );
    }
    SendMessageW( ui.setups_box, LB_SETCURSEL, select, 0 );
    load_setup();
    if ( !ui.profiles.errors.empty() ) {
        set_status( L"profiles.txt has problems, " + ui.profiles.errors[0] + L". Use Edit profiles to fix it." );
    }
}

void open_current() {
    const setup *chosen = current_setup();
    if ( !chosen ) return;
    remember_ticks();

    vector<vector<wstring>> picks( chosen->items.size() );
    for ( const pick_row &row : ui.rows ) {
        if ( ui.ticked.count( row_key( row ) ) ) picks[ row.item_index ].push_back( row.pdf.path );
    }
    bool nothing_ticked = ui.ticked.empty();
    int selected = ListView_GetNextItem( ui.list, -1, LVNI_SELECTED );
    if ( nothing_ticked && selected >= 0 ) {
        const pick_row &row = ui.rows[ ui.shown[selected] ];
        picks[ row.item_index ].push_back( row.pdf.path );
    }

    bool separate = SendMessageW( ui.separate_check, BM_GETCHECK, 0, 0 ) == BST_CHECKED;
    ShowWindow( ui.window, SW_HIDE );
    vector<wstring> problems = open_setup( *chosen, picks, separate );
    write_text_file( last_setup_path(), chosen->title );

    if ( problems.empty() ) {
        DestroyWindow( ui.window );
        return;
    }
    ShowWindow( ui.window, SW_SHOW );
    wstring text;
    for ( const wstring &problem : problems ) text += problem + L"\r\n";
    MessageBoxW( ui.window, text.c_str(), L"Cramstation", MB_ICONWARNING );
}

void lay_out( int width, int height ) {
    UINT dpi = ui.dpi;
    int pad = scaled( 8, dpi ), row = scaled( 28, dpi ), left_width = scaled( 230, dpi );
    int bottom = height - pad - row;

    MoveWindow( ui.setups_box, pad, pad, left_width, bottom - 2 * pad, TRUE );
    MoveWindow( ui.edit_button, pad, bottom, left_width, row, TRUE );

    int x = pad + left_width + pad;
    int right_width = width - x - pad;
    int keys_width = scaled( 160, dpi ), label_width = scaled( 40, dpi );
    MoveWindow( ui.filter_label, x, pad + scaled( 5, dpi ), label_width, row, TRUE );
    MoveWindow( ui.filter_box, x + label_width, pad, right_width - label_width - keys_width - pad, row, TRUE );
    MoveWindow( ui.keys_check, x + right_width - keys_width, pad, keys_width, row, TRUE );

    int summary_top = pad + row + pad, summary_height = scaled( 54, dpi );
    MoveWindow( ui.summary, x, summary_top, right_width, summary_height, TRUE );
    int list_top = summary_top + summary_height + pad;
    MoveWindow( ui.list, x, list_top, right_width, bottom - pad - list_top, TRUE );

    int open_width = scaled( 100, dpi ), separate_width = scaled( 250, dpi );
    MoveWindow( ui.open_button, x, bottom, open_width, row, TRUE );
    MoveWindow( ui.separate_check, x + open_width + pad, bottom, separate_width, row, TRUE );
    int status_left = x + open_width + separate_width + 2 * pad;
    MoveWindow( ui.status, status_left, bottom + scaled( 5, dpi ), width - status_left - pad, row, TRUE );

    RECT list_area;
    GetClientRect( ui.list, &list_area );
    int folder_width = scaled( 120, dpi ), side_width = scaled( 50, dpi ), date_width = scaled( 95, dpi );
    ListView_SetColumnWidth( ui.list, 1, folder_width );
    ListView_SetColumnWidth( ui.list, 2, side_width );
    ListView_SetColumnWidth( ui.list, 3, date_width );
    int name_width = static_cast<int>( list_area.right ) - folder_width - side_width - date_width;
    ListView_SetColumnWidth( ui.list, 0, std::max( scaled( 120, dpi ), name_width ) );
}

void create_controls( HWND window ) {
    ui.window = window;
    ui.font = make_font( ui.dpi, L"Segoe UI", 10 );

    ui.setups_box = add_control( window, WC_LISTBOXW, L"", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP,
                                 id_setups, ui.font, WS_EX_CLIENTEDGE );
    ui.edit_button = add_control( window, WC_BUTTONW, L"Edit profiles", WS_TABSTOP, id_edit, ui.font );
    ui.filter_label = add_control( window, WC_STATICW, L"Filter", 0, 0, ui.font );
    ui.filter_box = add_control( window, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, id_filter, ui.font, WS_EX_CLIENTEDGE );
    ui.keys_check = add_control( window, WC_BUTTONW, L"Show answer keys", BS_AUTOCHECKBOX | WS_TABSTOP, id_keys, ui.font );
    ui.summary = add_control( window, WC_STATICW, L"", SS_NOPREFIX, 0, ui.font );
    ui.list = add_control( window, WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_TABSTOP,
                           id_list, ui.font, WS_EX_CLIENTEDGE );
    ui.open_button = add_control( window, WC_BUTTONW, L"Open", BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, ui.font );
    ui.separate_check = add_control( window, WC_BUTTONW, L"Picked PDFs in separate windows",
                                     BS_AUTOCHECKBOX | WS_TABSTOP, id_separate, ui.font );
    ui.status = add_control( window, WC_STATICW, L"", SS_NOPREFIX | SS_ENDELLIPSIS, 0, ui.font );

    ListView_SetExtendedListViewStyle( ui.list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
    const wchar_t *headings[] = { L"Name", L"Folder", L"Side", L"Modified" };
    for ( int i = 0; i < 4; ++i ) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.pszText = const_cast<wchar_t *>( headings[i] );
        column.cx = scaled( 100, ui.dpi );
        ListView_InsertColumn( ui.list, i, &column );
    }
}

// Clicking anywhere on a row ticks it. Double-clicking opens it straight away.
void on_list_notify( NMHDR *header ) {
    auto *click = reinterpret_cast<NMITEMACTIVATE *>( header );
    if ( header->code == NM_CLICK && click->iItem >= 0 ) {
        LVHITTESTINFO hit{};
        hit.pt = click->ptAction;
        ListView_SubItemHitTest( ui.list, &hit );
        if ( hit.iItem >= 0 && !( hit.flags & LVHT_ONITEMSTATEICON ) ) {
            ListView_SetCheckState( ui.list, hit.iItem, !ListView_GetCheckState( ui.list, hit.iItem ) );
        }
    } else if ( header->code == NM_DBLCLK && click->iItem >= 0 ) {
        ListView_SetCheckState( ui.list, click->iItem, TRUE );
        open_current();
    }
}

LRESULT CALLBACK launcher_proc( HWND window, UINT message, WPARAM wparam, LPARAM lparam ) {
    switch ( message ) {
    case WM_CREATE:
        create_controls( window );
        return 0;
    case WM_SIZE:
        lay_out( LOWORD( lparam ), HIWORD( lparam ) );
        return 0;
    case WM_COMMAND: {
        int id = LOWORD( wparam ), code = HIWORD( wparam );
        if ( id == IDOK ) open_current();
        else if ( id == IDCANCEL ) DestroyWindow( window );
        else if ( id == id_edit ) show_editor( window );
        else if ( id == id_setups && code == LBN_SELCHANGE ) load_setup();
        else if ( id == id_setups && code == LBN_DBLCLK ) open_current();
        else if ( id == id_filter && code == EN_CHANGE ) show_rows();
        else if ( id == id_keys && code == BN_CLICKED ) show_rows();
        return 0;
    }
    case WM_NOTIFY:
        if ( reinterpret_cast<NMHDR *>( lparam )->hwndFrom == ui.list ) on_list_notify( reinterpret_cast<NMHDR *>( lparam ) );
        return 0;
    case profiles_changed_message: {
        const setup *chosen = current_setup();
        load_profiles_into_list( chosen ? chosen->title : L"" );
        return 0;
    }
    case WM_DESTROY:
        DeleteObject( ui.font );
        PostQuitMessage( 0 );
        return 0;
    }
    return DefWindowProcW( window, message, wparam, lparam );
}

}  // namespace

void show_launcher( const wstring &start_setup ) {
    INITCOMMONCONTROLSEX common{ sizeof( common ), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx( &common );
    ui.dpi = GetDpiForSystem();

    WNDCLASSEXW window_class{ sizeof( window_class ) };
    window_class.lpfnWndProc = launcher_proc;
    window_class.hInstance = GetModuleHandleW( nullptr );
    window_class.hCursor = LoadCursorW( nullptr, IDC_ARROW );
    window_class.hbrBackground = reinterpret_cast<HBRUSH>( COLOR_BTNFACE + 1 );
    window_class.lpszClassName = L"CramstationLauncher";
    RegisterClassExW( &window_class );

    RECT place = centred_on_screen( scaled( 900, ui.dpi ), scaled( 600, ui.dpi ) );
    HWND window = CreateWindowExW( 0, window_class.lpszClassName, L"Cramstation", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                   place.left, place.top, place.right - place.left, place.bottom - place.top,
                                   nullptr, nullptr, window_class.hInstance, nullptr );

    wstring title = start_setup.empty() ? trim( read_text_file( last_setup_path() ) ) : start_setup;
    load_profiles_into_list( title );
    ShowWindow( window, SW_SHOW );
    SetForegroundWindow( window );
    SetFocus( ui.filter_box );
}
