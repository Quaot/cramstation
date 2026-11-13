#include "profiles.h"

#include <windows.h>

namespace {

struct written_item {
    wstring side;
    item_kind kind;
    vector<wstring> paths;  // as written, not yet joined to the root
};

struct block {
    wstring name;
    wstring root;
    wstring notes_folder;
    wstring desktop;
    vector<written_item> items;
    vector<block> setups;
};

bool read_kind( const wstring &word, item_kind &kind ) {
    if ( word == L"notes" ) kind = item_kind::notes;
    else if ( word == L"pdf" ) kind = item_kind::pdf;
    else if ( word == L"tabs" ) kind = item_kind::tabs;
    else if ( word == L"pick" ) kind = item_kind::pick;
    else if ( word == L"folder" ) kind = item_kind::folder;
    else return false;
    return true;
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

wstring check_count( item_kind kind, size_t count ) {
    if ( kind == item_kind::notes && count > 1 ) return L"notes takes at most one file";
    if ( ( kind == item_kind::pdf || kind == item_kind::folder ) && count != 1 ) return L"this takes exactly one path";
    if ( ( kind == item_kind::tabs || kind == item_kind::pick ) && count == 0 ) return L"this needs at least one path";
    return L"";
}

wstring resolve( const wstring &folder, const wstring &path ) {
    wstring expanded = expand_variables( path );
    return is_absolute( expanded ) ? expanded : join_path( folder, expanded );
}

setup build_setup( const wstring &title, const block &profile, const block *own ) {
    setup result;
    result.title = title;

    wstring root = ( own && !own->root.empty() ) ? own->root : profile.root;
    result.root = resolve( user_folder(), root );

    wstring notes_folder = ( own && !own->notes_folder.empty() ) ? own->notes_folder : profile.notes_folder;
    if ( !notes_folder.empty() ) result.notes_folder = resolve( result.root, notes_folder );
    else if ( is_folder( join_path( result.root, L"2 Practice" ) ) ) result.notes_folder = join_path( result.root, L"2 Practice" );
    else result.notes_folder = result.root;

    result.desktop = ( own && !own->desktop.empty() ) ? own->desktop : profile.desktop;

    vector<written_item> items = profile.items;
    if ( own ) items.insert( items.end(), own->items.begin(), own->items.end() );
    for ( const written_item &item : items ) {
        layout_item resolved{ item.side, item.kind, {} };
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

profile_file read_profiles( const wstring &text ) {
    profile_file result;
    vector<block> profiles;
    block *current = nullptr;
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
            if ( line.back() != L']' ) { result.errors.push_back( where + L"a heading needs a closing ]" ); continue; }
            wstring heading = line.substr( 1, line.size() - 2 );
            size_t colon = heading.find( L':' );
            wstring profile_name = trim( heading.substr( 0, colon ) );
            wstring setup_name = ( colon == wstring::npos ) ? L"" : trim( heading.substr( colon + 1 ) );

            block *profile = nullptr;
            for ( block &existing : profiles ) {
                if ( lower( existing.name ) == lower( profile_name ) ) profile = &existing;
            }
            if ( !profile ) {
                profiles.push_back( { profile_name } );
                profile = &profiles.back();
            }
            if ( setup_name.empty() ) current = profile;
            else {
                profile->setups.push_back( { setup_name } );
                current = &profile->setups.back();
            }
            continue;
        }

        size_t equals = line.find( L'=' );
        if ( equals == wstring::npos ) { result.errors.push_back( where + L"expected  name = value" ); continue; }
        if ( !current ) { result.errors.push_back( where + L"this line needs a [Profile] heading above it" ); continue; }

        wstring key = lower( trim( line.substr( 0, equals ) ) );
        wstring value = trim( line.substr( equals + 1 ) );

        if ( key == L"root" ) { current->root = read_paths( value ).empty() ? L"" : read_paths( value )[0]; continue; }
        if ( key == L"notes folder" ) { current->notes_folder = read_paths( value ).empty() ? L"" : read_paths( value )[0]; continue; }
        if ( key == L"desktop" ) { current->desktop = read_paths( value ).empty() ? L"" : read_paths( value )[0]; continue; }

        if ( key != L"left" && key != L"right" && key != L"full" ) {
            result.errors.push_back( where + L"unknown setting '" + key + L"' (use root, notes folder, desktop, left, right or full)" );
            continue;
        }

        size_t space = value.find_first_of( L" \t" );
        wstring word = lower( value.substr( 0, space ) );
        wstring rest = ( space == wstring::npos ) ? L"" : value.substr( space + 1 );
        item_kind kind;
        if ( !read_kind( word, kind ) ) {
            result.errors.push_back( where + L"unknown window '" + word + L"' (use notes, pdf, tabs, pick or folder)" );
            continue;
        }
        vector<wstring> paths = read_paths( rest );
        wstring problem = check_count( kind, paths.size() );
        if ( !problem.empty() ) { result.errors.push_back( where + problem ); continue; }
        current->items.push_back( { key, kind, paths } );
    }

    for ( const block &profile : profiles ) {
        if ( profile.setups.empty() ) {
            result.setups.push_back( build_setup( profile.name, profile, nullptr ) );
        }
        for ( const block &own : profile.setups ) {
            result.setups.push_back( build_setup( profile.name + L": " + own.name, profile, &own ) );
        }
    }
    for ( const setup &each : result.setups ) {
        if ( each.items.empty() ) result.errors.push_back( L"[" + each.title + L"] has no windows" );
    }
    return result;
}

profile_file load_profiles() {
    wstring path = profiles_path();
    if ( !is_file( path ) ) {
        profile_file missing;
        missing.errors.push_back( L"No profiles file at " + path );
        return missing;
    }
    return read_profiles( read_text_file( path ) );
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

wstring describe( const setup &chosen ) {
    static const wchar_t *kind_names[] = { L"notes", L"PDF", L"tabs", L"pick from", L"File Explorer" };
    wstring text;
    for ( const layout_item &item : chosen.items ) {
        if ( !text.empty() ) text += L"\r\n";
        text += item.side + L": " + kind_names[ static_cast<int>( item.kind ) ];
        for ( const wstring &path : item.paths ) text += L" " + file_name( path );
    }
    return text;
}
