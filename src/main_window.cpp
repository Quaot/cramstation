#include "ui.h"

#include <commctrl.h>

#include <cwctype>

#include "canvas.h"
#include "desktops.h"
#include "dialogs.h"
#include "launch.h"

namespace {

enum control_id { id_setups = 100, id_new, id_rename, id_delete, id_desktop, id_notes, id_text };

struct main_state {
    HWND window = nullptr;
    HWND setups_label = nullptr, setups_box = nullptr, new_button = nullptr, rename_button = nullptr, delete_button = nullptr;
    HWND desktop_label = nullptr, desktop_box = nullptr, notes_button = nullptr, text_button = nullptr;
    HWND canvas = nullptr, hint = nullptr, open_button = nullptr;
    HFONT font = nullptr;
    UINT dpi = 96;
    profile_book book;
    vector<setup> setups;           // in the order of the list
    vector<wstring> desktops;       // names in the desktop list, after "This desktop"
};

main_state ui;

wstring last_setup_path() {
    return join_path( parent_folder( profiles_path() ), L"last setup.txt" );
}

void set_hint( const wstring &text ) {
    SetWindowTextW( ui.hint, text.c_str() );
}

const setup *current_setup() {
    LRESULT index = SendMessageW( ui.setups_box, LB_GETCURSEL, 0, 0 );
    if ( index == LB_ERR || index >= LRESULT( ui.setups.size() ) ) return nullptr;
    return &ui.setups[index];
}

void show_desktop_choice( const profile_block &profile ) {
    SetWindowTextW( ui.desktop_label, ( L"Desktop for " + profile.name ).c_str() );
    SendMessageW( ui.desktop_box, CB_RESETCONTENT, 0, 0 );
    SendMessageW( ui.desktop_box, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>( L"Whichever is showing" ) );
    ui.desktops = desktop_names();
    int selected = 0;
    for ( size_t i = 0; i < ui.desktops.size(); ++i ) {
        SendMessageW( ui.desktop_box, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>( ui.desktops[i].c_str() ) );
        if ( lower( ui.desktops[i] ) == lower( profile.desktop ) ) selected = int( i ) + 1;
    }
    SendMessageW( ui.desktop_box, CB_SETCURSEL, selected, 0 );
}

void show_current_setup() {
    const setup *chosen = current_setup();
    bool has_setup = chosen != nullptr;
    EnableWindow( ui.rename_button, has_setup );
    EnableWindow( ui.delete_button, has_setup );
    EnableWindow( ui.desktop_box, has_setup );
    EnableWindow( ui.notes_button, has_setup );
    EnableWindow( ui.open_button, has_setup );
    if ( !has_setup ) {
        show_items( nullptr, L"" );
        return;
    }
    profile_block &profile = ui.book.profiles[chosen->profile_index];
    show_items( &items_of( ui.book, chosen->profile_index, chosen->setup_index ), root_of( profile ) );
    show_desktop_choice( profile );
}

void fill_setup_list( const wstring &select_title ) {
    ui.setups = all_setups( ui.book );
    SendMessageW( ui.setups_box, LB_RESETCONTENT, 0, 0 );
    int selected = ui.setups.empty() ? -1 : 0;
    for ( size_t i = 0; i < ui.setups.size(); ++i ) {
        SendMessageW( ui.setups_box, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>( ui.setups[i].title.c_str() ) );
        if ( lower( ui.setups[i].title ) == lower( select_title ) ) selected = int( i );
    }
    SendMessageW( ui.setups_box, LB_SETCURSEL, selected, 0 );
    show_current_setup();
}

void save() {
    if ( !save_book( ui.book ) ) set_hint( L"Could not save profiles.txt." );
    ui.setups = all_setups( ui.book );
}

void reload_from_file() {
    const setup *chosen = current_setup();
    wstring title = chosen ? chosen->title : L"";
    ui.book = load_book();
    fill_setup_list( title );
    if ( !ui.book.errors.empty() ) set_hint( L"profiles.txt has a problem, " + ui.book.errors[0] );
}

// Course folders in Downloads, such as "ECE 150", that are not subjects yet
vector<wstring> subject_choices() {
    vector<wstring> choices;
    for ( const profile_block &profile : ui.book.profiles ) choices.push_back( profile.name );
    WIN32_FIND_DATAW data;
    wstring downloads = join_path( user_folder(), L"Downloads" );
    HANDLE find = FindFirstFileW( join_path( downloads, L"*" ).c_str(), &data );
    if ( find == INVALID_HANDLE_VALUE ) return choices;
    do {
        wstring name = data.cFileName;
        bool is_course = ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && name.size() >= 4 &&
                         std::iswdigit( name.back() ) && std::iswalpha( name[0] );
        bool known = false;
        for ( const wstring &choice : choices ) if ( lower( choice ) == lower( name ) ) known = true;
        if ( is_course && !known ) choices.push_back( name );
    } while ( FindNextFileW( find, &data ) );
    FindClose( find );
    return choices;
}

// A desktop named after the course or its number, such as "150" for ECE 150
wstring matching_desktop( const wstring &subject ) {
    wstring number;
    for ( wchar_t c : subject ) if ( std::iswdigit( c ) ) number += c;
    for ( const wstring &name : desktop_names() ) {
        if ( lower( name ) == lower( subject ) || ( !number.empty() && name == number ) ) return name;
    }
    return L"";
}

void new_setup() {
    const setup *chosen = current_setup();
    wstring subject = chosen ? ui.book.profiles[chosen->profile_index].name : L"";
    name_answer answer = ask_for_name( ui.window, L"New setup", subject_choices(), subject, L"Lecture", false );
    if ( !answer.accepted ) return;

    profile_block *profile = nullptr;
    for ( profile_block &existing : ui.book.profiles ) if ( lower( existing.name ) == lower( answer.subject ) ) profile = &existing;
    if ( !profile ) {
        wstring course_folder = join_path( join_path( user_folder(), L"Downloads" ), answer.subject );
        ui.book.profiles.push_back( { answer.subject } );
        profile = &ui.book.profiles.back();
        if ( is_folder( course_folder ) ) profile->root = L"Downloads\\" + answer.subject;
        profile->desktop = matching_desktop( answer.subject );
    }
    // A subject that opened as it was becomes a setup of its own, so both can be listed
    if ( profile->setups.empty() && !profile->items.empty() ) {
        profile->setups.push_back( { L"Main", L"", L"", profile->items } );
        profile->items.clear();
    }
    profile->setups.push_back( { answer.name } );
    save();
    fill_setup_list( profile->name + L": " + answer.name );
    set_hint( L"Now drag PDFs or folders from File Explorer onto the screen." );
}

void rename_setup() {
    const setup *chosen = current_setup();
    if ( !chosen ) return;
    profile_block &profile = ui.book.profiles[chosen->profile_index];
    bool whole_subject = chosen->setup_index < 0;
    wstring old_name = whole_subject ? profile.name : profile.setups[chosen->setup_index].name;
    name_answer answer = ask_for_name( ui.window, L"Rename", { profile.name }, profile.name, old_name, !whole_subject );
    if ( !answer.accepted ) return;
    if ( whole_subject ) profile.name = answer.name;
    else profile.setups[chosen->setup_index].name = answer.name;
    save();
    fill_setup_list( whole_subject ? answer.name : profile.name + L": " + answer.name );
}

void delete_setup() {
    const setup *chosen = current_setup();
    if ( !chosen ) return;
    wstring question = L"Delete " + chosen->title + L"? Its files stay where they are.";
    if ( MessageBoxW( ui.window, question.c_str(), L"Cramstation", MB_YESNO | MB_ICONQUESTION ) != IDYES ) return;
    profile_block &profile = ui.book.profiles[chosen->profile_index];
    if ( chosen->setup_index >= 0 ) profile.setups.erase( profile.setups.begin() + chosen->setup_index );
    if ( chosen->setup_index < 0 || ( profile.setups.empty() && profile.items.empty() ) ) {
        ui.book.profiles.erase( ui.book.profiles.begin() + chosen->profile_index );
    }
    save();
    fill_setup_list( L"" );
}

void choose_desktop() {
    const setup *chosen = current_setup();
    if ( !chosen ) return;
    LRESULT index = SendMessageW( ui.desktop_box, CB_GETCURSEL, 0, 0 );
    ui.book.profiles[chosen->profile_index].desktop = index > 0 ? ui.desktops[index - 1] : L"";
    save();
}

void open_current() {
    const setup *found = current_setup();
    if ( !found ) return;
    save();
    setup chosen = *found;
    if ( chosen.items.empty() ) {
        set_hint( L"This setup is empty. Drag PDFs or folders onto the screen first." );
        return;
    }

    pick_answer picks;
    for ( const layout_item &item : chosen.items ) {
        if ( item.kind == item_kind::pick ) {
            picks = ask_for_picks( ui.window, chosen );
            if ( !picks.accepted ) return;
            break;
        }
    }

    ShowWindow( ui.window, SW_HIDE );
    vector<wstring> problems = open_setup( chosen, picks.picks, picks.separate );
    write_text_file( last_setup_path(), chosen.title );
    if ( !problems.empty() ) {
        wstring text;
        for ( const wstring &problem : problems ) text += problem + L"\r\n";
        MessageBoxW( nullptr, text.c_str(), L"Cramstation", MB_ICONINFORMATION );
    }
    DestroyWindow( ui.window );
}

void lay_out( int width, int height ) {
    UINT dpi = ui.dpi;
    int pad = scaled( 10, dpi ), row = scaled( 30, dpi ), list_width = scaled( 250, dpi );
    int bottom = height - pad - row;

    MoveWindow( ui.setups_label, pad, pad + scaled( 6, dpi ), list_width, row, TRUE );
    MoveWindow( ui.setups_box, pad, pad + row, list_width, bottom - 2 * pad - row, TRUE );
    int button = ( list_width - 2 * scaled( 6, dpi ) ) / 3;
    HWND buttons[] = { ui.new_button, ui.rename_button, ui.delete_button };
    for ( int i = 0; i < 3; ++i ) MoveWindow( buttons[i], pad + i * ( button + scaled( 6, dpi ) ), bottom, button, row, TRUE );

    int x = pad + list_width + pad;
    int right_width = width - x - pad;
    int label = scaled( 150, dpi ), box = scaled( 190, dpi ), tool = scaled( 110, dpi );
    MoveWindow( ui.desktop_label, x, pad + scaled( 6, dpi ), label, row, TRUE );
    MoveWindow( ui.desktop_box, x + label, pad, box, scaled( 300, dpi ), TRUE );
    MoveWindow( ui.text_button, width - pad - tool, pad, tool, row, TRUE );
    MoveWindow( ui.notes_button, width - 2 * pad - 2 * tool, pad, tool, row, TRUE );

    MoveWindow( ui.canvas, x, pad + row + pad, right_width, bottom - 3 * pad - row, TRUE );
    int open_width = scaled( 130, dpi );
    MoveWindow( ui.hint, x, bottom + scaled( 6, dpi ), right_width - open_width - pad, row, TRUE );
    MoveWindow( ui.open_button, width - pad - open_width, bottom, open_width, row, TRUE );
}

void create_controls( HWND window ) {
    ui.window = window;
    ui.font = make_font( ui.dpi, L"Segoe UI", 10 );
    HFONT font = ui.font;
    ui.setups_label = add_control( window, WC_STATICW, L"Setups", 0, 0, font );
    ui.setups_box = add_control( window, WC_LISTBOXW, L"", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP,
                                 id_setups, font, WS_EX_CLIENTEDGE );
    ui.new_button = add_control( window, WC_BUTTONW, L"New setup", WS_TABSTOP, id_new, font );
    ui.rename_button = add_control( window, WC_BUTTONW, L"Rename", WS_TABSTOP, id_rename, font );
    ui.delete_button = add_control( window, WC_BUTTONW, L"Delete", WS_TABSTOP, id_delete, font );
    ui.desktop_label = add_control( window, WC_STATICW, L"Desktop", SS_NOPREFIX | SS_ENDELLIPSIS, 0, font );
    ui.desktop_box = add_control( window, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, id_desktop, font );
    ui.notes_button = add_control( window, WC_BUTTONW, L"Add notes", WS_TABSTOP, id_notes, font );
    ui.text_button = add_control( window, WC_BUTTONW, L"Edit as text", WS_TABSTOP, id_text, font );
    ui.canvas = create_canvas( window, font, ui.dpi );
    ui.hint = add_control( window, WC_STATICW, L"", SS_NOPREFIX | SS_ENDELLIPSIS, 0, font );
    ui.open_button = add_control( window, WC_BUTTONW, L"Open", BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, font );
}

LRESULT CALLBACK main_proc( HWND window, UINT message, WPARAM wparam, LPARAM lparam ) {
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
        else if ( id == id_new ) new_setup();
        else if ( id == id_rename ) rename_setup();
        else if ( id == id_delete ) delete_setup();
        else if ( id == id_notes ) add_notes_tile();
        else if ( id == id_text ) show_editor( window );
        else if ( id == id_desktop && code == CBN_SELCHANGE ) choose_desktop();
        else if ( id == id_setups && code == LBN_SELCHANGE ) show_current_setup();
        else if ( id == id_setups && code == LBN_DBLCLK ) open_current();
        return 0;
    }
    case canvas_changed_message:
        save();
        return 0;
    case canvas_hint_message:
        set_hint( reinterpret_cast<const wchar_t *>( lparam ) );
        return 0;
    case profiles_changed_message:
        reload_from_file();
        return 0;
    case WM_DESTROY:
        DeleteObject( ui.font );
        PostQuitMessage( 0 );
        return 0;
    }
    return DefWindowProcW( window, message, wparam, lparam );
}

}  // namespace

void show_main_window( const wstring &start_setup ) {
    INITCOMMONCONTROLSEX common{ sizeof( common ), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx( &common );
    ui.dpi = GetDpiForSystem();

    HWND window = make_window( L"CramstationMain", main_proc, L"Cramstation", scaled( 1040, ui.dpi ), scaled( 660, ui.dpi ), nullptr );
    ui.book = load_book();
    wstring title = start_setup.empty() ? trim( read_text_file( last_setup_path() ) ) : start_setup;
    fill_setup_list( title );
    if ( !ui.book.errors.empty() ) set_hint( L"profiles.txt has a problem, " + ui.book.errors[0] );
    else set_hint( L"Pick a setup and press Open, or make one with New setup." );

    RECT client;
    GetClientRect( window, &client );
    lay_out( client.right, client.bottom );
    ShowWindow( window, SW_SHOW );
    SetForegroundWindow( window );
    SetFocus( ui.setups_box );
}
