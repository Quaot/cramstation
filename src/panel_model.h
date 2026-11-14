#pragma once

#include "desktops.h"
#include "timetable.h"

struct today_row {
    class_meeting meeting;
    wstring kind_name;
    bool known;     // the course has a folder in Downloads
    bool marked;    // the class on now, or the next one today
    wstring note;   // "now", "in N min", or ""
};

struct today_section {
    vector<today_row> rows;
    wstring message;
};

today_section build_today( const vector<class_meeting> &meetings, int minute_now,
                           const vector<wstring> &known_courses, const vector<wstring> &timetable_errors );

struct desktop_tile {
    wstring name;
    bool current;
    vector<wstring> program_paths;
};

vector<desktop_tile> build_tiles( const vector<wstring> &names, int current_index,
                                  const vector<window_on_desktop> &windows );

struct class_click {
    wstring desktop;
    bool create_desktop;
    wstring setup_title;
};

class_click plan_class_click( const class_meeting &meeting, const vector<wstring> &desktop_names,
                              const vector<desktop_tile> &tiles, const vector<wstring> &setup_titles );
