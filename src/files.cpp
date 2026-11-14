#include "files.h"

#include <windows.h>

#include <algorithm>
#include <regex>

namespace {

unsigned long long to_number( const FILETIME &time ) {
    return ( static_cast<unsigned long long>( time.dwHighDateTime ) << 32 ) | time.dwLowDateTime;
}

bool skip_folder( const wstring &name ) {
    wstring folder = lower( name );
    return folder == L"build" || folder == L"source" || folder == L"__pycache__" || name[0] == L'.';
}

// pattern is a folder\name that may hold * or ?
void search( const wstring &pattern, bool go_down, vector<pdf_file> &found ) {
    WIN32_FIND_DATAW data;
    HANDLE handle = FindFirstFileExW( pattern.c_str(), FindExInfoBasic, &data, FindExSearchNameMatch,
                                      nullptr, FIND_FIRST_EX_LARGE_FETCH );
    if ( handle == INVALID_HANDLE_VALUE ) return;
    wstring folder = parent_folder( pattern );
    do {
        wstring name = data.cFileName;
        if ( name == L"." || name == L".." ) continue;
        wstring path = join_path( folder, name );
        if ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
            if ( go_down && !skip_folder( name ) ) search( join_path( path, L"*" ), true, found );
        } else if ( ends_with( lower( name ), L".pdf" ) ) {
            found.push_back( { path, name, file_name( folder ), to_number( data.ftLastWriteTime ) } );
        }
    } while ( FindNextFileW( handle, &data ) );
    FindClose( handle );
}

}  // namespace

vector<pdf_file> find_pdfs( const vector<wstring> &paths, bool newest_first ) {
    vector<pdf_file> found;
    for ( const wstring &path : paths ) {
        if ( is_folder( path ) ) search( join_path( path, L"*" ), true, found );
        else search( path, false, found );
    }
    std::sort( found.begin(), found.end(), [newest_first]( const pdf_file &a, const pdf_file &b ) {
        return newest_first ? a.modified > b.modified : lower( a.name ) < lower( b.name );
    } );
    found.erase( std::unique( found.begin(), found.end(),
                              []( const pdf_file &a, const pdf_file &b ) { return lower( a.path ) == lower( b.path ); } ),
                 found.end() );
    return found;
}

// The renamed course files mark a key with -S at the end of the stem, as in "F25-M-S.pdf".
bool is_answer_key( const wstring &name ) {
    static const std::wregex key_names( L"answer key|-M-S \\(|-F-S\\b|solutions?\\b|-S(\\.[^.]*)?$",
                                        std::regex_constants::icase );
    return std::regex_search( name, key_names );
}

// One or two digits are a week or lecture number; longer runs such as course codes stay as they are.
wstring newest_pattern( const wstring &path ) {
    static const std::wregex short_number( L"(^|\\D)\\d{1,2}(?=\\D|$)" );
    wstring name = file_name( path );
    wstring pattern = std::regex_replace( name, short_number, L"$1*" );
    if ( pattern == name ) return L"";
    wstring folder = parent_folder( path );
    return folder.empty() ? pattern : join_path( folder, pattern );
}

static wstring newest_matching( const wstring &pattern ) {
    WIN32_FIND_DATAW data;
    HANDLE handle = FindFirstFileW( pattern.c_str(), &data );
    if ( handle == INVALID_HANDLE_VALUE ) return L"";
    wstring newest;
    unsigned long long newest_time = 0;
    do {
        if ( !( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && to_number( data.ftLastWriteTime ) >= newest_time ) {
            newest = data.cFileName;
            newest_time = to_number( data.ftLastWriteTime );
        }
    } while ( FindNextFileW( handle, &data ) );
    FindClose( handle );
    return newest.empty() ? L"" : join_path( parent_folder( pattern ), newest );
}

wstring notes_file( const setup &chosen, const layout_item &notes_item, const wstring &first_pdf ) {
    wstring path;
    if ( !notes_item.paths.empty() ) {
        path = notes_item.paths[0];
    } else {
        static const std::wregex paper_tags( L"\\s*\\((blank|mock version)[^)]*\\)", std::regex_constants::icase );
        wstring base = first_pdf.empty() ? L"Notes" : std::regex_replace( stem( first_pdf ), paper_tags, L"" );
        path = newest_matching( join_path( chosen.notes_folder, base + L" - my answers*.txt" ) );
        if ( path.empty() ) path = join_path( chosen.notes_folder, base + L" - my answers (" + today_text() + L").txt" );
    }
    if ( !is_file( path ) ) {
        write_text_file( path, stem( path ) + L"\r\nStart time:\r\nFinish time:\r\n\r\n" );
    }
    return path;
}
