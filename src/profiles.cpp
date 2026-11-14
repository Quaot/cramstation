#include "profiles.h"

#include <windows.h>

namespace {

const wchar_t *header =
    L"# Crammy profiles. The Crammy window writes this file, and you can also edit it by hand.\r\n"
    L"#\r\n"
    L"# [Subject] starts a subject. [Subject: Setup] adds a setup to it.\r\n"
    L"#\r\n"
    L"# Settings\r\n"
    L"#   root = Downloads\\ECE 150     the folder other paths start from (inside your user folder)\r\n"
    L"#   notes folder = 2 Practice    where \"notes\" keeps answer files (default \"2 Practice\", else root)\r\n"
    L"#   desktop = 150                switch to the virtual desktop with this name first\r\n"
    L"#\r\n"
    L"# Windows are written as  side = what\r\n"
    L"#   side is left or right, or full for the whole screen. Windows on one side stack top to bottom.\r\n"
    L"#\r\n"
    L"#   notes                       Notepad++ with \"<first PDF> - my answers (date).txt\"\r\n"
    L"#   notes \"file.txt\"            Notepad++ with that file, made if missing\r\n"
    L"#   pdf \"file.pdf\"              one PDF\r\n"
    L"#   newest \"Lec*.pdf\"           the newest PDF whose name fits; * matches any text\r\n"
    L"#   tabs \"a.pdf\" \"b.pdf\"        one PdfCraft window, one tab per PDF\r\n"
    L"#   pick \"folder\" \"file\"        tick PDFs from these each time you open the setup\r\n"
    L"#   folder \"folder\"             File Explorer on that folder\r\n"
    L"#\r\n"
    L"# Add  with acrobat  at the end of a PDF line to open it in Adobe Acrobat in place of PdfCraft.\r\n";

const wchar_t *kind_words[] = { L"notes", L"pdf", L"tabs", L"pick", L"folder", L"newest" };

bool read_kind( const wstring &word, item_kind &kind ) {
    for ( int i = 0; i < 6; ++i ) {
        if ( word == kind_words[i] ) {
            kind = static_cast<item_kind>( i );
            return true;
        }
    }
    return false;
}

vector<wstring> read_paths( const wstring &text ) {
    vector<wstring> paths;
    if ( text.find( L'"' ) == wstring::npos ) {
        if ( !trim( text ).empty() ) paths.push_back( trim( text ) );
        return paths;
    }
    size_t position = 0;
    while ( ( position = text.find( L'"', position ) ) != wstring::npos ) {
        size_t end = text.find( L'"', position + 1 );
        if ( end == wstring::npos ) end = text.size();
        paths.push_back( text.substr( position + 1, end - position - 1 ) );
        position = end + 1;
    }
    return paths;
}

wstring first_path( const wstring &text ) {
    vector<wstring> paths = read_paths( text );
    return paths.empty() ? L"" : paths[0];
}

// Splits "\"a.pdf\" with acrobat" into its paths part and the viewer
viewer take_viewer( wstring &rest ) {
    size_t last_quote = rest.rfind( L'"' );
    size_t with = lower( rest ).rfind( L" with " );
    if ( with == wstring::npos && starts_with( lower( rest ), L"with " ) ) with = 0;
    if ( with == wstring::npos || ( last_quote != wstring::npos && with < last_quote ) ) return viewer::pdfcraft;
    wstring word = lower( trim( rest.substr( with + 5 ) ) );
    rest = rest.substr( 0, with );
    return word == L"acrobat" ? viewer::acrobat : viewer::pdfcraft;
}

wstring check_count( item_kind kind, size_t count ) {
    if ( kind == item_kind::notes && count > 1 ) return L"notes takes at most one file";
    if ( ( kind == item_kind::pdf || kind == item_kind::folder || kind == item_kind::newest ) && count != 1 ) {
        return L"this takes exactly one path";
    }
    if ( ( kind == item_kind::tabs || kind == item_kind::pick ) && count == 0 ) return L"this needs at least one path";
    return L"";
}

wstring resolve( const wstring &folder, const wstring &path ) {
    wstring expanded = expand_variables( path );
    return is_absolute( expanded ) ? expanded : join_path( folder, expanded );
}

wstring write_item( const written_item &item ) {
    wstring line = item.side + L" = " + kind_words[ static_cast<int>( item.kind ) ];
    for ( const wstring &path : item.paths ) line += L" \"" + path + L"\"";
    if ( item.app == viewer::acrobat ) line += L" with acrobat";
    return line + L"\r\n";
}

wstring write_settings( const wstring &root, const wstring &notes_folder, const wstring &desktop ) {
    wstring text;
    if ( !root.empty() ) text += L"root = " + root + L"\r\n";
    if ( !notes_folder.empty() ) text += L"notes folder = " + notes_folder + L"\r\n";
    if ( !desktop.empty() ) text += L"desktop = " + desktop + L"\r\n";
    return text;
}

setup build_setup( const profile_block &profile, size_t profile_index, int setup_index ) {
    const setup_block *own = setup_index >= 0 ? &profile.setups[setup_index] : nullptr;
    setup result;
    result.title = own ? profile.name + L": " + own->name : profile.name;
    result.profile_index = profile_index;
    result.setup_index = setup_index;
    result.root = root_of( profile );

    wstring notes_folder = ( own && !own->notes_folder.empty() ) ? own->notes_folder : profile.notes_folder;
    if ( !notes_folder.empty() ) result.notes_folder = resolve( result.root, notes_folder );
    else if ( is_folder( join_path( result.root, L"2 Practice" ) ) ) result.notes_folder = join_path( result.root, L"2 Practice" );
    else result.notes_folder = result.root;

    result.desktop = ( own && !own->desktop.empty() ) ? own->desktop : profile.desktop;

    vector<written_item> items = profile.items;
    if ( own ) items.insert( items.end(), own->items.begin(), own->items.end() );
    for ( const written_item &item : items ) {
        layout_item resolved{ item.side, item.kind, {}, item.app };
        for ( const wstring &path : item.paths ) resolved.paths.push_back( resolve( result.root, path ) );
        result.items.push_back( resolved );
    }
    return result;
}

}  // namespace

