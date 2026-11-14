// Written from tests\specs\panel.md.

#include "../src/panel_model.h"
#include "check.h"

namespace {

class_meeting meeting( const wstring &course, const wstring &kind, int start_minute, int end_minute,
                       const wstring &room ) {
    class_meeting made;
    made.course = course;
    made.kind = kind;
    made.room = room;
    made.start_minute = start_minute;
    made.end_minute = end_minute;
    return made;
}

vector<class_meeting> monday() {
    return {
        meeting( L"MATH 117", L"TUT", 510, 620, L"DWE 1515" ),
        meeting( L"GENE 119", L"SEM", 630, 680, L"STC 0050" ),
        meeting( L"MATH 117", L"LEC", 750, 800, L"PSE 5353" ),
        meeting( L"ECE 150", L"LEC", 810, 860, L"PSE 5353" ),
        meeting( L"MATH 115", L"LEC", 870, 920, L"PSE 5353" ),
        meeting( L"ECE 105", L"LEC", 930, 980, L"PSE 5353" ),
    };
}

vector<wstring> known_courses() {
    return { L"ECE 105", L"ECE 150", L"ECE 190", L"ECE 198", L"ENGL 192", L"Math 115", L"Math 117" };
}

vector<int> marked_rows( const today_section &section ) {
    vector<int> marked;
    for ( size_t i = 0; i < section.rows.size(); i++ )
        if ( section.rows[i].marked ) marked.push_back( (int)i );
    return marked;
}

bool only_marked( const today_section &section, int row, const wstring &note ) {
    vector<int> marked = marked_rows( section );
    return marked.size() == 1 && marked[0] == row && section.rows[row].note == note;
}

bool no_notes_on_unmarked( const today_section &section ) {
    for ( const today_row &row : section.rows )
        if ( !row.marked && !row.note.empty() ) return false;
    return true;
}

window_on_desktop window_on( int desktop_index, const wstring &program ) {
    window_on_desktop made;
    made.window = nullptr;
    made.desktop_index = desktop_index;
    made.program_path = program;
    return made;
}

desktop_tile tile( const wstring &name, const vector<wstring> &programs ) {
    desktop_tile made;
    made.name = name;
    made.current = false;
    made.program_paths = programs;
    return made;
}

void check_today_rows() {
    today_section section = build_today( monday(), 790, known_courses(), {} );
    check( section.rows.size() == 6, "The today section has one row for each meeting." );
    bool in_order = section.rows.size() == 6;
    vector<class_meeting> given = monday();
    for ( size_t i = 0; in_order && i < given.size(); i++ )
        in_order = section.rows[i].meeting.course == given[i].course
                   && section.rows[i].meeting.kind == given[i].kind
                   && section.rows[i].meeting.start_minute == given[i].start_minute;
    check( in_order, "The rows keep the order of the meetings given." );
    if ( section.rows.size() == 6 ) {
        check( section.rows[0].kind_name == L"tutorial", "A TUT meeting has the kind name tutorial." );
        check( section.rows[1].kind_name == L"seminar", "A SEM meeting has the kind name seminar." );
        check( section.rows[2].kind_name == L"lecture", "A LEC meeting has the kind name lecture." );
        check( !section.rows[1].known, "GENE 119 is not a known course." );
        check( section.rows[0].known && section.rows[2].known && section.rows[3].known
                   && section.rows[4].known && section.rows[5].known,
               "Every course with a folder is known, whatever the case of its name." );
    }
}

void check_today_marking() {
    today_section inside = build_today( monday(), 790, known_courses(), {} );
    check( only_marked( inside, 2, L"now" ), "At 13:10 only the MATH 117 lecture row is marked, with the note now." );
    check( no_notes_on_unmarked( inside ), "At 13:10 the rows that are not marked have no note." );

    today_section before = build_today( monday(), 805, known_courses(), {} );
    check( only_marked( before, 3, L"in 5 min" ), "At 13:25 only the ECE 150 row is marked, with the note in 5 min." );
    check( no_notes_on_unmarked( before ), "At 13:25 the rows that are not marked have no note." );

    today_section during = build_today( monday(), 825, known_courses(), {} );
    check( only_marked( during, 3, L"now" ), "At 13:45 only the ECE 150 row is marked, with the note now." );
    check( no_notes_on_unmarked( during ), "At 13:45 the rows that are not marked have no note." );

    today_section at_end = build_today( monday(), 860, known_courses(), {} );
    check( only_marked( at_end, 4, L"in 10 min" ),
           "At the minute ECE 150 ends, only the MATH 115 row is marked, with the note in 10 min." );

    today_section early = build_today( monday(), 400, known_courses(), {} );
    check( only_marked( early, 0, L"" ), "At 06:40 the first class is marked with no note, as it is over an hour away." );

    today_section late = build_today( monday(), 1320, known_courses(), {} );
    check( marked_rows( late ).empty(), "At 22:00 no row is marked." );
    check( no_notes_on_unmarked( late ), "At 22:00 no row has a note." );
}

void check_today_unknown_course() {
    today_section next = build_today( monday(), 625, known_courses(), {} );
    check( only_marked( next, 1, L"in 5 min" ), "A GENE 119 row is marked when it is the next class." );
    check( next.rows.size() == 6 && !next.rows[1].known, "The marked GENE 119 row is still not known." );

    today_section now = build_today( monday(), 640, known_courses(), {} );
    check( only_marked( now, 1, L"now" ), "A GENE 119 row is marked when it is the class on now." );
}

void check_today_message() {
    check( build_today( monday(), 790, known_courses(), {} ).message.empty(),
           "With meetings and no errors the message is empty." );
    check( build_today( {}, 790, known_courses(), {} ).message == L"No classes today",
           "With no meetings and no errors the message is No classes today." );
    check( build_today( {}, 790, known_courses(), {} ).rows.empty(), "With no meetings there are no rows." );

    vector<wstring> errors = { L"line 3: no such day", L"line 9: bad time" };
    check( build_today( monday(), 790, known_courses(), errors ).message == L"timetable.txt: line 3: no such day",
           "With errors and meetings the message names the first error after timetable.txt: ." );
    check( build_today( {}, 790, known_courses(), errors ).message == L"timetable.txt: line 3: no such day",
           "With errors and no meetings the error wins over No classes today." );
    check( build_today( monday(), 790, known_courses(), errors ).rows.size() == 6,
           "Errors do not remove the rows." );
}

void check_tiles() {
    vector<wstring> names = { L"Main", L"Desktop 2", L"150", L"Bakfiets" };
    vector<window_on_desktop> windows = {
        window_on( 0, L"C:\\apps\\pdfcraft.exe" ),
        window_on( 0, L"C:\\apps\\notepad++.exe" ),
        window_on( 0, L"C:\\apps\\pdfcraft.exe" ),
        window_on( 2, L"C:\\apps\\acrobat.exe" ),
        window_on( 5, L"C:\\apps\\other.exe" ),
    };
    for ( int i = 1; i <= 9; i++ )
        windows.push_back( window_on( 3, L"C:\\apps\\program" + std::to_wstring( i ) + L".exe" ) );

    vector<desktop_tile> tiles = build_tiles( names, 2, windows );
    check( tiles.size() == 4, "There is one tile for each desktop name." );
    if ( tiles.size() != 4 ) return;

    check( tiles[0].name == L"Main" && tiles[1].name == L"Desktop 2" && tiles[2].name == L"150"
               && tiles[3].name == L"Bakfiets",
           "The tiles have the desktop names in order." );
    check( !tiles[0].current && !tiles[1].current && tiles[2].current && !tiles[3].current,
           "Only the tile for the current desktop is current." );

    vector<wstring> main_programs = { L"C:\\apps\\pdfcraft.exe", L"C:\\apps\\notepad++.exe" };
    check( tiles[0].program_paths == main_programs,
           "A tile lists each program once, in the order first seen." );
    check( tiles[1].program_paths.empty(), "A desktop with no windows has no programs." );
    check( tiles[2].program_paths == vector<wstring>{ L"C:\\apps\\acrobat.exe" },
           "The tile for the 150 desktop has its one program." );
    check( tiles[3].program_paths.size() == 8, "A tile keeps at most eight programs." );
    bool first_eight = tiles[3].program_paths.size() == 8;
    for ( int i = 0; first_eight && i < 8; i++ )
        first_eight = tiles[3].program_paths[i] == L"C:\\apps\\program" + std::to_wstring( i + 1 ) + L".exe";
    check( first_eight, "A tile with more than eight programs keeps the first eight." );

    bool out_of_range_ignored = true;
    for ( const desktop_tile &each : tiles )
        for ( const wstring &path : each.program_paths )
            if ( path == L"C:\\apps\\other.exe" ) out_of_range_ignored = false;
    check( out_of_range_ignored, "A window on a desktop index out of range is ignored." );

    vector<desktop_tile> none_current = build_tiles( names, -1, windows );
    bool any_current = false;
    for ( const desktop_tile &each : none_current ) any_current = any_current || each.current;
    check( none_current.size() == 4 && !any_current, "With a current index of -1 no tile is current." );
}

void check_class_clicks() {
    vector<wstring> setups = { L"ECE 150: Lecture", L"ECE 150: Exam", L"Math 117: Lecture", L"ENGL 192: Lecture" };
    vector<wstring> names = { L"Main", L"150", L"192" };
    vector<desktop_tile> empty_tiles = { tile( L"Main", {} ), tile( L"150", {} ), tile( L"192", {} ) };
    vector<desktop_tile> busy_tiles = { tile( L"Main", {} ), tile( L"150", { L"C:\\apps\\pdfcraft.exe" } ),
                                        tile( L"192", {} ) };
    vector<desktop_tile> very_busy_tiles = { tile( L"Main", {} ),
                                             tile( L"150", { L"C:\\apps\\pdfcraft.exe", L"C:\\apps\\acrobat.exe" } ),
                                             tile( L"192", {} ) };

    class_meeting ece_lecture = meeting( L"ECE 150", L"LEC", 810, 860, L"PSE 5353" );
    class_click empty_click = plan_class_click( ece_lecture, names, empty_tiles, setups );
    check( empty_click.desktop == L"150" && !empty_click.create_desktop && empty_click.setup_title == L"ECE 150: Lecture",
           "A lecture on an empty existing desktop switches there and opens its lecture setup." );

    class_click busy_click = plan_class_click( ece_lecture, names, busy_tiles, setups );
    check( busy_click.desktop == L"150" && !busy_click.create_desktop && busy_click.setup_title.empty(),
           "A lecture on a desktop with one program only switches." );
    class_click very_busy_click = plan_class_click( ece_lecture, names, very_busy_tiles, setups );
    check( very_busy_click.desktop == L"150" && very_busy_click.setup_title.empty(),
           "A lecture on a desktop with two programs only switches." );
    check( empty_click.desktop != L"Main", "A class never chooses the Main desktop." );

    class_click created = plan_class_click( meeting( L"MATH 117", L"LEC", 750, 800, L"PSE 5353" ), names, empty_tiles,
                                            setups );
    check( created.desktop == L"117" && created.create_desktop && created.setup_title == L"Math 117: Lecture",
           "A class with no desktop of its own asks for one to be created and opens its setup there." );

    class_click lab = plan_class_click( meeting( L"ECE 150", L"LAB", 900, 960, L"E7 1000" ), names, empty_tiles, setups );
    check( lab.desktop == L"150" && !lab.create_desktop && lab.setup_title.empty(),
           "A lab on an empty desktop only switches." );

    class_click english = plan_class_click( meeting( L"ENGL 192", L"LEC", 600, 650, L"HH 373" ), names, empty_tiles,
                                           setups );
    check( english.desktop == L"192" && !english.create_desktop && english.setup_title == L"ENGL 192: Lecture",
           "An ENGL 192 lecture on an empty 192 desktop opens the ENGL 192 lecture setup." );

    vector<wstring> near_names = { L"Main", L"1500" };
    vector<desktop_tile> near_tiles = { tile( L"Main", {} ), tile( L"1500", {} ) };
    class_click near_click = plan_class_click( ece_lecture, near_names, near_tiles, setups );
    check( near_click.desktop == L"150" && near_click.create_desktop,
           "The 150 desktop is not matched to a desktop named 1500, so a new one is created." );
}

}  // namespace

void test_panel() {
    check_today_rows();
    check_today_marking();
    check_today_unknown_course();
    check_today_message();
    check_tiles();
    check_class_clicks();
}
