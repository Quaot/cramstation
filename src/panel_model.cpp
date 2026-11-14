#include "panel_model.h"
#include "subjects.h"

namespace {

const int max_tile_programs = 8;
const int note_window_minutes = 60;

bool is_known( const wstring &course, const vector<wstring> &known_courses ) {
    for ( const wstring &each : known_courses )
        if ( same_course( each, course ) ) return true;
    return false;
}

int marked_index( const vector<class_meeting> &meetings, int minute_now ) {
    int next = -1;
    for ( size_t i = 0; i < meetings.size(); i++ ) {
        const class_meeting &each = meetings[i];
        if ( each.start_minute <= minute_now && minute_now < each.end_minute ) return (int)i;
        if ( each.start_minute > minute_now
             && ( next < 0 || each.start_minute < meetings[next].start_minute ) )
            next = (int)i;
    }
    return next;
}

wstring note_for( const class_meeting &meeting, int minute_now ) {
    if ( meeting.start_minute <= minute_now ) return L"now";
    int minutes = meeting.start_minute - minute_now;
    if ( minutes <= note_window_minutes ) return L"in " + std::to_wstring( minutes ) + L" min";
    return L"";
}

bool has_name( const vector<wstring> &names, const wstring &name ) {
    for ( const wstring &each : names )
        if ( each == name ) return true;
    return false;
}

bool desktop_is_empty( const vector<desktop_tile> &tiles, const wstring &name ) {
    for ( const desktop_tile &each : tiles )
        if ( each.name == name ) return each.program_paths.empty();
    return true;
}

}  // namespace

today_section build_today( const vector<class_meeting> &meetings, int minute_now,
                           const vector<wstring> &known_courses, const vector<wstring> &timetable_errors ) {
    today_section section;
    int marked = marked_index( meetings, minute_now );
    for ( size_t i = 0; i < meetings.size(); i++ ) {
        today_row row;
        row.meeting = meetings[i];
        row.kind_name = kind_name( meetings[i].kind );
        row.known = is_known( meetings[i].course, known_courses );
        row.marked = (int)i == marked;
        if ( row.marked ) row.note = note_for( meetings[i], minute_now );
        section.rows.push_back( row );
    }
    if ( !timetable_errors.empty() )
        section.message = L"timetable.txt: " + timetable_errors[0];
    else if ( meetings.empty() )
        section.message = L"No classes today";
    return section;
}

vector<desktop_tile> build_tiles( const vector<wstring> &names, int current_index,
                                  const vector<window_on_desktop> &windows ) {
    vector<desktop_tile> tiles;
    for ( size_t i = 0; i < names.size(); i++ ) {
        desktop_tile tile;
        tile.name = names[i];
        tile.current = (int)i == current_index;
        tiles.push_back( tile );
    }
    for ( const window_on_desktop &each : windows ) {
        if ( each.desktop_index < 0 || each.desktop_index >= (int)tiles.size() ) continue;
        vector<wstring> &paths = tiles[each.desktop_index].program_paths;
        if ( (int)paths.size() >= max_tile_programs || has_name( paths, each.program_path ) ) continue;
        paths.push_back( each.program_path );
    }
    return tiles;
}

class_click plan_class_click( const class_meeting &meeting, const vector<wstring> &desktop_names,
                              const vector<desktop_tile> &tiles, const vector<wstring> &setup_titles ) {
    class_click click;
    click.desktop = desktop_name_for( meeting.course );
    click.create_desktop = !has_name( desktop_names, click.desktop );
    bool empty = click.create_desktop || desktop_is_empty( tiles, click.desktop );
    if ( empty ) click.setup_title = setup_for_class( meeting.course, meeting.kind, setup_titles );
    return click;
}
