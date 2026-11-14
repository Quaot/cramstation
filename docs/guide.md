# Working on Crammy

This guide is for anyone changing Crammy. Read it once before touching the code. The README covers using it; this covers how it works and how to change it without breaking it.

## The short version

Crammy is a Win32 program in C++17 with no libraries beyond Windows itself. It reads `profiles.txt`, shows a window where setups can be built by dragging files, and opens a setup by starting PdfCraft, Adobe Acrobat, Notepad++ or File Explorer and moving each window to its place on the screen.

It can also run in the background with a tray icon and a keyboard shortcut. From there it shows a panel with today's classes from `timetable.txt`, a tile for each virtual desktop, and two buttons that start Youtonomous.

Paths in this guide are relative to the repo folder unless they start with a drive letter. The owner's machine is set up like this:

| What | Where |
|---|---|
| Source | `C:\Users\Justin\Desktop\Projects\Crammy` (repo `github.com/Quaot/crammy`, branch `main`) |
| The program | `crammy.exe` in the repo folder, next to `profiles.txt`; built, not committed |
| Desktop shortcut | `C:\Users\Justin\Desktop\Crammy.lnk`, pointing at `crammy.exe` |
| `settings.txt` | Next to `crammy.exe`; not committed. The panel shortcut, the Youtonomous program and videos, and whether to start at sign-in. Every key has a default, so the file may be missing |
| `timetable.txt` | Next to `crammy.exe`; committed. One line for each class block, copied from Quest |
| Startup shortcut | `Crammy.lnk` in the user's Startup folder, running `crammy.exe --background`; made and removed by the tray menu |
| Windhawk mod | `windhawk\crammy-button.wh.cpp`, added to Windhawk by hand; it puts the Crammy button on the taskbar |
| Course files | `C:\Users\Justin\Downloads\<course>`, such as `Downloads\ECE 190`. File names follow `Downloads\Index.md` (`ECE190-W10-Slides.pdf`, `MATH117-L23.pdf`) |
| Compiler | MSYS2 UCRT64 g++ at `C:\msys64\ucrt64\bin` |

## Build and check