wstring profiles_path() {
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW( nullptr, exe_path, MAX_PATH );
    return join_path( parent_folder( exe_path ), L"profiles.txt" );
}

wstring root_of( const profile_block &profile ) {
    return resolve( user_folder(), profile.root );
}

// Paths inside the root are stored relative to it, so the file stays short and readable
wstring relative_to( const wstring &root, const wstring &path ) {
    wstring prefix = root + L"\\";
    if ( lower( path ) == lower( root ) ) return L".";
    if ( starts_with( lower( path ), lower( prefix ) ) ) return path.substr( prefix.size() );
    return path;
}

vector<written_item> &items_of( profile_book &book, size_t profile_index, int setup_index ) {
    profile_block &profile = book.profiles[profile_index];
    return setup_index >= 0 ? profile.setups[setup_index].items : profile.items;
}

profile_book read_book( const wstring &text ) {
    profile_book book;
    profile_block *profile = nullptr;
    setup_block *own = nullptr;
    int line_number = 0;

    size_t start = 0;
    while ( start <= text.size() ) {
        size_t end = text.find( L'\n', start );
        if ( end == wstring::npos ) end = text.size();
        wstring line = trim( text.substr( start, end - start ) );
        start = end + 1;
        ++line_number;
        wstring where = L"line " + std::to_wstring( line_number ) + L": ";

        if ( line.empty() || line[0] == L'#' ) continue;

        if ( line[0] == L'[' ) {
            if ( line.back() != L']' ) { book.errors.push_back( where + L"a heading needs a closing ]" ); continue; }
            wstring heading = line.substr( 1, line.size() - 2 );
            size_t colon = heading.find( L':' );
            wstring profile_name = trim( heading.substr( 0, colon ) );
            wstring setup_name = ( colon == wstring::npos ) ? L"" : trim( heading.substr( colon + 1 ) );

            profile = nullptr;
            for ( profile_block &existing : book.profiles ) {
                if ( lower( existing.name ) == lower( profile_name ) ) profile = &existing;
            }
            if ( !profile ) {
                book.profiles.push_back( { profile_name } );
                profile = &book.profiles.back();
            }
            own = nullptr;
            if ( !setup_name.empty() ) {
                profile->setups.push_back( { setup_name } );
                own = &profile->setups.back();
            }
            continue;
        }

        size_t equals = line.find( L'=' );
        if ( equals == wstring::npos ) { book.errors.push_back( where + L"expected  name = value" ); continue; }
        if ( !profile ) { book.errors.push_back( where + L"this line needs a [Subject] heading above it" ); continue; }

        wstring key = lower( trim( line.substr( 0, equals ) ) );
        wstring value = trim( line.substr( equals + 1 ) );

        if ( key == L"root" ) { profile->root = first_path( value ); continue; }
        if ( key == L"notes folder" ) { ( own ? own->notes_folder : profile->notes_folder ) = first_path( value ); continue; }
        if ( key == L"desktop" ) { ( own ? own->desktop : profile->desktop ) = first_path( value ); continue; }

        if ( key != L"left" && key != L"right" && key != L"full" ) {
            book.errors.push_back( where + L"unknown setting '" + key + L"' (use root, notes folder, desktop, left, right or full)" );
            continue;
        }

        size_t space = value.find_first_of( L" \t" );
        wstring word = lower( value.substr( 0, space ) );
        wstring rest = ( space == wstring::npos ) ? L"" : value.substr( space + 1 );
        item_kind kind;
        if ( !read_kind( word, kind ) ) {
            book.errors.push_back( where + L"unknown window '" + word + L"' (use notes, pdf, newest, tabs, pick or folder)" );
            continue;
        }
        viewer app = take_viewer( rest );
        vector<wstring> paths = read_paths( rest );
        wstring problem = check_count( kind, paths.size() );
        if ( !problem.empty() ) { book.errors.push_back( where + problem ); continue; }
        ( own ? own->items : profile->items ).push_back( { key, kind, paths, app } );
    }
    return book;
}

