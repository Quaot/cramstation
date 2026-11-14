# Working on Crammy

This guide is for anyone changing Crammy. Read it once before touching the code. The README covers using it; this covers how it works and how to change it without breaking it.

## The short version

Crammy is a Win32 program in C++17 with no libraries beyond Windows itself. It reads `profiles.txt`, shows a window where setups can be built by dragging files, and opens a setup by starting PdfCraft, Adobe Acrobat, Notepad++ or File Explorer and moving each window to its place on the screen.

Paths in this guide are relative to the repo folder unless they start with a drive letter. The owner's machine is set up like this:

| What | Where |
|---|---|
| Source | `C:\Users\Justin\Desktop\Projects\Crammy` (repo `github.com/Quaot/crammy`, branch `main`) |
| The program | `crammy.exe` in the repo folder, next to `profiles.txt`; built, not committed |
| Desktop shortcut | `C:\Users\Justin\Desktop\Crammy.lnk`, pointing at `crammy.exe` |
| Course files | `C:\Users\Justin\Downloads\<course>`, such as `Downloads\ECE 190`. File names follow `Downloads\Index.md` (`ECE190-W10-Slides.pdf`, `MATH117-L23.pdf`) |
| Compiler | MSYS2 UCRT64 g++ at `C:\msys64\ucrt64\bin` |

## Build and check