Run these from the repo folder, with `.\` in front. This machine sets `NoDefaultCurrentDirectoryInExePath`, so a bare `build.cmd` is not found.

```
.\build.cmd                 builds crammy.exe
.\tests\test.cmd            builds tests\crammy_test.exe and runs every check
.\crammy.exe --check        reads profiles.txt and reports problems
```

Run all three after every change. `build.cmd` uses `-Wall`, and the code builds with no warnings; keep it that way. A new source file goes into the file lists of both `build.cmd` and `tests\test.cmd`.

The Testing section below says how the checks are written and what they need on this machine.

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
| `src/desktops.cpp` | Reading virtual desktop names, switching between them, finding which desktop each window is on, and creating a desktop |
| `src/timetable.cpp` | Reading `timetable.txt` and working out which classes meet on a date, with the date and weekday helpers |
| `src/settings.cpp` | Reading and writing `settings.txt` and the shortcut text, and building the command that starts Youtonomous |
| `src/subjects.cpp` | Matching a class to a desktop name and to a setup, plus two small helpers: the name of a class kind, and the check for a course folder in Downloads |
| `src/panel_model.cpp` | What the panel shows and does, as plain data with no window: today's rows and the desktop tiles, and the plan for a class click (the types are in `panel_model.h`) |
| `src/panel.cpp` | The panel window; `toggle_panel` in `panel.h` shows it or hides it |
| `src/background.cpp` | Background mode: the hidden `CrammyBackground` window, the tray icon, the shortcut, the one-copy lock and the Startup shortcut |
| `src/ui.cpp` | Fonts, DPI scaling, window creation, the message loop |
| `src/text.cpp` | String, path and file helpers |
| `tests/test_main.cpp` | The older checks, and the calls to every `test_<area>()`; writes a picture when asked |
| `tests/check.cpp`, `tests/check.h` | `check()`, which prints one line and counts a failure |
| `tests/test_<area>.cpp` | The checks for one area, written from `tests/specs/<area>.md`: `existing`, `timetable`, `settings`, `subjects`, `panel`, `background` and `desktops` |
| `tests/specs/*.md` | One spec for each area |
| `tests/test.cmd` | Builds the checks and runs them |
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

## Desktops

**Reading and switching.** Windows keeps the desktops in the registry under `HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\VirtualDesktops`: `VirtualDesktopIDs` lists them in order, `CurrentVirtualDesktop` is the one showing, and each name is under `Desktops\{id}\Name`. An unnamed desktop is called "Desktop 2" and so on. `desktop_names` and `current_desktop_index` read this. `switch_to_desktop` presses Ctrl+Win+Left or Right as many times as needed and waits until the registry shows the new desktop. Moving another program's windows between desktops needs undocumented calls that change between Windows updates, which is why Crammy switches first and opens afterwards.

**Windows on each desktop.** `windows_by_desktop` lists the top-level windows that are visible, have a title and have no owner. For each one it asks the documented `IVirtualDesktopManager::GetWindowDesktopId` and matches the answer to the ids in the registry. Windows whose desktop cannot be found are left out. Explorer does not track some windows on any desktop, such as tool windows (`WS_EX_TOOLWINDOW`) and windows that cannot be activated (`WS_EX_NOACTIVATE`), so these never appear. A test that needs a window on a desktop must make an ordinary application window.

**Creating a desktop.** There is no documented call for this. `create_desktop` reaches Explorer's `IVirtualDesktopManagerInternal` through the immersive shell service, using the interface for Windows 11 24H2 and later, which is build 26200 on this machine. The interface is called by slot number, so a wrong order would call the wrong method. The order was taken from MScholtes' `VirtualDesktop11-24H2.cs`, version 1.21 (github.com/MScholtes/VirtualDesktop), and the comment above `desktop_manager_internal` in `desktops.cpp` lists it. After a Windows update, check that list against the new build before trusting the calls. Crammy calls `CreateDesktop` and then `SetDesktopName`.

If the interface cannot be reached, or `CreateDesktop` fails, Crammy presses Ctrl+Win+D instead. That makes a desktop and switches to it, but leaves it unnamed. If the desktop was made and only the name failed, nothing more is pressed. In both cases `create_desktop` returns true and sets `problem` to a sentence saying the desktop needs its name in Task View, and the panel shows it.

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

## Background mode and the panel

`crammy.exe` takes these options for background mode:

| Option | What it does |
|---|---|
| `--background` | Starts with no window: a tray icon, the keyboard shortcut and a hidden window. Exits at once when a background Crammy is already running |
| `--background --show` | The same, and shows the panel as soon as it has started. When a background Crammy is already running, asks it to show the panel and exits |
| `--panel` | Asks the running background Crammy to show or hide its panel, then exits. When none is running, starts one with `--background --show` and exits |

`--panel` starts the new copy itself with `CreateProcessW` and `DETACHED_PROCESS`, from the folder of `crammy.exe`. The copy then keeps running when whatever ran `--panel` ends, such as a taskbar button. If the start fails, it is tried once more with `CREATE_BREAKAWAY_FROM_JOB` added. An earlier version started it through `explorer.exe`, which can drop the arguments.

**The window and the message.** `run_background` in `background.cpp` makes a hidden top-level window of the class `CrammyBackground`. Anything that wants the panel finds that window and posts it the registered message `CrammyShowPanel` (`show_panel_message()`, from `RegisterWindowMessageW`). The window answers by calling `toggle_panel`. `--panel` and the Windhawk mod both work this way, (`--panel` through `ask_running_crammy_to_toggle_panel`). Keep the class name and the message name as they are: the mod has them written in.

**One copy only.** `run_background` first takes the named mutex `CrammyBackgroundLock` and holds it until the process ends. A second copy finds the mutex taken and exits. The mutex covers two launches at the same moment, before either has made its window; `background_is_running` only looks for the window.

**The shortcut.** It is registered with `RegisterHotKey` from the `shortcut` setting, and the default is Ctrl+Alt+C. The setting is read once, when the background copy starts, so a change needs a restart of it. If another program already owns the keys, registering fails without any message and the tray icon still works.

**The tray icon.** A left click shows or hides the panel. A right click opens a menu with Setups, Edit timetable (opens `timetable.txt` in Notepad++), Start at sign-in (ticked when the Startup shortcut exists; choosing it flips `start at sign in` in `settings.txt` and makes or removes the shortcut) and Quit. The icon is added again when Explorer restarts.

**Start at sign-in.** `set_start_at_sign_in` makes or removes `Crammy.lnk` in the user's Startup folder. The background Crammy calls it when it starts, so the shortcut matches the `start at sign in` setting. The shortcut runs `crammy.exe --background` from the folder of the running program. The folder comes from `FOLDERID_Startup`, unless the environment variable `CRAMMY_STARTUP_FOLDER` is set; then that folder is used. The tests set it to a folder in `%TEMP%` so that they never touch the real Startup folder. Do not set it for any other reason.

**The panel.** It is a borderless window that opens above the bottom-left corner of the taskbar and closes when it loses focus or Esc is pressed. It shows the date and time, today's classes, a tile for every virtual desktop, and the buttons for the two Pomodoro videos and for Setups. `docs/panel-design.md` is the approved design, with the look it asks for. What the panel shows and what a click does are worked out in `panel_model.cpp`, without any window, so that they can be checked in tests. `panel.cpp` only draws that data and passes clicks to it.

Clicking a class goes to the desktop named after the course number (`desktop_name_for`; `150` for ECE 150). If that desktop does not exist, it is created first. If nothing is open on it, the setup `<Subject>: Lecture` or `<Subject>: Tutorial` is opened there when it exists. Other kinds of class only switch. `plan_class_click` makes this plan.

The Pomodoro buttons start Youtonomous through `youtonomous_command`, with the long or the short video from the settings. Nothing else starts Youtonomous.

**The Windhawk mod.** `windhawk\crammy-button.wh.cpp` hides the Widgets button on the taskbar and puts a Crammy button in its place. A click posts `CrammyShowPanel`, or runs `crammy.exe --panel` when the background window does not exist. It does nothing else, so that as little as possible runs inside Explorer. `windhawk\README.md` says how to add it and switch it off. The owner adds it himself.

## Timetable

`timetable.txt` is read by `read_timetable` in `timetable.cpp`. It has one line for each class block:

```
ECE 150 | LEC | MWF | 13:30-14:20 | PSE 5353 | 2026-10-28 to 2026-12-08
```

The six fields are separated by `|`, and spaces around them are ignored.

| Field | Meaning |
|---|---|
| Course | Kept as written, such as `ECE 150` |
| Kind | Quest's code: `LEC`, `TUT`, `LAB`, `TST` or `SEM`. `kind_name` in `subjects.cpp` turns these into words, and any other code is shown in lower case |
| Days | Quest's letters: `M`, `T`, `W`, `Th` and `F`. `MTTh` is Monday, Tuesday and Thursday, and `TTh` is Tuesday and Thursday |
| Times | 24-hour `HH:MM-HH:MM`; the start must be before the end |
| Room | Kept as written; `TBA` is a room like any other |
| Dates | `YYYY-MM-DD to YYYY-MM-DD`, the first not after the last |

Blank lines and lines starting with `#` are ignored. A line that cannot be read is skipped, and an error `line N: cannot read "..."` is kept. The panel shows the first error in one line, so a typo does not hide the rest of the day.

**Which classes meet on a date.** `classes_on` takes a block when the weekday of the date is one of its days and the date is inside its range, both ends included. Blocks at different start times are separate classes, even for the same course and kind: on a lab day with two blocks, both show. When two blocks have the same course, the same kind and the same start time, only the one with the shorter date range is kept. This is how a one-off change replaces the usual block: a block for one Wednesday with another room wins over the block for the whole term. The classes are sorted by start time, then by course.

**Updating it for a new term.**

1. In Quest, open My Class Schedule and choose the list view.
2. For each class, take each row of its meeting information and write one line: the course, the kind (the section's component code), the days, the times, the room, and the first and last dates of that row. Change the times to 24-hour and the dates to `YYYY-MM-DD`. A class with several rows, such as a lecture that stops for reading week, needs a line for each row. A class with no time, such as an online one, is left out.
3. Put the lines in `timetable.txt` in place of the old ones, and update the comment at the top.
4. Check that each course has a folder in Downloads named like it, such as `ECE 150` (spaces and case do not matter), or its row is shown in grey and does nothing. The desktop for a course is named with its course number. For a click to open anything, `profiles.txt` needs the setups `<Subject>: Lecture` or `<Subject>: Tutorial`.
5. Run `.\tests\test.cmd`. The timetable checks read the real file and name real days of the Fall 2026 term, so they will fail for a new term. First correct `tests\specs\timetable.md` to the new term, then change the checks to match it. A check is not edited to make it pass.

Edits to the file apply the next time the panel opens.

## Settings

`settings.txt` is next to `crammy.exe`, in the same `key = value` style as `profiles.txt`. Lines starting with `#` and blank lines are ignored, key names ignore case, and spaces around the value are trimmed. A missing key keeps its default. An unknown key, a shortcut that cannot be read, or a `start at sign in` value other than `yes` or `no` gives an error `line N: ...` and keeps the default. The file may be missing altogether.

| Key | Default | Meaning |
|---|---|---|
| `shortcut` | `Ctrl+Alt+C` | The keys that show the panel. Modifiers are Ctrl, Alt, Shift and Win, and at least one is needed. The key is a letter, a digit, F1 to F24 or Space |
| `youtonomous` | `C:\Code\Youtonomous\build\youtonomous.exe` | The program the Pomodoro buttons start |
| `pomodoro long` | `C:\Users\Justin\Videos\Youtonomous\74cOUSKXMz0.mp4` | The video for the 3-hour button |
| `pomodoro short` | `C:\Users\Justin\Videos\Youtonomous\sdhh7AYzsTY.mp4` | The video for the 1.5-hour button |
| `start at sign in` | `yes` | `yes` or `no` |

`youtonomous_command` makes the command `"<program>" "<video>"`. When the program or the video is missing, it gives the sentence `Youtonomous was not found at <path>` or `The video was not found at <path>` for the panel to show.

To add a setting, change `crammy_settings`, `default_settings`, `read_settings` and `write_settings` in `settings.cpp`, then this table. Write the spec first (see Testing).

## Testing

The owner's rule is that every behaviour has a spec in `tests\specs`. The tests are written from that spec by someone who has not seen the code, and they are written before the code. They are run once to see them fail, and the code is finished when they all pass.

- A failing test means the code is wrong.
- A test changes only when the spec is corrected first. If a spec turns out to be wrong, change the spec, then the test, then the code.
- The older behaviour has its own spec, `tests\specs\existing.md`, and tests written from it in the same way.

There is one file for each area: `tests\test_<area>.cpp`, with a function `test_<area>()` made of `check( condition, "plain sentence" )` calls from `tests\check.h`. `wmain` in `tests\test_main.cpp` calls every one of them. To add an area, write its spec, write `test_<area>.cpp` from the spec, and call the function from `test_main.cpp`. `tests\test.cmd` already compiles `tests\*.cpp`; a new source file that the tests link against must be added to its list.

The checks never take over the mouse or keyboard and never take screenshots of the owner's screen. He has asked for that not to happen. Windows are made hidden or off screen, and drops are handed to the screen picture in code. When a change cannot be checked this way, add a way to check it, as `close_old_notes` does by taking the window to work on as a parameter.

**What the checks need on this machine.** Some checks use the owner's real setup, so on another machine they can fail for reasons that have nothing to do with a change.

- The profile checks read the real `profiles.txt` in the repo folder, one level above the test program.
- The drop checks use real files under `%USERPROFILE%\Downloads\ECE 190`, such as `1 Lectures\PDF\ECE190-W10-Slides.pdf`.
- The notes check starts a separate, minimized copy of Notepad++ with scratch files in `%TEMP%`, and closes it afterwards. It needs Notepad++ at the path in `program_file`.
- The timetable checks read the real `timetable.txt` and name days of the Fall 2026 term.
- The settings checks make files in `%TEMP%\crammy settings checks`.
- The background checks make a hidden window of the class `CrammyBackground` and set `CRAMMY_STARTUP_FOLDER` to a folder in `%TEMP%`. If a real background Crammy is running, they skip those checks and say so.
- The desktops checks only read. They need an ordinary application window on the current desktop, so the test makes one off screen. They never switch, create or rename a desktop.

**Pictures.** `tests\crammy_test.exe out.bmp` draws a sample setup into `out.bmp`, which is the way to see a change to the screen picture. `tests\crammy_test.exe --panel-picture panel.bmp` draws the panel, with sample classes and desktops, into a `.bmp`. Use it to judge a change to the look of the panel, since the real panel is not shown during the checks.

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
- Write the spec and the blind tests first (see Testing), then the code. A failing test is fixed in the code; a spec is corrected before any test that follows it.
- Run `git pull --rebase --autostash` before pushing, and read the push output; a rejected push can look like success if the output is hidden.
- `crammy.exe`, `tests\crammy_test.exe`, `last setup.txt` and `notes opened.txt` are not committed.
- When a change makes this guide or the README wrong, fix them in the same commit. Remove a Known limits entry as soon as the limit is gone.

## Known limits

- Program paths are fixed in `program_file`. If PdfCraft, Acrobat or Notepad++ moves, Crammy will report that no window appeared.
- If Notepad++ is open on another desktop, a new notes tab opens in that window, and moving it brings it to the current desktop.
- `newest` uses the modified date, so re-saving an old PDF makes it the newest.
- Only one monitor is handled: everything uses the primary monitor's work area, and the panel assumes one as well.
- `create_desktop` and the Windhawk mod are not covered by tests. They change the real desktops or the real taskbar, which a hidden test cannot do, so they were tried by hand. Try them by hand again after a Windows update.
- `create_desktop` depends on the slot order of an undocumented interface for build 26200. On another build it can call the wrong method. The Ctrl+Win+D fallback only runs when the interface cannot be reached or `CreateDesktop` fails.
- The Windhawk mod hooks a function in `Taskbar.View.dll` by its debug symbol, so a Windows update can rename it. Windhawk then reports that the mod failed to load, and the taskbar stays as Windows made it.
- The mod needs Windhawk itself to keep working. If the mod breaks, switch it off in Windhawk to bring the Widgets button back; the shortcut and the tray icon work without it.
- The mod has the path to `crammy.exe` as a setting, and reads the button's icon from that file. Change the setting when Crammy moves.
- The panel only knows the windows that Explorer tracks on a desktop (see Desktops). Tool windows and similar are not shown on the tiles.
