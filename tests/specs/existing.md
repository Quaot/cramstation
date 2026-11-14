# Spec: Crammy's existing behaviour

The tests for this spec go in `tests\test_existing.cpp`, in one function `void test_existing( HWND canvas );` that `test_main.cpp` calls with the hidden canvas it already makes. Each check is `check( condition, "plain sentence" )` from `tests\check.h`. Temporary files go under `%TEMP%\crammy existing checks`, which the tests create at the start and delete at the end.

The interfaces are in `src\profiles.h`, `src\files.h`, `src\launch.h`, `src\canvas.h` and `src\text.h`. Use only what those headers declare.

## 1. Reading profiles (`read_book`, `all_setups`)

Profile text A:

```
[ECE 150]
root = %TEMP%\crammy existing checks\ECE 150
desktop = 150

[ECE 150: Exam]
left = notes
right = pick "6 Exams" "2 Practice"

[ECE 150: Lecture]
full = newest "1 Lectures\PDF\ECE150-L*.pdf" with acrobat
```

- `read_book(A)` gives no errors and one subject named "ECE 150", with root `%TEMP%\crammy existing checks\ECE 150` as written, desktop "150" and two setups named "Exam" and "Lecture".
- Exam has two items. The first is side "left", kind notes, no paths. The second is side "right", kind pick, with paths "6 Exams" and "2 Practice" in that order, and app pdfcraft.
- Lecture has one item: side "full", kind newest, one path `1 Lectures\PDF\ECE150-L*.pdf`, app acrobat.
- `all_setups` gives two setups titled "ECE 150: Exam" and "ECE 150: Lecture", in that order. Both have desktop "150", and their root is the expanded folder (the `%TEMP%` value put in). The pick paths are joined to the root: `<root>\6 Exams` and `<root>\2 Practice`.
- Notes folder: when `<root>\2 Practice` exists as a folder, `notes_folder` is `<root>\2 Practice`; when it does not exist, `notes_folder` is the root. Check both by creating and removing that folder.

Profile text B, a subject with no setups:

```
[ECE 198]
root = C:\Courses\ECE 198
full = pick "."
```

- `all_setups` gives one setup titled "ECE 198", with `setup_index` -1, and its one item's path is `C:\Courses\ECE 198` (`"."` means the root itself).

Profile text C, lines shared by every setup:

```
[Math 117]
root = C:\Courses\Math 117
left = notes

[Math 117: Exam]
right = pdf "a.pdf"
```

- The setup "Math 117: Exam" has two items: the shared notes item first, then its own pdf item.

Unquoted paths and the setup's own settings:

- `left = folder 6 Exams` reads one path, "6 Exams".
- In a setup, `desktop = 117` overrides the subject's desktop for that setup only.

Errors: each bad line adds one entry to `errors` starting with `line N: `, where N counts from 1 and includes blank and comment lines. The line is skipped and reading continues. Check these, each in its own small text with the bad line on line 3:

- `left = magic "x"` (unknown window kind)
- `size = 5` (unknown setting)
- `[Broken` (heading without `]`)
- `left = pdf` (no path)
- `left = pdf "a.pdf" "b.pdf"` (too many paths)

Also, a window line before any heading, on line 1, gives an error starting `line 1: `.

## 2. Writing profiles (`write_book`)

- `write_book` output starts with the line `# Crammy profiles. The Crammy window writes this file, and you can also edit it by hand.`
- Reading text A, writing it, and reading the result again gives the same subjects, setups and items (compare names, sides, kinds, paths and apps).
- Writing that second book gives exactly the same text as the first write.
- A pdf item with app acrobat is written with ` with acrobat` at the end of its line.

## 3. Paths (`relative_to`, `find_setup`)

- `relative_to( L"C:\\A\\B", L"C:\\A\\B\\c\\d.pdf" )` is `c\d.pdf`.
- The comparison ignores case: `relative_to( L"C:\\A\\B", L"c:\\a\\b\\X.pdf" )` is `X.pdf`.
- A path outside the root comes back unchanged: `relative_to( L"C:\\A\\B", L"D:\\e.pdf" )` is `D:\e.pdf`.
- The root itself gives `.`.
- `find_setup` on `load`-style data (build a `profile_file` from `all_setups` of text A) finds "ece 150: exam" (any case) and the unique partial "Lecture". It returns null for "ECE 150", which matches both setups, and for "Biology".

## 4. Newest patterns and answer keys (`newest_pattern`, `is_answer_key`)

`newest_pattern` replaces runs of one or two digits with `*` and keeps longer runs:

| Input | Output |
|---|---|
| `ECE190-W10-Slides.pdf` | `ECE190-W*-Slides.pdf` |
| `PDF\MATH117-L23.pdf` | `PDF\MATH117-L*.pdf` |
| `Lec_23_M117.pdf` | `Lec_*_M117.pdf` |
| `MATH115-W07-Shell.pdf` | `MATH115-W*-Shell.pdf` |
| `Formula sheet.pdf` | (empty) |
| `ECE150-Mock-1112a.pdf` | (empty) |

`is_answer_key` is true for `ECE150-Mock-1112a-S.pdf`, `F25-M-S.pdf`, `F25-M-S (answer key, rebuilt).pdf`, `Week 3 solutions.pdf` and `Practice exam - ANSWER KEY.pdf`. It is false for `ECE190-W10-Slides.pdf`, `ECE190-W10-Sum.pdf`, `F25-M.pdf` and `ECE150-Mock-1112a.pdf`.

