#pragma once

#include "text.h"

struct calendar_date {
    int year, month, day;
};

bool operator<( const calendar_date &a, const calendar_date &b );
bool operator==( const calendar_date &a, const calendar_date &b );
bool operator<=( const calendar_date &a, const calendar_date &b );

int weekday_of( calendar_date date );   // 0 is Monday, 6 is Sunday

struct class_block {
    wstring course, kind, days, room;   // "ECE 150", "LEC", "MWF", "PSE 5353"
    int start_minute, end_minute;       // minutes after midnight
    calendar_date first, last;
};

struct class_meeting {
    wstring course, kind, room;
    int start_minute, end_minute;
};

vector<class_block> read_timetable( const wstring &text, vector<wstring> &errors );
vector<class_meeting> classes_on( const vector<class_block> &blocks, calendar_date date );
wstring timetable_path();
