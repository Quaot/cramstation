# Spec: subjects

The tests go in `tests\test_subjects.cpp`, in `void test_subjects();`, made of `check()` calls from `tests\check.h`. Temporary folders go under `%TEMP%\crammy subjects checks`.

## Interface (`src\subjects.h`)

```cpp
wstring desktop_name_for( const wstring &course );
bool same_course( const wstring &a, const wstring &b );
bool has_course_folder( const wstring &course, const wstring &downloads_folder );
wstring setup_for_class( const wstring &course, const wstring &kind, const vector<wstring> &setup_titles );
wstring kind_name( const wstring &kind );
```

## Behaviour

**`desktop_name_for`** gives the digits of the course name, in order:

- "ECE 150" gives "150", "MATH 117" gives "117", "ENGL 192" gives "192" and "GENE 119" gives "119".
- "Math 115" gives "115".
- A course with no digits gives "".

**`same_course`** compares two course names, ignoring case and spaces:

- ("MATH 117", "Math 117") is true.
- ("ECE150", "ECE 150") is true.
- ("ECE 150", "ECE 105") is false.
- ("ECE 150", "ECE 1500") is false.

**`has_course_folder`** is true when `downloads_folder` has a subfolder whose name is the same course by `same_course`. With a test folder that holds subfolders `Math 117` and `ECE 150` and a file `GENE 119.txt`:

- "MATH 117" and "ECE 150" are true;
- "GENE 119" is false, since a file is not a folder;
- "ECE 190" is false.

**`setup_for_class`** looks through setup titles of the form `<Subject>: <Setup>`. The subject is matched to the course with `same_course`. The setup name is matched ignoring case.

- A kind of LEC wants a setup named "Lecture", and TUT wants "Tutorial". Any other kind wants none and gives "".
- The result is the matching title exactly as given, or "" when there is no match.

With the titles "ECE 150: Lecture", "ECE 150: Exam", "Math 117: Lecture", "Math 117: Tutorial" and "ECE 198":

- ECE 150 with LEC gives "ECE 150: Lecture".
- MATH 117 with TUT gives "Math 117: Tutorial".
- MATH 117 with LEC gives "Math 117: Lecture".
- ECE 150 with TUT gives "" (it has no tutorial setup).
- ECE 150 with LAB, TST or SEM gives "".
- ECE 198 with LAB gives "".
- MATH 115 with LEC gives "".

**`kind_name`** gives LEC "lecture", TUT "tutorial", LAB "lab", TST "test" and SEM "seminar". Any other kind comes back in lower case, so "PRJ" gives "prj".
