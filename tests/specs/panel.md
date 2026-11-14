# Spec: what the panel shows, and what a class click does

The tests go in `tests\test_panel.cpp`, in `void test_panel();`, made of `check()` calls from `tests\check.h`. They use only the functions below, with data built in the test, and touch no real desktops.

## Interface (`src\panel_model.h`)

```cpp
#include "timetable.h"   // class_meeting
#include "desktops.h"    // window_on_desktop { HWND window; int desktop_index; wstring program_path; }

struct today_row {
    class_meeting meeting;
    wstring kind_name;   // from kind_name() in subjects.h: "lecture", "tutorial", ...
    bool known;          // the course is one of known_courses
    bool marked;         // the class on now, or the next one today
    wstring note;        // "now", "in N min", or ""
};

struct today_section {
    vector<today_row> rows;
    wstring message;     // one line shown under the rows, or ""
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
    wstring desktop;        // the desktop to switch to
    bool create_desktop;    // it does not exist yet
    wstring setup_title;    // the setup to open there, or ""
};

class_click plan_class_click( const class_meeting &meeting, const vector<wstring> &desktop_names,
                              const vector<desktop_tile> &tiles, const vector<wstring> &setup_titles );
```

Courses are compared with `same_course` and desktop names come from `desktop_name_for` (both in `subjects.h`).

## Sample data

Monday Nov 16, with times as minutes after midnight:

| Course | Kind | Time | Room |
|---|---|---|---|
| MATH 117 | TUT | 510-620 (08:30-10:20) | DWE 1515 |
| GENE 119 | SEM | 630-680 | STC 0050 |
| MATH 117 | LEC | 750-800 | PSE 5353 |
| ECE 150 | LEC | 810-860 | PSE 5353 |
| MATH 115 | LEC | 870-920 | PSE 5353 |
| ECE 105 | LEC | 930-980 | PSE 5353 |

Known courses are the seven with folders: ECE 105, ECE 150, ECE 190, ECE 198, ENGL 192, MATH 115 and MATH 117 (as "Math 115" and "Math 117", to check that case is ignored).

## `build_today`

- The rows are the meetings in the order given, each with its kind name.
- `known` is false for GENE 119 and true for every other row.
- **Marking**:
  - At minute 790 (13:10), MATH 117 LEC (750-800) is on, so only that row is marked, with note "now".
  - At minute 805 (13:25), MATH 117 LEC has ended, so only the ECE 150 row (810) is marked, with note "in 5 min".
  - At 825 (13:45), only the ECE 150 row is marked, with note "now", because it runs until 860.
  - At 860 (14:20, the moment ECE 150 ends), only MATH 115 is marked, with note "in 10 min".
  - At 400 (06:40), MATH 117 TUT is marked with note "" (it starts in more than 60 minutes).
  - At 1320 (22:00), no row is marked.
- **Notes**: a row that is not marked has note "".
- **Unknown courses**: a GENE 119 row is still marked if it is the current or next class.
- **Message**:
  - With no meetings and no errors, the message is "No classes today".
  - With timetable errors, the message is "timetable.txt: " followed by the first error. This holds whether or not there are meetings, and wins over "No classes today".
  - Otherwise it is "".

## `build_tiles`

Desktop names: "Main", "Desktop 2", "150", "Bakfiets". The current index is 2.

- There are four tiles, in that order, with those names. Only the tile for "150" is current.
- Windows given as (desktop index, program):
  - (0, pdfcraft), (0, notepad++), (0, pdfcraft), (2, acrobat);
  - (5, other), whose index is out of range, so it is ignored;
  - then nine windows on desktop 3 with nine different programs.
- From those:
  - Tile 0 has `{pdfcraft, notepad++}`, without the repeat, in first-seen order.
  - Tile 1 has none, and tile 2 has `{acrobat}`.
  - Tile 3 has the first eight of its nine programs only.
- With a current index of -1, no tile is current.

## `plan_class_click`

Setup titles: "ECE 150: Lecture", "ECE 150: Exam", "Math 117: Lecture", "ENGL 192: Lecture". Desktop names: "Main", "150", "192".

- ECE 150 LEC, when tile "150" has no programs, gives desktop "150", create false and setup "ECE 150: Lecture".
- The same, when tile "150" has one or more programs, gives setup "" (it only switches).
- MATH 117 LEC, with no "117" desktop, gives desktop "117", create true and setup "Math 117: Lecture". A desktop that is being created always counts as empty.
- ECE 150 LAB, on an empty "150", gives setup "" (labs only switch).
- ENGL 192 LEC, on an empty "192", gives setup "ENGL 192: Lecture".
- Names are matched exactly, never partly: a class never chooses "Main", and "150" does not match a desktop named "1500".