profile_book load_book() {
    wstring path = profiles_path();
    if ( !is_file( path ) ) return {};
    return read_book( read_text_file( path ) );
}

wstring write_book( const profile_book &book ) {
    wstring text = header;
    for ( const profile_block &profile : book.profiles ) {
        text += L"\r\n\r\n[" + profile.name + L"]\r\n";
        text += write_settings( profile.root, profile.notes_folder, profile.desktop );
        for ( const written_item &item : profile.items ) text += write_item( item );
        for ( const setup_block &own : profile.setups ) {
            text += L"\r\n[" + profile.name + L": " + own.name + L"]\r\n";
            text += write_settings( L"", own.notes_folder, own.desktop );
            for ( const written_item &item : own.items ) text += write_item( item );
        }
    }
    return text;
}

bool save_book( const profile_book &book ) {
    return write_text_file( profiles_path(), write_book( book ) );
}

vector<setup> all_setups( const profile_book &book ) {
    vector<setup> setups;
    for ( size_t p = 0; p < book.profiles.size(); ++p ) {
        const profile_block &profile = book.profiles[p];
        if ( profile.setups.empty() ) setups.push_back( build_setup( profile, p, -1 ) );
        for ( size_t s = 0; s < profile.setups.size(); ++s ) setups.push_back( build_setup( profile, p, static_cast<int>( s ) ) );
    }
    return setups;
}

profile_file load_profiles() {
    profile_book book = load_book();
    if ( !is_file( profiles_path() ) ) return { {}, { L"No profiles file at " + profiles_path() } };
    return { all_setups( book ), book.errors };
}

const setup *find_setup( const profile_file &profiles, const wstring &title ) {
    for ( const setup &each : profiles.setups ) {
        if ( lower( each.title ) == lower( title ) ) return &each;
    }
    const setup *partial = nullptr;
    for ( const setup &each : profiles.setups ) {
        if ( contains_ci( each.title, title ) ) {
            if ( partial ) return nullptr;
            partial = &each;
        }
    }
    return partial;
}
