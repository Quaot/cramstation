#include "text.h"

#include <windows.h>

#include <cwctype>

wstring lower( wstring text ) {
    for ( wchar_t &c : text ) c = static_cast<wchar_t>( std::towlower( c ) );
    return text;
}

wstring trim( const wstring &text ) {
    size_t first = text.find_first_not_of( L" \t\r\n" );
    if ( first == wstring::npos ) return L"";
    size_t last = text.find_last_not_of( L" \t\r\n" );
    return text.substr( first, last - first + 1 );
}

bool contains_ci( const wstring &text, const wstring &part ) {
    return lower( text ).find( lower( part ) ) != wstring::npos;
}

bool starts_with( const wstring &text, const wstring &start ) {
    return text.compare( 0, start.size(), start ) == 0;
}

bool ends_with( const wstring &text, const wstring &end ) {
    return text.size() >= end.size() && text.compare( text.size() - end.size(), end.size(), end ) == 0;
}

wstring join_path( const wstring &folder, const wstring &name ) {
    if ( folder.empty() ) return name;
    if ( name.empty() || name == L"." ) return folder;
    return ( folder.back() == L'\\' ) ? folder + name : folder + L"\\" + name;
}

wstring file_name( const wstring &path ) {
    size_t slash = path.find_last_of( L"\\/" );
    return ( slash == wstring::npos ) ? path : path.substr( slash + 1 );
}

wstring parent_folder( const wstring &path ) {
    size_t slash = path.find_last_of( L"\\/" );
    return ( slash == wstring::npos ) ? L"" : path.substr( 0, slash );
}

wstring stem( const wstring &path ) {
    wstring name = file_name( path );
    size_t dot = name.find_last_of( L'.' );
    return ( dot == wstring::npos ) ? name : name.substr( 0, dot );
}

bool is_absolute( const wstring &path ) {
    return ( path.size() >= 2 && path[1] == L':' ) || starts_with( path, L"\\\\" );
}

bool is_file( const wstring &path ) {
    DWORD attributes = GetFileAttributesW( path.c_str() );
    return attributes != INVALID_FILE_ATTRIBUTES && !( attributes & FILE_ATTRIBUTE_DIRECTORY );
}

bool is_folder( const wstring &path ) {
    DWORD attributes = GetFileAttributesW( path.c_str() );
    return attributes != INVALID_FILE_ATTRIBUTES && ( attributes & FILE_ATTRIBUTE_DIRECTORY );
}

wstring expand_variables( const wstring &text ) {
    wchar_t buffer[4096];
    DWORD length = ExpandEnvironmentStringsW( text.c_str(), buffer, 4096 );
    return ( length > 0 && length <= 4096 ) ? wstring( buffer ) : text;
}

wstring user_folder() {
    return expand_variables( L"%USERPROFILE%" );
}

std::string to_utf8( const wstring &text ) {
    if ( text.empty() ) return {};
    int size = WideCharToMultiByte( CP_UTF8, 0, text.data(), static_cast<int>( text.size() ), nullptr, 0, nullptr, nullptr );
    std::string bytes( size, '\0' );
    WideCharToMultiByte( CP_UTF8, 0, text.data(), static_cast<int>( text.size() ), bytes.data(), size, nullptr, nullptr );
    return bytes;
}

wstring from_utf8( const std::string &bytes ) {
    if ( bytes.empty() ) return {};
    int size = MultiByteToWideChar( CP_UTF8, 0, bytes.data(), static_cast<int>( bytes.size() ), nullptr, 0 );
    wstring text( size, L'\0' );
    MultiByteToWideChar( CP_UTF8, 0, bytes.data(), static_cast<int>( bytes.size() ), text.data(), size );
    return text;
}

wstring read_text_file( const wstring &path ) {
    HANDLE file = CreateFileW( path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr );
    if ( file == INVALID_HANDLE_VALUE ) return L"";
    std::string bytes( GetFileSize( file, nullptr ), '\0' );
    DWORD bytes_read = 0;
    ReadFile( file, bytes.data(), static_cast<DWORD>( bytes.size() ), &bytes_read, nullptr );
    CloseHandle( file );
    bytes.resize( bytes_read );
    if ( bytes.rfind( "\xEF\xBB\xBF", 0 ) == 0 ) bytes.erase( 0, 3 );
    return from_utf8( bytes );
}

bool write_text_file( const wstring &path, const wstring &text ) {
    HANDLE file = CreateFileW( path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( file == INVALID_HANDLE_VALUE ) return false;
    std::string bytes = to_utf8( text );
    DWORD written = 0;
    bool ok = WriteFile( file, bytes.data(), static_cast<DWORD>( bytes.size() ), &written, nullptr ) && written == bytes.size();
    CloseHandle( file );
    return ok;
}

static wstring format_date( const SYSTEMTIME &time ) {
    wchar_t buffer[16];
    swprintf( buffer, 16, L"%04u-%02u-%02u", time.wYear, time.wMonth, time.wDay );
    return buffer;
}

wstring date_text( unsigned long long file_time ) {
    FILETIME utc{ static_cast<DWORD>( file_time ), static_cast<DWORD>( file_time >> 32 ) };
    FILETIME local;
    SYSTEMTIME time;
    FileTimeToLocalFileTime( &utc, &local );
    FileTimeToSystemTime( &local, &time );
    return format_date( time );
}

wstring today_text() {
    SYSTEMTIME time;
    GetLocalTime( &time );
    return format_date( time );
}
