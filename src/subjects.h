#pragma once

#include "text.h"

wstring desktop_name_for( const wstring &course );
bool same_course( const wstring &a, const wstring &b );
bool has_course_folder( const wstring &course, const wstring &downloads_folder );
wstring setup_for_class( const wstring &course, const wstring &kind, const vector<wstring> &setup_titles );
wstring kind_name( const wstring &kind );
