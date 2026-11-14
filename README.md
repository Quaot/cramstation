# Crammy

Crammy opens the files for a study session and puts each window in its place. A lecture setup can open this week's slides on the left and the summary sheet in Acrobat on the right, on that subject's own desktop.

## Use

Run `crammy.exe`. Setups are listed on the left and the middle shows a picture of your screen. Press Open, or double-click a setup, to open it. The last setup you opened is selected next time.

To build a setup, press New setup, then drag PDFs or folders from File Explorer onto the picture. Dropping on the left or right third fills that half of the screen; the middle fills the whole screen. Tiles can be dragged from one side to the other. Right-click a tile to open it in Acrobat, or to make it always open the newest file like it, which suits weekly slides. A folder becomes a list you tick from each time you open the setup. Every change is saved straight away.

Edit as text opens `profiles.txt` in a small editor for anything the picture does not cover.

Notes open in Notepad++. Each time a setup opens, Crammy saves and closes the notes tabs it opened before, using commands Notepad++ accepts from other programs, so nothing comes to the front and no keys are pressed. Tabs you opened yourself stay open.

It also runs from a terminal:

```
crammy --setup "ECE 150: Exam" --pick F24-F
crammy --setup "Math 115: Exam" --pick F23-F F23-M --separate
crammy --list
crammy --list "ECE 150: Exam"
crammy --check
crammy --edit
```

`--pick` takes the PDF with that exact name, or else the newest one whose name contains the text. `--separate` gives each picked PDF its own window.

## The panel

The panel is a window about 300 pixels wide. It opens above the bottom-left corner of the taskbar and grows upward to fit what it shows. The top line has the date, with the time at the right.

Under the date is a row for each of today's classes from the timetable. A row starts with the start time and the course. After them comes the kind as a lower-case code from Quest, such as `lec` or `tut`. Only the class on now, or the next one today, is drawn in red. That row also shows the room, and at the right the minutes until it starts, such as `25m`, or `now` while it is on. A course with no folder in Downloads is grey. A mistake in the timetable or the settings is shown in one line under the classes.

Below the classes is a chip for each virtual desktop, four to a row, with its name and the icons of up to 3 programs open on it. The chip of the desktop showing has a red outline. At the bottom are plain red text links. `Pomodoro 3h` and `1.5h` start Youtonomous on the long or the short video. `Setups…` brings up the main window.

Clicking a class goes to the desktop named after its course number, and makes that desktop if it is missing. When nothing is open there yet, the subject's setup opens on it. Clicking a chip switches to that desktop.

Open the panel with Ctrl+Alt+C or with the Crammy icon in the tray. The Crammy button on the taskbar opens it as well. That button comes from a Windhawk mod (see `windhawk\README.md`), and the shortcut and the tray icon work without it.

These work while Crammy runs in the background. Start it with `crammy --background`, or choose Start at sign-in in the tray menu to make a shortcut in the Startup folder that does so each time you sign in. `crammy --panel` shows the panel and starts the background copy first when it is not running.

The timetable is `timetable.txt` and the settings are `settings.txt`, both next to the exe. The settings file may be missing; `docs/guide.md` lists its keys and says how to copy a new term's classes from Quest into the timetable.

## Profiles

`profiles.txt` sits next to the exe. A profile names a root folder. Its setups say what opens on each side.

```
[ECE 150]
root = Downloads\ECE 150

[ECE 150: Exam]
left = notes
right = pick "6 Exams" "2 Practice"

[ECE 150: Lecture and lab]
left = tabs "1 Lectures\PDF\*Week 10*"
right = pdf "4 Labs\Lab 5.pdf"
right = folder "4 Labs"
```

A side is `left` or `right`, or `full` for the whole screen. When several windows share a side they are stacked from top to bottom in the order written. A window with a side to itself is snapped the way Win+Left snaps it, so dragging the line between two snapped windows resizes both.

| Line | Opens |
|---|---|
| `notes` | Notepad++ with `<first PDF> - my answers (date).txt`, or the newest file of that name |
| `notes "file.txt"` | Notepad++ with that file, made if missing |
| `pdf "file.pdf"` | one PdfCraft window |
| `newest "L*.pdf"` | the newest PDF whose name fits the pattern |
| `tabs "a.pdf" "b.pdf"` | one PdfCraft window with a tab for each PDF |
| `pick "folder" "file"` | the PDFs you tick at launch |
| `folder "folder"` | File Explorer |

Ending a PDF line with `with acrobat` opens it in Adobe Acrobat in place of PdfCraft. Paths start from the root unless they are absolute. `*` matches any text in a file name, and variables such as `%USERPROFILE%` are expanded. Answer files go in `2 Practice` when the course has one. Set `notes folder = ...` to change that.

`desktop = 150` switches to the virtual desktop named 150 before anything opens, so each subject can keep its own desktop. Crammy presses Ctrl+Win+Left or Right as many times as it takes and reads the desktop names from the registry.

A setup starts with every line of its profile, so lines shared by all setups can go under the profile heading.

## Build

The build needs MSYS2 with the UCRT64 g++. Run `build.cmd`. PdfCraft is expected at `C:\Program Files\PdfCraft`.

`tests\test.cmd` builds and runs the checks. They drop files on the picture in a hidden window, so nothing appears on screen.

`docs/guide.md` explains how Crammy works and how to change it.