## 5. Finding PDFs (`find_pdfs`)

Make this tree under `<tmp>\tree`:

```
a.pdf
x.txt
sub\b.pdf
sub\deeper\c.pdf
build\skip1.pdf
Source\skip2.pdf
.hidden\skip3.pdf
```

Give `b.pdf` a later modified time than `a.pdf`, and `c.pdf` an earlier one than both (use `SetFileTime`).

- `find_pdfs( { tree }, false )` finds exactly `a.pdf`, `b.pdf` and `c.pdf`, sorted by name. The `build`, `Source` and dot folders are skipped, and `x.txt` is not a PDF.
- `find_pdfs( { tree }, true )` gives the same files, newest first: `b.pdf`, then `a.pdf`, then `c.pdf`.
- Each result's `folder` is the name of the folder it is in: "tree" for `a.pdf` and "sub" for `b.pdf`.
- A pattern only searches its own folder: `find_pdfs( { <tree>\*.pdf }, false )` finds only `a.pdf`.
- A single file path gives that file.
- The same folder given twice gives each file once.

## 6. Notes file names (`notes_file_name`)

Use a setup whose `notes_folder` is `<tmp>\notes`, and a notes item with no paths. "today" is the local date as `YYYY-MM-DD`.

- First PDF `C:\x\F26-M (blank, for redo).pdf` gives `<tmp>\notes\F26-M - my answers (<today>).txt`.
- First PDF `C:\x\F25-M (mock version).pdf` gives `<tmp>\notes\F25-M - my answers (<today>).txt`.
- With no first PDF (empty text), the name is `<tmp>\notes\Notes - my answers (<today>).txt`.
- When `<tmp>\notes\F26-M - my answers (2026-11-12).txt` already exists, that path is returned in place of a new one. When two such files exist, the one modified last is returned.
- A notes item with a path returns that path as it is.
- `notes_file_name` never creates a file: the folder holds the same files before and after.

## 7. Planning a setup (`plan_setup`)

Make PDFs `<tmp>\plan\one.pdf` and `two.pdf`, and a folder `<tmp>\plan\Labs`. Build `setup` values by hand: title "Test", `notes_folder` `<tmp>\plan`, and items with full paths. `picks` has one entry per item, and `separate` is false unless stated.

Each case gives the planned windows in item order, with side, program and paths:

- **Notes and a PDF.** Items left notes and right pdf `one.pdf` give two windows. The first is left, notepad_pp, with one path `<tmp>\plan\one - my answers (<today>).txt`. The second is right, pdfcraft, `{ one.pdf }`. No problems.
- **Tabs.** Right tabs `{ one.pdf, two.pdf }` gives one pdfcraft window with both paths.
- **Acrobat.** A pdf item with app acrobat gives one acrobat window. A tabs item of both PDFs with app acrobat gives two acrobat windows, one per PDF.
- **Picks.** A right pick item, with `picks[0] = { one.pdf, two.pdf }`, gives one pdfcraft window with both. With `separate` true it gives two windows. With `picks[0]` empty it gives no window for that item.
- **Folders.** A folder item for `Labs` gives an explorer window with that path.
- **Missing files.** A pdf item for `<tmp>\plan\missing.pdf` gives no window and a problem `PDF not found: <tmp>\plan\missing.pdf`. A folder item for a missing folder gives a problem starting `Folder not found: `.
- **Newest.** A newest item for `<tmp>\plan\*.pdf`, where `two.pdf` was modified last, gives one pdfcraft window with `two.pdf`.
  - If that file's modified date is not today, there is one problem that contains "may not be downloaded yet".
  - If it was modified today, there are no problems.
- **Nothing to open.** A setup with only a notes item and nothing else to open gives an empty plan and the problem `Nothing to open. Tick at least one PDF.`
- **The notes file is named after the first PDF opened.** That means in item order, including picked PDFs. Items left notes and right pick, with `picks[1] = { two.pdf }`, give a notes path named `two - my answers (<today>).txt`.

`plan_setup` never starts a program and never creates a file.

## 8. Dropping files on the screen picture

Use the hidden `canvas` passed to `test_existing`, which is 820 by 470 at 96 DPI scaled. Get the drop target with `canvas_drop_target()`. Simulate a drop the way File Explorer does it: build an `IDataObject` holding `CF_HDROP` file paths (`SHCreateDataObject`, then `SetData` with a `DROPFILES` block), then call `DragEnter`, `DragOver` and `Drop` at a screen point. Fractions below are of the canvas client area: 0.1 means 10 % across.

Zones:

- On an empty setup, the left third (x 0.1) gives side "left", the middle third (x 0.5) gives "full", and the right third (x 0.9) gives "right".
- Once the setup has a left or right window, the canvas splits into halves: x 0.4 gives "left" and x 0.6 gives "right".
- Dropping on a side while whole-screen windows exist moves those windows to the other side.

What a drop adds, with root `<tmp>\plan` passed to `show_items`:

- A folder adds a pick item, with its path stored relative to the root, as "Labs".
- One PDF adds a pdf item.
- Two PDFs dropped together add one tabs item with both paths.
- A `.txt` file adds a notes item with that path.
- A `.docx` adds nothing.
- With `show_items( nullptr, L"" )` (no setup chosen), a drop adds nothing and does not crash.
