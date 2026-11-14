#include "timetable.h"

// Placeholder until the timetable is built from tests\specs\timetable.md

bool operator<( const calendar_date &, const calendar_date & ) { return false; }
bool operator==( const calendar_date &, const calendar_date & ) { return false; }
bool operator<=( const calendar_date &, const calendar_date & ) { return false; }
int weekday_of( calendar_date ) { return -1; }
vector<class_block> read_timetable( const wstring &, vector<wstring> & ) { return {}; }
vector<class_meeting> classes_on( const vector<class_block> &, calendar_date ) { return {}; }
wstring timetable_path() { return L""; }
