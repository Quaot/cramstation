#include "timetable.h"

#include <algorithm>
#include <windows.h>

bool operator<( const calendar_date &a, const calendar_date &b ) {
    if ( a.year != b.year ) return a.year < b.year;
    if ( a.month != b.month ) return a.month < b.month;
    return a.day < b.day;
}

bool operator==( const calendar_date &a, const calendar_date &b ) {
    return a.year == b.year && a.month == b.month && a.day == b.day;
}

bool operator<=( const calendar_date &a, const calendar_date &b ) {
    return a < b || a == b;
}

// Days since 1970-01-01, by the usual civil calendar count
static long day_number( calendar_date date ) {
    long year = date.year - ( date.month <= 2 ? 1 : 0 );
    long era = ( year >= 0 ? year : year - 399 ) / 400;
    long year_of_era = year - era * 400;
    long month_shifted = date.month + ( date.month > 2 ? -3 : 9 );
    long day_of_year = ( 153 * month_shifted + 2 ) / 5 + date.day - 1;
    long day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    return era * 146097 + day_of_era - 719468;
}

int weekday_of( calendar_date date ) {
    long days = day_number( date ) + 3;   // 1970-01-01 was a Thursday, which is 3
    return (int)( ( days % 7 + 7 ) % 7 );
}

static bool read_number( const wstring &text, size_t digits, int &value ) {
    if ( text.size() != digits ) return false;
    value = 0;
    for ( wchar_t letter : text ) {
        if ( letter < L'0' || letter > L'9' ) return false;
        value = value * 10 + ( letter - L'0' );
    }
    return true;
}

static vector<wstring> split_on( const wstring &text, wchar_t separator ) {
    vector<wstring> parts;
    size_t start = 0;
    while ( true ) {
        size_t found = text.find( separator, start );
        if ( found == wstring::npos ) {
            parts.push_back( text.substr( start ) );
            return parts;
        }
        parts.push_back( text.substr( start, found - start ) );
        start = found + 1;
    }
}

static bool read_clock( const wstring &text, int &minute ) {
    if ( text.size() != 5 || text[2] != L':' ) return false;
    int hours, minutes;
    if ( !read_number( text.substr( 0, 2 ), 2, hours ) ) return false;
    if ( !read_number( text.substr( 3, 2 ), 2, minutes ) ) return false;
    if ( hours > 23 || minutes > 59 ) return false;
    minute = hours * 60 + minutes;
    return true;
}

static bool read_times( const wstring &text, int &start, int &end ) {
    vector<wstring> parts = split_on( text, L'-' );
    if ( parts.size() != 2 ) return false;
    if ( !read_clock( trim( parts[0] ), start ) ) return false;
    if ( !read_clock( trim( parts[1] ), end ) ) return false;
    return start < end;
}

static bool read_date( const wstring &text, calendar_date &date ) {
    vector<wstring> parts = split_on( text, L'-' );
    if ( parts.size() != 3 ) return false;
    if ( !read_number( parts[0], 4, date.year ) ) return false;
    if ( !read_number( parts[1], 2, date.month ) ) return false;
    if ( !read_number( parts[2], 2, date.day ) ) return false;
    return date.month >= 1 && date.month <= 12 && date.day >= 1 && date.day <= 31;
}

static bool read_dates( const wstring &text, calendar_date &first, calendar_date &last ) {
    size_t split = text.find( L" to " );
    if ( split == wstring::npos ) return false;
    if ( !read_date( trim( text.substr( 0, split ) ), first ) ) return false;
    if ( !read_date( trim( text.substr( split + 4 ) ), last ) ) return false;
    return first <= last;
}

// Weekdays named by the day letters, as bits with Monday as bit 0; 0 when a letter is wrong
static int weekday_bits( const wstring &days ) {
    int bits = 0;
    for ( size_t i = 0; i < days.size(); i++ ) {
        if ( days[i] == L'T' && i + 1 < days.size() && days[i + 1] == L'h' ) {
            bits |= 1 << 3;
            i++;
        } else if ( days[i] == L'M' ) bits |= 1 << 0;
        else if ( days[i] == L'T' ) bits |= 1 << 1;
        else if ( days[i] == L'W' ) bits |= 1 << 2;
        else if ( days[i] == L'F' ) bits |= 1 << 4;
        else return 0;
    }
    return bits;
}

static bool read_block( const wstring &line, class_block &block ) {
    vector<wstring> fields = split_on( line, L'|' );
    if ( fields.size() != 6 ) return false;
    for ( wstring &field : fields ) field = trim( field );
    block.course = fields[0];
    block.kind = fields[1];
    block.days = fields[2];
    block.room = fields[4];
    if ( weekday_bits( block.days ) == 0 ) return false;
    if ( !read_times( fields[3], block.start_minute, block.end_minute ) ) return false;
    return read_dates( fields[5], block.first, block.last );
}

vector<class_block> read_timetable( const wstring &text, vector<wstring> &errors ) {
    vector<class_block> blocks;
    int line_number = 0;
    size_t start = 0;
    while ( start <= text.size() ) {
        size_t end = text.find( L'\n', start );
        if ( end == wstring::npos ) end = text.size();
        wstring line = trim( text.substr( start, end - start ) );
        start = end + 1;
        line_number++;
        if ( line.empty() || line[0] == L'#' ) continue;
        class_block block;
        if ( read_block( line, block ) ) blocks.push_back( block );
        else errors.push_back( L"line " + std::to_wstring( line_number ) + L": cannot read \"" + line + L"\"" );
    }
    return blocks;
}

static long range_days( const class_block &block ) {
    return day_number( block.last ) - day_number( block.first );
}

static bool meets_on( const class_block &block, calendar_date date ) {
    if ( !( block.first <= date ) || !( date <= block.last ) ) return false;
    return ( weekday_bits( block.days ) >> weekday_of( date ) ) & 1;
}

static bool same_slot( const class_block &a, const class_block &b ) {
    return a.course == b.course && a.kind == b.kind && a.start_minute == b.start_minute;
}

vector<class_meeting> classes_on( const vector<class_block> &blocks, calendar_date date ) {
    vector<const class_block *> today;
    for ( const class_block &block : blocks ) {
        if ( !meets_on( block, date ) ) continue;
        bool found = false;
        for ( const class_block *&kept : today ) {
            if ( !same_slot( *kept, block ) ) continue;
            if ( range_days( block ) < range_days( *kept ) ) kept = &block;
            found = true;
            break;
        }
        if ( !found ) today.push_back( &block );
    }

    vector<class_meeting> meetings;
    for ( const class_block *block : today )
        meetings.push_back( { block->course, block->kind, block->room, block->start_minute, block->end_minute } );
    std::stable_sort( meetings.begin(), meetings.end(), []( const class_meeting &a, const class_meeting &b ) {
        if ( a.start_minute != b.start_minute ) return a.start_minute < b.start_minute;
        return a.course < b.course;
    } );
    return meetings;
}

wstring timetable_path() {
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW( nullptr, exe_path, MAX_PATH );
    return join_path( parent_folder( exe_path ), L"timetable.txt" );
}