Run these from the repo folder, with `.\` in front. This machine sets `NoDefaultCurrentDirectoryInExePath`, so a bare `build.cmd` is not found.

```
.\build.cmd                 builds crammy.exe
.\tests\test.cmd            builds tests\crammy_test.exe and runs every check
.\crammy.exe --check        reads profiles.txt and reports problems
```

Run all three after every change. `build.cmd` uses `-Wall`, and the code builds with no warnings; keep it that way.

The checks put the screen picture in a hidden window and hand it fake file drops in code. They also start a separate, minimized copy of Notepad++ with scratch files in `%TEMP%` to test the notes cleanup, and close it afterwards. `tests\crammy_test.exe out.bmp` also draws a sample setup into `out.bmp`, which is the way to see a change to the look.

The checks use the owner's real setup, so on another machine some fail for reasons unrelated to a change:

- the profile checks read the real `profiles.txt` in the repo folder, one level above the test program;
- the drop checks use real files under `%USERPROFILE%\Downloads\ECE 190`, such as `1 Lectures\PDF\ECE190-W10-Slides.pdf`;
- the notes check needs Notepad++ at the path in `program_file`.

To add a check, write a `check_<thing>()` function in `tests\test_main.cpp` made of `check( condition, "plain sentence" )` calls, and call it from `wmain`. Plain helpers such as `newest_pattern` and `is_answer_key` are checked by calling them directly; anything that needs a window uses the hidden `canvas` made in `wmain`. Make sure a new check fails before the fix and passes after it.

Never test by taking over the mouse or keyboard, or by taking screenshots of the owner's screen. He has asked for that not to happen. If a change cannot be checked in a hidden window, add a way to check it in `tests\test_main.cpp`, as `close_old_notes` does by taking the window to work on as a parameter.

To try a setup for real, run it from a terminal: `crammy.exe --setup "Math 117: Lecture"`. Only do this when the owner has asked for something to be opened, since it switches desktops and opens windows.

## How a setup opens

This is the path through the code, in order:

1. `main.cpp` reads the command line. With `--setup` and either `--pick` or a setup that has nothing to pick, it opens the setup straight away; otherwise it shows the main window.
2. `profiles.cpp` reads `profiles.txt` into a `profile_book`, which keeps each subject with its setups and every window line as written. `all_setups` turns that into a list of `setup` values with every path joined to the subject's root.
3. If the setup has `pick` lines, `dialogs.cpp` shows a tick list and returns the PDFs chosen.
4. `launch.cpp` (`open_setup`) turns each line into a `planned_window`, which says what program to start on which side of the screen with what files. `newest` lines find their file here, and a warning is added when that file is not from today. A `notes` line gets its file name from `files.cpp` (`notes_file`), named after the first PDF of the setup.
5. `notes_tabs.cpp` (`close_old_notes`) saves and closes, in Notepad++, the notes files Crammy opened for earlier setups.
6. `desktops.cpp` (`switch_to_desktop`) moves to the setup's virtual desktop if it names one.
7. `tiling.cpp` (`open_windows`) starts every program, finds each new window, moves it to its place, and snaps the windows that have a side to themselves.

Problems at any step are collected as plain sentences and shown at the end; nothing stops halfway with an error.

## Source files

| File | Job |
|---|---|
| `src/main.cpp` | Command line, then the main window or a direct open |
| `src/main_window.cpp` | The main window: setup list, New setup / Rename / Delete, desktop choice, Add notes, Edit as text, Open |
| `src/canvas.cpp` | The screen picture: drawing tiles, drag and drop from File Explorer, dragging tiles, the right-click menu |
| `src/dialogs.cpp` | The New setup / Rename dialog and the tick list shown before opening; the tick list is the only place answer keys are hidden |
| `src/editor_window.cpp` | Edit as text: a plain editor for `profiles.txt` |
| `src/profiles.cpp` | Reading and writing `profiles.txt`; joining paths to the root |
| `src/files.cpp` | Finding PDFs, spotting answer keys, the `newest` pattern, naming notes files |
| `src/launch.cpp` | Turning a setup into windows to open |
| `src/tiling.cpp` | Starting programs, finding their windows, moving and snapping them; small helpers shared with other files |
| `src/notes_tabs.cpp` | Closing old notes tabs in Notepad++ |
| `src/desktops.cpp` | Reading virtual desktop names and switching between them |
| `src/ui.cpp` | Fonts, DPI scaling, window creation, the message loop |
| `src/text.cpp` | String, path and file helpers |
| `tests/test_main.cpp` | The checks |
| `windhawk/crammy-button.wh.cpp` | The Windhawk mod that puts the Crammy button on the taskbar; `windhawk/README.md` says how to add it |
| `art/make_icon.py` | Draws `crammy.ico` with Pillow (`python art\make_icon.py`) |

Answer keys are hidden from the tick list until its "Show answer keys" box is ticked. `is_answer_key` in `files.cpp` decides what counts as a key, with one case-insensitive pattern: the name contains "answer key", the word "solution" or "solutions", `-M-S (` or `-F-S`, or its stem ends in `-S` (the renamed course files, such as `ECE150-Mock-1112a-S.pdf`). `--pick` and `--list` do not use it.

## The profile file

`profiles.txt` sits next to `crammy.exe`. The main window rewrites the whole file on every change, using `write_book` in `profiles.cpp`. Only the header comment at the top survives a rewrite; comments written anywhere else are lost. The tests check that reading and writing the file loses nothing else.

```
[ECE 190]
root = Downloads\ECE 190
desktop = 190

[ECE 190: Lecture]
left = newest "1 Lectures\PDF\ECE190-W*-Slides.pdf"
right = newest "1 Lectures\PDF\ECE190-W*-Sum.pdf" with acrobat
```

- `[Subject]` starts a subject; `[Subject: Setup]` adds a setup to it. A subject with no setups opens as it is and is listed under its own name.
- Subject settings: `root` (relative to the user folder), `desktop` (a virtual desktop name), `notes folder` (relative to the root; the default is `2 Practice` when it exists, else the root). A setup may set its own `notes folder` or `desktop`.
- Window lines are `side = kind "path" "path" ...`. `side` is `left`, `right` or `full`. Lines under a subject heading are shared by all its setups.
- Kinds: `notes` (Notepad++; no path means a file named after the first PDF), `pdf` (one file), `newest` (one pattern, newest file by modified date), `tabs` (several PDFs in one PdfCraft window), `pick` (folders or files to tick from), `folder` (File Explorer).
- A PDF line may end with `with acrobat`. Acrobat gets one window per PDF.
- Paths are stored relative to the root when they are inside it. `*` works in file names. `%USERPROFILE%` and other variables are expanded.

To add a setting or a kind, change `profiles.h` (the structs), `read_book` and `write_item` or `write_settings` (reading and writing), the `header` comment, `check_count` if it takes paths, and the README table. Then add a check that the new line survives a round trip.

## How windows are found and placed

`tiling.cpp` starts each program from the user folder, never from Crammy's folder; a program that inherits Crammy's folder as its working folder stops that folder from being renamed.

| Program | Started with | Its window is found by |
|---|---|---|
| PdfCraft | `pdfcraft.exe --new-window "a.pdf" "b.pdf"` (several files give tabs in one window) | the process ID of the program started |
| Acrobat | `Acrobat.exe /n "file.pdf"` (`/n` forces a new window) | a title containing the file name and "Adobe Acrobat"; `/n` hands off to another process, so the ID cannot be used |
| Notepad++ | `notepad++.exe "file.txt"` (opens a tab in the running copy) | a title containing the file name and "Notepad++" |
| File Explorer | `explorer.exe "folder"` | window class `CabinetWClass` and the folder name in the title |

The program paths are in `program_file` in `tiling.cpp`. Each side of the work area is split into equal rows for the windows on it. Windows 11 draws invisible borders around windows, so sizes use the visible frame from `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)`.

Programs often restore their own saved size a moment after appearing, so `open_windows` keeps moving each window back until all of them stay put for four checks in a row (15 seconds at most).

A window with a side to itself is then snapped by pressing Win+Left or Win+Right, which makes Windows treat it as snapped. Before pressing keys, `bring_to_front` presses F24 (a key no keyboard has) because Windows only lets the program that sent the last key press bring a window forward. A window that does not end up in place is moved there directly. Two or more windows on one side are only moved, never snapped: snapping to quarters was tried and did not work reliably with PdfCraft.

`crammy.manifest` makes the program aware of per-monitor DPI, so all coordinates are real pixels. Do not remove it.

## Virtual desktops

Windows keeps the desktops in the registry under `HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\VirtualDesktops`: `VirtualDesktopIDs` lists them in order, `CurrentVirtualDesktop` is the one showing, and each name is under `Desktops\{id}\Name`. An unnamed desktop is called "Desktop 2" and so on. `switch_to_desktop` presses Ctrl+Win+Left or Right as many times as needed and waits until the registry shows the new desktop. Moving another program's windows between desktops needs undocumented calls that change between Windows updates, which is why Crammy switches first and opens afterwards.

## Notes tabs in Notepad++

Notes are plain `.txt` files opened in Notepad++. Crammy writes each one it opens to `notes opened.txt` (next to the exe, not committed). Before the next setup opens, `close_old_notes` sends messages straight to the Notepad++ window; these are the numbers from Notepad++'s `Notepad_plus_msgs.h`, where `NPPMSG` is `WM_USER + 1000`:

| Message | Use |
|---|---|
| `NPPMSG + 7`, lParam 1 or 2 | number of tabs in the main or second view |
| `NPPMSG + 28`, wParam view, lParam index | switch to a tab |
| `NPPMSG + 38` | save the tab showing |
| `NPPMSG + 48`, lParam 41003 | menu command File > Close |

The title of the Notepad++ window shows the full path of the tab showing, with `*` in front while it has unsaved changes. A tab is closed only after saving cleared the `*`; otherwise it stays open and stays on the list. Tabs not on the list are never touched, and the tab that was showing before is shown again at the end. None of this brings Notepad++ forward or presses keys.

Windows 11 Notepad has no way for another program to save or close a tab, which is why notes moved to Notepad++.

## Common changes

**Add a program**, such as VS Code: add it to `enum class program` in `tiling.h`, give its path in `program_file`, add how it is started in `open_windows` and how its window is found in `find_window`, add its name to `program_names` in `canvas.cpp`, and decide in `launch.cpp` and `profiles.cpp` how a profile line asks for it (a new kind, or a `with` word like `with acrobat`).

**Add a kind of window line**: see the profile section above, then handle it in `open_setup` (`launch.cpp`) and in `describe` (`canvas.cpp`) so its tile says what it opens. If it can be dropped onto the screen picture, handle it in `add_dropped`.

**Change the look of the screen picture**: `paint` and `describe` in `canvas.cpp`. Run `tests\crammy_test.exe out.bmp` and look at the picture. The owner dislikes styling that looks machine-made, such as pastel cards or paragraphs of instructions on screen. Plain tiles with the real program icons are the current style.

**Add a command line option**: the loop in `wWinMain` in `main.cpp`, and the list at the top of that file and in the README.

**Add a subject or setup**: use the main window, or edit `profiles.txt` and run `crammy.exe --check`.

## Rules for every change

- Variables, fields and parameters are `snake_case`. Function and type names follow the existing code.
- Keep the code plain and readable, in small functions. Comments are few, and each one says why rather than what.
- Commit as Justin Gu `<enderjustin@icloud.com>` with small, focused commits straight to `main`. No `Co-Authored-By` lines, no mention of AI anywhere, and no `CLAUDE.md` or `.claude` folder in the repo.
- Commit messages and documentation use plain words and short sentences. Avoid figures of speech, including saying a file "lives" somewhere; say where it is. Avoid lists of exactly three items in prose; use two, or split the sentence.
- Run `git pull --rebase --autostash` before pushing, and read the push output; a rejected push can look like success if the output is hidden.
- `crammy.exe`, `tests\crammy_test.exe`, `last setup.txt` and `notes opened.txt` are not committed.
- When a change makes this guide or the README wrong, fix them in the same commit. Remove a Known limits entry as soon as the limit is gone.

## Known limits

- Program paths are fixed in `program_file`. If PdfCraft, Acrobat or Notepad++ moves, Crammy will report that no window appeared.
- If Notepad++ is open on another desktop, a new notes tab opens in that window, and moving it brings it to the current desktop.
- `newest` uses the modified date, so re-saving an old PDF makes it the newest.
- Only one monitor is handled: everything uses the primary monitor's work area.
