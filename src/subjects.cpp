#include "subjects.h"

#include <filesystem>

namespace {

wstring without_spaces( const wstring &text ) {
	wstring result;
	for( wchar_t letter : text )
		if( letter != L' ' )
			result += letter;
	return result;
}

wstring setup_name_for( const wstring &kind ) {
	if( kind == L"LEC" ) return L"lecture";
	if( kind == L"TUT" ) return L"tutorial";
	return L"";
}

}

wstring desktop_name_for( const wstring &course ) {
	wstring digits;
	for( wchar_t letter : course )
		if( letter >= L'0' && letter <= L'9' )
			digits += letter;
	return digits;
}

bool same_course( const wstring &a, const wstring &b ) {
	return lower( without_spaces( a ) ) == lower( without_spaces( b ) );
}

bool has_course_folder( const wstring &course, const wstring &downloads_folder ) {
	std::error_code error;
	for( const auto &entry : std::filesystem::directory_iterator( downloads_folder, error ) ) {
		std::error_code entry_error;
		if( entry.is_directory( entry_error ) && same_course( course, entry.path().filename().wstring() ) )
			return true;
	}
	return false;
}

wstring setup_for_class( const wstring &course, const wstring &kind, const vector<wstring> &setup_titles ) {
	wstring wanted = setup_name_for( kind );
	if( wanted.empty() )
		return L"";

	for( const wstring &title : setup_titles ) {
		size_t colon = title.find( L':' );
		if( colon == wstring::npos )
			continue;
		if( same_course( course, trim( title.substr( 0, colon ) ) ) && lower( trim( title.substr( colon + 1 ) ) ) == wanted )
			return title;
	}
	return L"";
}

wstring kind_name( const wstring &kind ) {
	if( kind == L"LEC" ) return L"lecture";
	if( kind == L"TUT" ) return L"tutorial";
	if( kind == L"LAB" ) return L"lab";
	if( kind == L"TST" ) return L"test";
	if( kind == L"SEM" ) return L"seminar";
	return lower( kind );
}
