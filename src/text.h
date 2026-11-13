#pragma once

#include <string>
#include <vector>

using std::wstring;
using std::vector;

wstring lower( wstring text );
wstring trim( const wstring &text );
bool contains_ci( const wstring &text, const wstring &part );
bool starts_with( const wstring &text, const wstring &start );
bool ends_with( const wstring &text, const wstring &end );

wstring join_path( const wstring &folder, const wstring &name );
wstring file_name( const wstring &path );
wstring parent_folder( const wstring &path );
wstring stem( const wstring &path );
bool is_absolute( const wstring &path );
bool is_file( const wstring &path );
bool is_folder( const wstring &path );
wstring expand_variables( const wstring &text );
wstring user_folder();

std::string to_utf8( const wstring &text );
wstring from_utf8( const std::string &text );
wstring read_text_file( const wstring &path );
bool write_text_file( const wstring &path, const wstring &text );

wstring date_text( unsigned long long file_time );
wstring today_text();
