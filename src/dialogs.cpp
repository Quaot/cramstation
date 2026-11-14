#include "dialogs.h"

#include <commctrl.h>

#include <algorithm>
#include <set>

#include "files.h"
#include "ui.h"

namespace {

// ---------------------------------------------------------------------------
// Subject and name
// ---------------------------------------------------------------------------

enum name_id { id_subject = 300, id_name };

struct name_state {
    HWND window = nullptr, subject_label = nullptr, subject_box = nullptr, name_label = nullptr, name_box = nullptr;
    HWND ok_button = nullptr, cancel_button = nullptr;
    HFONT font = nullptr;
    UINT dpi = 96;
    name_answer answer;
};

name_state name_dialog;

wstring text_of( HWND control ) {
    int length = GetWindowTextLengthW( control );
    wstring text( length + 1, L'\0' );
    GetWindowTextW( control, text.data(), length + 1 );
    text.resize( length );
    return trim( text );
}

LRESULT CALLBACK name_proc( HWND window, UINT message, WPARAM wparam, LPARAM lparam ) {
    switch ( message ) {
    case WM_SIZE: {
        UINT dpi = name_dialog.dpi;
        int pad = scaled( 12, dpi ), row = scaled( 28, dpi ), label = scaled( 70, dpi );
        int width = LOWORD( lparam ) - 2 * pad - label;
        MoveWindow( name_dialog.subject_label, pad, pad + scaled( 5, dpi ), label, row, TRUE );
        MoveWindow( name_dialog.subject_box, pad + label, pad, width, scaled( 300, dpi ), TRUE );
        MoveWindow( name_dialog.name_label, pad, 2 * pad + row + scaled( 5, dpi ), label, row, TRUE );
        MoveWindow( name_dialog.name_box, pad + label, 2 * pad + row, width, row, TRUE );
        int button_width = scaled( 90, dpi ), bottom = HIWORD( lparam ) - pad - row;
        MoveWindow( name_dialog.cancel_button, LOWORD( lparam ) - pad - button_width, bottom, button_width, row, TRUE );
        MoveWindow( name_dialog.ok_button, LOWORD( lparam ) - 2 * pad - 2 * button_width, bottom, button_width, row, TRUE );
        return 0;
    }
    case WM_COMMAND:
        if ( LOWORD( wparam ) == IDOK ) {
            name_dialog.answer.subject = text_of( name_dialog.subject_box );
            name_dialog.answer.name = text_of( name_dialog.name_box );
            if ( name_dialog.answer.subject.empty() || name_dialog.answer.name.empty() ) {
                MessageBeep( MB_ICONWARNING );
                return 0;
            }
            name_dialog.answer.accepted = true;
            DestroyWindow( window );
        } else if ( LOWORD( wparam ) == IDCANCEL ) {
            DestroyWindow( window );
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow( window );
        return 0;
    case WM_DESTROY:
        DeleteObject( name_dialog.font );
        return 0;
    }
    return DefWindowProcW( window, message, wparam, lparam );
}

// ---------------------------------------------------------------------------
// Ticking PDFs before opening
// ---------------------------------------------------------------------------

enum pick_id { id_filter = 400, id_keys, id_list, id_separate };

struct pick_row {
    size_t item_index;
    pdf_file pdf;
};

struct pick_state {
    HWND window = nullptr, filter_label = nullptr, filter_box = nullptr, keys_check = nullptr, list = nullptr;
    HWND separate_check = nullptr, open_button = nullptr, cancel_button = nullptr;
    HFONT font = nullptr;
    UINT dpi = 96;
    const setup *chosen = nullptr;
    vector<pick_row> rows;
    vector<size_t> shown;
    std::set<wstring> ticked;   // kept while the filter hides rows
    pick_answer answer;
};

pick_state picker;

wstring row_key( const pick_row &row ) {
    return std::to_wstring( row.item_index ) + L"|" + row.pdf.path;
}

void remember_ticks() {
    int count = ListView_GetItemCount( picker.list );
    for ( int i = 0; i < count; ++i ) {
        wstring key = row_key( picker.rows[ picker.shown[i] ] );
        if ( ListView_GetCheckState( picker.list, i ) ) picker.ticked.insert( key );
        else picker.ticked.erase( key );
    }
}

void show_rows() {
    remember_ticks();
    wchar_t filter[256];
    GetWindowTextW( picker.filter_box, filter, 256 );
    bool show_keys = SendMessageW( picker.keys_check, BM_GETCHECK, 0, 0 ) == BST_CHECKED;

    SendMessageW( picker.list, WM_SETREDRAW, FALSE, 0 );
    ListView_DeleteAllItems( picker.list );
    picker.shown.clear();
    for ( size_t i = 0; i < picker.rows.size(); ++i ) {
        const pick_row &row = picker.rows[i];
        if ( !show_keys && is_answer_key( row.pdf.name ) ) continue;
        if ( filter[0] && !contains_ci( row.pdf.path, filter ) ) continue;

        int line = static_cast<int>( picker.shown.size() );
        picker.shown.push_back( i );
        wstring name = stem( row.pdf.name );
        wstring side = picker.chosen->items[ row.item_index ].side;
        wstring date = date_text( row.pdf.modified );
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = line;
        item.pszText = name.data();
        ListView_InsertItem( picker.list, &item );
        ListView_SetItemText( picker.list, line, 1, const_cast<wchar_t *>( row.pdf.folder.c_str() ) );
        ListView_SetItemText( picker.list, line, 2, side.data() );
        ListView_SetItemText( picker.list, line, 3, date.data() );
        if ( picker.ticked.count( row_key( row ) ) ) ListView_SetCheckState( picker.list, line, TRUE );
    }
    SendMessageW( picker.list, WM_SETREDRAW, TRUE, 0 );
    InvalidateRect( picker.list, nullptr, TRUE );
}

void accept_picks() {
    remember_ticks();
    picker.answer.picks.assign( picker.chosen->items.size(), {} );
    bool any = false;
    for ( const pick_row &row : picker.rows ) {
        if ( picker.ticked.count( row_key( row ) ) ) {
            picker.answer.picks[ row.item_index ].push_back( row.pdf.path );
            any = true;
        }
    }
    int selected = ListView_GetNextItem( picker.list, -1, LVNI_SELECTED );
    if ( !any && selected >= 0 ) {
        const pick_row &row = picker.rows[ picker.shown[selected] ];
        picker.answer.picks[ row.item_index ].push_back( row.pdf.path );
        any = true;
    }
    if ( !any ) {
        MessageBeep( MB_ICONWARNING );
        return;
    }
    picker.answer.separate = SendMessageW( picker.separate_check, BM_GETCHECK, 0, 0 ) == BST_CHECKED;
    picker.answer.accepted = true;
    DestroyWindow( picker.window );
}

void lay_out_picker( int width, int height ) {
    UINT dpi = picker.dpi;
    int pad = scaled( 8, dpi ), row = scaled( 28, dpi ), label = scaled( 40, dpi ), keys = scaled( 160, dpi );
    MoveWindow( picker.filter_label, pad, pad + scaled( 5, dpi ), label, row, TRUE );
    MoveWindow( picker.filter_box, pad + label, pad, width - 3 * pad - label - keys, row, TRUE );
    MoveWindow( picker.keys_check, width - pad - keys, pad, keys, row, TRUE );
    int bottom = height - pad - row;
    MoveWindow( picker.list, pad, 2 * pad + row, width - 2 * pad, bottom - 3 * pad - row, TRUE );
    int button = scaled( 100, dpi );
    MoveWindow( picker.separate_check, pad, bottom, scaled( 280, dpi ), row, TRUE );
    MoveWindow( picker.cancel_button, width - pad - button, bottom, button, row, TRUE );
    MoveWindow( picker.open_button, width - 2 * pad - 2 * button, bottom, button, row, TRUE );

    RECT area;
    GetClientRect( picker.list, &area );
    int folder = scaled( 130, dpi ), side = scaled( 55, dpi ), date = scaled( 95, dpi );
    ListView_SetColumnWidth( picker.list, 1, folder );
    ListView_SetColumnWidth( picker.list, 2, side );
    ListView_SetColumnWidth( picker.list, 3, date );
    ListView_SetColumnWidth( picker.list, 0, std::max( scaled( 120, dpi ), int( area.right ) - folder - side - date ) );
}

LRESULT CALLBACK pick_proc( HWND window, UINT message, WPARAM wparam, LPARAM lparam ) {
    switch ( message ) {
    case WM_SIZE:
        lay_out_picker( LOWORD( lparam ), HIWORD( lparam ) );
        return 0;
    case WM_COMMAND: {
        int id = LOWORD( wparam ), code = HIWORD( wparam );
        if ( id == IDOK ) accept_picks();
        else if ( id == IDCANCEL ) DestroyWindow( window );
        else if ( id == id_filter && code == EN_CHANGE ) show_rows();
        else if ( id == id_keys && code == BN_CLICKED ) show_rows();
        return 0;
    }
    case WM_NOTIFY: {
        auto *header = reinterpret_cast<NMHDR *>( lparam );
        auto *click = reinterpret_cast<NMITEMACTIVATE *>( lparam );
        if ( header->hwndFrom != picker.list || click->iItem < 0 ) return 0;
        // Clicking anywhere on a row ticks it. Double-clicking opens it straight away.
        if ( header->code == NM_CLICK ) {
            LVHITTESTINFO hit{};
            hit.pt = click->ptAction;
            ListView_SubItemHitTest( picker.list, &hit );
            if ( hit.iItem >= 0 && !( hit.flags & LVHT_ONITEMSTATEICON ) ) {
                ListView_SetCheckState( picker.list, hit.iItem, !ListView_GetCheckState( picker.list, hit.iItem ) );
            }
        } else if ( header->code == NM_DBLCLK ) {
            ListView_SetCheckState( picker.list, click->iItem, TRUE );
            accept_picks();
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow( window );
        return 0;
    case WM_DESTROY:
        DeleteObject( picker.font );
        return 0;
    }
    return DefWindowProcW( window, message, wparam, lparam );
}

}  // namespace

name_answer ask_for_name( HWND owner, const wchar_t *title, const vector<wstring> &subjects, const wstring &subject,
                          const wstring &name, bool subject_fixed ) {
    name_dialog = {};
    name_dialog.dpi = GetDpiForSystem();
    UINT dpi = name_dialog.dpi;
    HWND window = make_window( L"CramstationName", name_proc, title, scaled( 420, dpi ), scaled( 200, dpi ), owner,
                               WS_POPUP | WS_CAPTION | WS_SYSMENU );
    name_dialog.window = window;
    name_dialog.font = make_font( dpi, L"Segoe UI", 10 );
    HFONT font = name_dialog.font;
    name_dialog.subject_label = add_control( window, WC_STATICW, L"Subject", 0, 0, font );
    name_dialog.subject_box = add_control( window, WC_COMBOBOXW, L"", CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP,
                                           id_subject, font );
    name_dialog.name_label = add_control( window, WC_STATICW, L"Setup", 0, 0, font );
    name_dialog.name_box = add_control( window, WC_EDITW, name.c_str(), ES_AUTOHSCROLL | WS_TABSTOP, id_name, font, WS_EX_CLIENTEDGE );
    name_dialog.ok_button = add_control( window, WC_BUTTONW, L"OK", BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, font );
    name_dialog.cancel_button = add_control( window, WC_BUTTONW, L"Cancel", WS_TABSTOP, IDCANCEL, font );

    for ( const wstring &each : subjects ) {
        SendMessageW( name_dialog.subject_box, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>( each.c_str() ) );
    }
    SetWindowTextW( name_dialog.subject_box, subject.c_str() );
    EnableWindow( name_dialog.subject_box, !subject_fixed );

    RECT client;
    GetClientRect( window, &client );
    SendMessageW( window, WM_SIZE, 0, MAKELPARAM( client.right, client.bottom ) );
    SetFocus( subject_fixed || !subject.empty() ? name_dialog.name_box : name_dialog.subject_box );
    SendMessageW( name_dialog.name_box, EM_SETSEL, 0, -1 );
    run_modal( window, owner );
    return name_dialog.answer;
}

pick_answer ask_for_picks( HWND owner, const setup &chosen ) {
    picker = {};
    picker.chosen = &chosen;
    picker.dpi = GetDpiForSystem();
    UINT dpi = picker.dpi;
    HWND window = make_window( L"CramstationPick", pick_proc, ( L"Open " + chosen.title ).c_str(), scaled( 760, dpi ),
                               scaled( 520, dpi ), owner );
    picker.window = window;
    picker.font = make_font( dpi, L"Segoe UI", 10 );
    HFONT font = picker.font;
    picker.filter_label = add_control( window, WC_STATICW, L"Filter", 0, 0, font );
    picker.filter_box = add_control( window, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, id_filter, font, WS_EX_CLIENTEDGE );
    picker.keys_check = add_control( window, WC_BUTTONW, L"Show answer keys", BS_AUTOCHECKBOX | WS_TABSTOP, id_keys, font );
    picker.list = add_control( window, WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_TABSTOP, id_list,
                               font, WS_EX_CLIENTEDGE );
    picker.separate_check = add_control( window, WC_BUTTONW, L"Ticked PDFs in separate windows", BS_AUTOCHECKBOX | WS_TABSTOP,
                                         id_separate, font );
    picker.open_button = add_control( window, WC_BUTTONW, L"Open", BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, font );
    picker.cancel_button = add_control( window, WC_BUTTONW, L"Cancel", WS_TABSTOP, IDCANCEL, font );

    ListView_SetExtendedListViewStyle( picker.list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
    const wchar_t *headings[] = { L"Name", L"Folder", L"Side", L"Modified" };
    for ( int i = 0; i < 4; ++i ) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.pszText = const_cast<wchar_t *>( headings[i] );
        column.cx = scaled( 100, dpi );
        ListView_InsertColumn( picker.list, i, &column );
    }

    for ( size_t i = 0; i < chosen.items.size(); ++i ) {
        if ( chosen.items[i].kind != item_kind::pick ) continue;
        for ( const pdf_file &pdf : find_pdfs( chosen.items[i].paths, true ) ) picker.rows.push_back( { i, pdf } );
    }
    show_rows();
    if ( !picker.shown.empty() ) ListView_SetItemState( picker.list, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );

    RECT client;
    GetClientRect( window, &client );
    lay_out_picker( client.right, client.bottom );
    SetFocus( picker.filter_box );
    run_modal( window, owner );
    return picker.answer;
}
