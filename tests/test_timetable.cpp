// Written from tests\specs\timetable.md.

#include <windows.h>

#include "check.h"
#include "../src/timetable.h"

namespace {

wstring find_timetable_file() {
    wchar_t buffer[MAX_PATH * 4] = {};
    GetModuleFileNameW( nullptr, buffer, MAX_PATH * 4 );
    wstring folder = parent_folder( buffer );
    wstring beside = join_path( folder, L"timetable.txt" );
    if ( is_file( beside ) ) {
        return beside;
    }
    return join_path( join_path( folder, L".." ), L"timetable.txt" );
}

vector<class_block> read_text( const wstring &text, vector<wstring> &errors ) {
    errors.clear();
    return read_timetable( text, errors );
}

// True when the one-line text gives no block and one error that starts with "line 1: ".
bool one_bad_line( const wstring &line ) {
    vector<wstring> errors;
    vector<class_block> blocks = read_text( line, errors );
    return blocks.empty() && errors.size() == 1 && starts_with( errors[0], L"line 1: " );
}

int count_meetings( const vector<class_meeting> &meetings, const wstring &course, const wstring &kind ) {
    int count = 0;
    for ( const class_meeting &meeting : meetings ) {
        if ( meeting.course == course && meeting.kind == kind ) {
            count++;
        }
    }
    return count;
}

bool meeting_is( const class_meeting &meeting, const wstring &course, const wstring &kind,
                 int start_minute, int end_minute, const wstring &room ) {
    return meeting.course == course && meeting.kind == kind && meeting.start_minute == start_minute
           && meeting.end_minute == end_minute && meeting.room == room;
}

bool has_meeting( const vector<class_meeting> &meetings, const wstring &course, const wstring &kind,
                  int start_minute, int end_minute, const wstring &room ) {
    for ( const class_meeting &meeting : meetings ) {
        if ( meeting_is( meeting, course, kind, start_minute, end_minute, room ) ) {
            return true;
        }
    }
    return false;
}

int minutes( int hour, int minute ) {
    return hour * 60 + minute;
}

int meeting_count( const vector<class_block> &blocks, calendar_date date ) {
    return (int)classes_on( blocks, date ).size();
}

void check_dates() {
    check( weekday_of( { 2026, 11, 16 } ) == 0, "Monday 16 November 2026 is weekday 0" );
    check( weekday_of( { 2026, 11, 14 } ) == 5, "Saturday 14 November 2026 is weekday 5" );
    check( weekday_of( { 2026, 12, 8 } ) == 1, "Tuesday 8 December 2026 is weekday 1" );
    check( calendar_date { 2026, 9, 30 } < calendar_date { 2026, 10, 1 },
           "30 September comes before 1 October" );
    check( !( calendar_date { 2026, 10, 1 } < calendar_date { 2026, 9, 30 } ),
           "1 October does not come before 30 September" );
    check( calendar_date { 2026, 10, 1 } <= calendar_date { 2026, 10, 1 },
           "a date is not after itself" );
    check( calendar_date { 2026, 10, 1 } == calendar_date { 2026, 10, 1 }, "equal dates compare equal" );
    check( !( calendar_date { 2026, 10, 1 } == calendar_date { 2026, 10, 2 } ),
           "different dates do not compare equal" );
    check( calendar_date { 2025, 12, 31 } < calendar_date { 2026, 1, 1 },
           "the year counts before the month" );
}

void check_reading() {
    vector<wstring> errors;
    vector<class_block> blocks = read_text(
        L"MATH 117 | LEC | MTTh | 15:30-16:20 | PSE 5353 | 2026-09-09 to 2026-12-08", errors );
    check( errors.empty() && blocks.size() == 1, "one good line gives one block and no errors" );
    if ( blocks.size() == 1 ) {
        check( blocks[0].course == L"MATH 117" && blocks[0].kind == L"LEC" && blocks[0].room == L"PSE 5353",
               "the course, kind and room are kept as written" );
        check( blocks[0].start_minute == minutes( 15, 30 ) && blocks[0].end_minute == minutes( 16, 20 ),
               "the times are minutes after midnight" );
        check( blocks[0].first == calendar_date { 2026, 9, 9 } && blocks[0].last == calendar_date { 2026, 12, 8 },
               "the first and last dates are read" );
        check( meeting_count( blocks, { 2026, 11, 16 } ) == 1 && meeting_count( blocks, { 2026, 11, 17 } ) == 1,
               "MTTh meets on Monday and Tuesday" );
        check( meeting_count( blocks, { 2026, 11, 19 } ) == 1, "MTTh meets on Thursday" );
        check( meeting_count( blocks, { 2026, 11, 18 } ) == 0, "MTTh does not meet on Wednesday" );
        check( meeting_count( blocks, { 2026, 11, 20 } ) == 0, "MTTh does not meet on Friday" );
        check( meeting_count( blocks, { 2026, 9, 8 } ) == 0, "a block does not meet before its first date" );
        check( meeting_count( blocks, { 2026, 12, 8 } ) == 1, "a block meets on its last date" );
        check( meeting_count( blocks, { 2026, 12, 10 } ) == 0, "a block does not meet after its last date" );
        vector<class_meeting> meetings = classes_on( blocks, { 2026, 11, 19 } );
        check( meetings.size() == 1
                   && meeting_is( meetings[0], L"MATH 117", L"LEC", minutes( 15, 30 ), minutes( 16, 20 ), L"PSE 5353" ),
               "a meeting carries the course, kind, times and room of its block" );
    }

    blocks = read_text( L"  ECE 150   |  LEC |  MWF |  13:30-14:20  |  TBA  | 2026-10-28 to 2026-12-08  ", errors );
    check( errors.empty() && blocks.size() == 1, "spaces around the fields are ignored" );
    if ( blocks.size() == 1 ) {
        check( blocks[0].course == L"ECE 150" && blocks[0].kind == L"LEC" && blocks[0].room == L"TBA",
               "fields are trimmed and TBA is a room like any other" );
        check( meeting_count( blocks, { 2026, 11, 16 } ) == 1 && meeting_count( blocks, { 2026, 11, 18 } ) == 1
                   && meeting_count( blocks, { 2026, 11, 20 } ) == 1,
               "MWF meets on Monday, Wednesday and Friday" );
        check( meeting_count( blocks, { 2026, 11, 17 } ) == 0 && meeting_count( blocks, { 2026, 11, 19 } ) == 0,
               "MWF does not meet on Tuesday or Thursday" );
    }

    blocks = read_text( L"A 1 | LEC | TTh | 09:00-10:00 | R1 | 2026-09-01 to 2026-12-31", errors );
    check( errors.empty() && blocks.size() == 1, "the days TTh read without errors" );
    if ( blocks.size() == 1 ) {
        check( meeting_count( blocks, { 2026, 11, 17 } ) == 1 && meeting_count( blocks, { 2026, 11, 19 } ) == 1,
               "TTh meets on Tuesday and Thursday" );
        check( meeting_count( blocks, { 2026, 11, 16 } ) == 0, "TTh does not meet on Monday" );
    }
}

void check_bad_lines() {
    vector<wstring> errors;
    vector<class_block> blocks = read_text(
        L"A 1 | LEC | M | 09:00-10:00 | R1 | 2026-09-01 to 2026-12-31\n"
        L"B 2 | LEC | T | 09:00-10:00 | R1 | 2026-09-01 to 2026-12-31\n"
        L"C 3 | LEC | W | 25:00-26:00 | R1 | 2026-09-01 to 2026-12-31",
        errors );
    check( blocks.size() == 2, "a bad third line leaves the other two lines read" );
    check( errors.size() == 1 && starts_with( errors[0], L"line 3: " ), "the bad line is reported as line 3" );

    blocks = read_text(
        L"A 1 | LEC | M | 09:00-10:00 | R1 | 2026-09-01 to 2026-12-31\n"
        L"bad line\n"
        L"B 2 | LEC | T | 09:00-10:00 | R1 | 2026-09-01 to 2026-12-31",
        errors );
    check( blocks.size() == 2 && errors.size() == 1, "reading continues after a bad line" );

    blocks = read_text(
        L"# a comment\n"
        L"\n"
        L"A 1 | LEC | M | 09:00-10:00 | R1 | 2026-09-01 to 2026-12-31\n"
        L"A 1 | LEC | M | 09:00 | R1 | 2026-09-01 to 2026-12-31",
        errors );
    check( blocks.size() == 1 && errors.size() == 1 && starts_with( errors[0], L"line 4: " ),
           "line numbers count comment and blank lines" );

    check( one_bad_line( L"A 1 | LEC | M | 09:00-10:00 | 2026-09-01 to 2026-12-31" ),
           "a line with five fields is an error on line 1" );
    check( one_bad_line( L"A 1 | LEC | MX | 09:00-10:00 | R1 | 2026-09-01 to 2026-12-31" ),
           "the days MX are an error on line 1" );
    check( one_bad_line( L"A 1 | LEC | M | 09:00-10:00 | R1 | 2026-12-08 to 2026-09-09" ),
           "dates with the first after the last are an error on line 1" );
    check( one_bad_line( L"A 1 | LEC | M | 14:00-13:00 | R1 | 2026-09-01 to 2026-12-31" ),
           "a start time after the end time is an error on line 1" );
    check( one_bad_line( L"A 1 | LEC | M | 09:00-09:00 | R1 | 2026-09-01 to 2026-12-31" ),
           "a start time equal to the end time is an error on line 1" );
    check( one_bad_line( L"A 1 | LEC | M | 09:60-10:00 | R1 | 2026-09-01 to 2026-12-31" ),
           "the minute 60 is an error on line 1" );
    check( one_bad_line( L"A 1 | LEC | M | 25:00-26:00 | R1 | 2026-09-01 to 2026-12-31" ),
           "the hour 25 is an error on line 1" );

    blocks = read_text( L"# only a comment", errors );
    check( blocks.empty() && errors.empty(), "a comment line gives no blocks and no errors" );
    blocks = read_text( L"   \n\n", errors );
    check( blocks.empty() && errors.empty(), "blank lines give no blocks and no errors" );
}

void check_overlap() {
    vector<wstring> errors;
    vector<class_block> blocks = read_text(
        L"X 100 | LEC | W | 09:00-10:00 | R1 | 2026-09-09 to 2026-12-08\n"
        L"X 100 | LEC | W | 09:00-10:00 | R2 | 2026-11-04 to 2026-11-04",
        errors );
    check( errors.empty() && blocks.size() == 2, "the two overlapping blocks are read" );

    vector<class_meeting> meetings = classes_on( blocks, { 2026, 11, 4 } );
    check( meetings.size() == 1 && meetings[0].room == L"R2",
           "on the shared day there is one meeting, in the room of the block covering fewer days" );
    meetings = classes_on( blocks, { 2026, 11, 11 } );
    check( meetings.size() == 1 && meetings[0].room == L"R1",
           "outside the short block there is one meeting, in room R1" );

    blocks = read_text(
        L"X 100 | LEC | W | 09:00-10:00 | R1 | 2026-09-09 to 2026-12-08\n"
        L"X 100 | LEC | W | 11:00-12:00 | R2 | 2026-11-04 to 2026-11-04",
        errors );
    check( meeting_count( blocks, { 2026, 11, 4 } ) == 2,
           "blocks of the same course and kind at different starts are separate meetings" );
}

void check_order() {
    vector<wstring> errors;
    vector<class_block> blocks = read_text(
        L"B 2 | LEC | W | 13:00-14:00 | R1 | 2026-09-09 to 2026-12-08\n"
        L"C 3 | LEC | W | 09:00-10:00 | R1 | 2026-09-09 to 2026-12-08\n"
        L"A 1 | LEC | W | 09:00-10:00 | R1 | 2026-09-09 to 2026-12-08\n"
        L"B 2 | LEC | W | 09:00-10:00 | R1 | 2026-09-09 to 2026-12-08",
        errors );
    vector<class_meeting> meetings = classes_on( blocks, { 2026, 11, 4 } );
    check( meetings.size() == 4, "four blocks on one day give four meetings" );
    if ( meetings.size() == 4 ) {
        check( meetings[0].course == L"A 1" && meetings[1].course == L"B 2" && meetings[2].course == L"C 3",
               "meetings at the same start are sorted by course" );
        check( meetings[3].course == L"B 2" && meetings[3].start_minute == minutes( 13, 0 ),
               "a later start comes after the earlier ones" );
    }
}

void check_real_file() {
    wstring text = read_text_file( find_timetable_file() );
    check( !text.empty(), "timetable.txt is found one folder above the test program and is not empty" );

    vector<wstring> errors;
    vector<class_block> blocks = read_text( text, errors );
    check( errors.empty(), "timetable.txt reads with no errors" );
    check( !blocks.empty(), "timetable.txt gives some blocks" );

    vector<class_meeting> meetings = classes_on( blocks, { 2026, 11, 16 } );
    check( meetings.size() == 6, "Monday 16 November has exactly six meetings" );
    if ( meetings.size() == 6 ) {
        check( meeting_is( meetings[0], L"MATH 117", L"TUT", minutes( 8, 30 ), minutes( 10, 20 ), L"DWE 1515" ),
               "Monday 16 November starts with MATH 117 TUT at 08:30" );
        check( meeting_is( meetings[1], L"GENE 119", L"SEM", minutes( 10, 30 ), minutes( 11, 20 ), L"STC 0050" ),
               "Monday 16 November second is GENE 119 SEM at 10:30" );
        check( meeting_is( meetings[2], L"MATH 117", L"LEC", minutes( 12, 30 ), minutes( 13, 20 ), L"PSE 5353" ),
               "Monday 16 November third is MATH 117 LEC at 12:30" );
        check( meeting_is( meetings[3], L"ECE 150", L"LEC", minutes( 13, 30 ), minutes( 14, 20 ), L"PSE 5353" ),
               "Monday 16 November fourth is ECE 150 LEC at 13:30" );
        check( meeting_is( meetings[4], L"MATH 115", L"LEC", minutes( 14, 30 ), minutes( 15, 20 ), L"PSE 5353" ),
               "Monday 16 November fifth is MATH 115 LEC at 14:30" );
        check( meeting_is( meetings[5], L"ECE 105", L"LEC", minutes( 15, 30 ), minutes( 16, 20 ), L"PSE 5353" ),
               "Monday 16 November last is ECE 105 LEC at 15:30" );
    }

    meetings = classes_on( blocks, { 2026, 11, 4 } );
    check( has_meeting( meetings, L"ECE 150", L"LAB", minutes( 8, 30 ), minutes( 9, 20 ), L"PSE 5353" ),
           "Wednesday 4 November has the ECE 150 LAB at 08:30 in PSE 5353" );
    check( has_meeting( meetings, L"ECE 150", L"LAB", minutes( 10, 30 ), minutes( 12, 20 ), L"E2 1792" ),
           "Wednesday 4 November has the ECE 150 LAB at 10:30 in E2 1792" );
    check( has_meeting( meetings, L"MATH 117", L"LEC", minutes( 9, 30 ), minutes( 10, 20 ), L"PSE 5353" ),
           "Wednesday 4 November has the MATH 117 LEC at 09:30" );
    check( has_meeting( meetings, L"ECE 190", L"LEC", minutes( 16, 30 ), minutes( 17, 20 ), L"PSE 5353" ),
           "Wednesday 4 November has the ECE 190 LEC at 16:30" );

    meetings = classes_on( blocks, { 2026, 11, 11 } );
    check( has_meeting( meetings, L"ECE 150", L"LAB", minutes( 8, 30 ), minutes( 10, 20 ), L"PSE 5353" ),
           "Wednesday 11 November has the ECE 150 LAB at 08:30 to 10:20 in PSE 5353" );
    check( has_meeting( meetings, L"ECE 150", L"LAB", minutes( 10, 30 ), minutes( 12, 20 ), L"E2 1792" ),
           "Wednesday 11 November has the ECE 150 LAB at 10:30 in E2 1792" );

    meetings = classes_on( blocks, { 2026, 11, 18 } );
    check( count_meetings( meetings, L"ECE 150", L"LAB" ) == 1, "Wednesday 18 November has exactly one ECE 150 LAB" );
    check( has_meeting( meetings, L"ECE 150", L"LAB", minutes( 8, 30 ), minutes( 9, 20 ), L"PSE 5353" ),
           "Wednesday 18 November has that LAB at 08:30 to 09:20 in PSE 5353" );

    meetings = classes_on( blocks, { 2026, 10, 26 } );
    check( meetings.size() == 1
               && meeting_is( meetings[0], L"MATH 115", L"TST", minutes( 20, 0 ), minutes( 21, 50 ), L"TBA" ),
           "Monday 26 October has exactly one meeting, the MATH 115 TST at 20:00 in TBA" );

    meetings = classes_on( blocks, { 2026, 10, 27 } );
    check( has_meeting( meetings, L"ECE 105", L"TST", minutes( 16, 30 ), minutes( 18, 20 ), L"TBA" ),
           "Tuesday 27 October has the ECE 105 TST at 16:30" );
    check( has_meeting( meetings, L"ENGL 192", L"LEC", minutes( 13, 0 ), minutes( 14, 20 ), L"DWE 1515" ),
           "Tuesday 27 October has the ENGL 192 LEC at 13:00" );
    check( count_meetings( meetings, L"ECE 190", L"LEC" ) == 0, "Tuesday 27 October has no ECE 190 LEC" );
    check( count_meetings( meetings, L"MATH 115", L"TUT" ) == 0, "Tuesday 27 October has no MATH 115 TUT" );

    meetings = classes_on( blocks, { 2026, 11, 20 } );
    check( has_meeting( meetings, L"ECE 150", L"LEC", minutes( 13, 30 ), minutes( 14, 20 ), L"PSE 5353" ),
           "Friday 20 November has the ECE 150 LEC at 13:30 in PSE 5353" );
    check( has_meeting( meetings, L"ECE 150", L"LEC", minutes( 15, 30 ), minutes( 16, 20 ), L"PSE 5343" ),
           "Friday 20 November has the ECE 150 LEC at 15:30 in PSE 5343" );
    check( !meetings.empty() && meetings[0].course == L"ECE 105" && meetings[0].kind == L"TUT"
               && meetings[0].start_minute == minutes( 8, 30 ),
           "Friday 20 November starts with the ECE 105 TUT at 08:30" );

    check( meeting_count( blocks, { 2026, 11, 14 } ) == 0, "Saturday 14 November has no meetings" );
    check( meeting_count( blocks, { 2026, 11, 15 } ) == 0, "Sunday 15 November has no meetings" );
    check( meeting_count( blocks, { 2026, 12, 9 } ) == 0, "Wednesday 9 December has no meetings" );
}

void check_path() {
    check( ends_with( timetable_path(), L"\\timetable.txt" ), "the timetable path ends with \\timetable.txt" );
}

}  // namespace

void test_timetable() {
    check_dates();
    check_reading();
    check_bad_lines();
    check_overlap();
    check_order();
    check_real_file();
    check_path();
}
