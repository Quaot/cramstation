# Spec: the timetable

The tests go in `tests\test_timetable.cpp`, in one function `void test_timetable();`, made of `check( condition, "plain sentence" )` calls from `tests\check.h`. They read the repo's real `timetable.txt`, one folder above the test program: the folder holding `tests\crammy_test.exe`, then `..\timetable.txt`.

## Interface (`src\timetable.h`)

```cpp
struct calendar_date {
    int year, month, day;
};
bool operator<( const calendar_date &a, const calendar_date &b );
bool operator==( const calendar_date &a, const calendar_date &b );
bool operator<=( const calendar_date &a, const calendar_date &b );

int weekday_of( calendar_date date );   // 0 is Monday, 6 is Sunday

struct class_block {
    wstring course, kind, days, room;    // "ECE 150", "LEC", "MWF", "PSE 5353"
    int start_minute, end_minute;        // minutes after midnight: 13:30 is 810
    calendar_date first, last;
};

struct class_meeting {
    wstring course, kind, room;
    int start_minute, end_minute;
};

vector<class_block> read_timetable( const wstring &text, vector<wstring> &errors );
vector<class_meeting> classes_on( const vector<class_block> &blocks, calendar_date date );
wstring timetable_path();   // timetable.txt next to the running program
```

`wstring` and `vector` come from `src\text.h`.

## The file format

One class block per line, with six fields separated by `|`. Spaces around each field are ignored.

```
ECE 150 | LEC | MWF | 13:30-14:20 | PSE 5353 | 2026-10-28 to 2026-12-08
```

- **Course and kind** are kept as written, trimmed.
- **Days** use M, T, W, Th and F. "MTTh" means Monday, Tuesday and Thursday; "TTh" means Tuesday and Thursday; "MWF" means Monday, Wednesday and Friday.
- **Times** are 24-hour `HH:MM-HH:MM`. The start must be before the end, and both must be valid times.
- **Room** is kept as written ("TBA" is a room like any other).
- **Dates** are `YYYY-MM-DD to YYYY-MM-DD`, with the first date not after the last.
- **Blank lines** and lines starting with `#` are ignored.
- **A bad line** (wrong field count, bad days, time or dates) is skipped and adds one entry to `errors`, starting `line N: ` where N counts every line from 1. Reading continues after it.

## Which classes are on a date

- A block meets on a date when the date's weekday is one of its days and `first <= date <= last`.
- When two blocks of the same course and kind both meet that date and start at the same minute, only the one whose date range covers fewer days is kept.
- Blocks at different start times are separate meetings, even for the same course and kind.
- Meetings are sorted by start minute, then by course.

## Cases

Synthetic checks:

- `weekday_of({2026, 11, 16})` is 0 (Monday). `weekday_of({2026, 11, 14})` is 5 (Saturday). `weekday_of({2026, 12, 8})` is 1 (Tuesday).
- Dates compare in calendar order: `{2026, 9, 30} < {2026, 10, 1}` and `{2026, 10, 1} <= {2026, 10, 1}`.
- `MATH 117 | LEC | MTTh | 15:30-16:20 | PSE 5353 | 2026-09-09 to 2026-12-08` reads as days covering Monday, Tuesday and Thursday. On Thu Nov 19 it meets once, and on Wed Nov 18 it does not meet.
- Text with three lines, where line 3 has the time `25:00-26:00`, gives one error starting `line 3: `, and the other two lines are read.
- Also an error each, on line 1 of their own text:
  - a line with five fields;
  - the days `MX`;
  - dates `2026-12-08 to 2026-09-09`;
  - times `14:00-13:00`.
- A comment line and a blank line give no blocks and no errors.
- **Overlap:** block A is `X 100 | LEC | W | 09:00-10:00 | R1 | 2026-09-09 to 2026-12-08`, and block B is the same course, kind, day and start, room R2, `2026-11-04 to 2026-11-04`. On Wed Nov 4 there is one meeting, in R2. On Wed Nov 11 there is one meeting, in R1.

Checks with the real `timetable.txt`:

- It reads with no errors.
- **Mon Nov 16** has exactly these 6 meetings, in this order (course, kind, start-end, room):
  1. MATH 117, TUT, 08:30-10:20, DWE 1515
  2. GENE 119, SEM, 10:30-11:20, STC 0050
  3. MATH 117, LEC, 12:30-13:20, PSE 5353
  4. ECE 150, LEC, 13:30-14:20, PSE 5353
  5. MATH 115, LEC, 14:30-15:20, PSE 5353
  6. ECE 105, LEC, 15:30-16:20, PSE 5353
- **Wed Nov 4** includes:
  - ECE 150 LAB 08:30-09:20 in PSE 5353;
  - ECE 150 LAB 10:30-12:20 in E2 1792;
  - MATH 117 LEC 09:30-10:20 in PSE 5353.

  It also has ECE 190 LEC 16:30-17:20.
- **Wed Nov 11** has ECE 150 LAB 08:30-10:20 in PSE 5353 and ECE 150 LAB 10:30-12:20 in E2 1792.
- **Wed Nov 18** has exactly one ECE 150 LAB: 08:30-09:20 in PSE 5353.
- **Mon Oct 26** has exactly one meeting: MATH 115, TST, 20:00-21:50, TBA.
- **Tue Oct 27** has ECE 105 TST 16:30-18:20 and ENGL 192 LEC 13:00-14:20 in DWE 1515. It has no ECE 190 LEC and no MATH 115 TUT.
- **Fri Nov 20** has ECE 150 LEC 13:30-14:20 in PSE 5353 and also ECE 150 LEC 15:30-16:20 in PSE 5343. Its first meeting is ECE 105 TUT at 08:30.
- **Sat Nov 14**, **Sun Nov 15** and **Wed Dec 9** have no meetings.
- `timetable_path()` ends with `\timetable.txt`.
