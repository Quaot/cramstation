# Cramstation

Cramstation opens the files for a study session and puts each window in its place. A lecture setup can open this week's slides on the left and the summary sheet in Acrobat on the right, on that subject's own desktop.

## Use

Run `cramstation.exe`. Setups are listed on the left and the middle shows a picture of your screen. Press Open, or double-click a setup, to open it. The last setup you opened is selected next time.

To build a setup, press New setup, then drag PDFs or folders from File Explorer onto the picture. Dropping on the left or right third fills that half of the screen; the middle fills the whole screen. Tiles can be dragged from one side to the other. Right-click a tile to open it in Acrobat, or to make it always open the newest file like it, which suits weekly slides. A folder becomes a list you tick from each time you open the setup. Every change is saved straight away.

Edit as text opens `profiles.txt` in a small editor for anything the picture does not cover.

It also runs from a terminal:

```
cramstation --setup "ECE 150: Exam" --pick F24-F
cramstation --setup "Math 115: Exam" --pick F23-F F23-M --separate
cramstation --list
cramstation --list "ECE 150: Exam"
cramstation --check
cramstation --edit
```

`--pick` takes the PDF with that exact name, or else the newest one whose name contains the text. `--separate` gives each picked PDF its own window.

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
| `notes` | Notepad with `<first PDF> - my answers (date).txt`, or the newest file of that name |
| `notes "file.txt"` | Notepad with that file, made if missing |
| `pdf "file.pdf"` | one PdfCraft window |
| `newest "L*.pdf"` | the newest PDF whose name fits the pattern |
| `tabs "a.pdf" "b.pdf"` | one PdfCraft window with a tab for each PDF |
| `pick "folder" "file"` | the PDFs you tick at launch |
| `folder "folder"` | File Explorer |

Ending a PDF line with `with acrobat` opens it in Adobe Acrobat in place of PdfCraft. Paths start from the root unless they are absolute. `*` matches any text in a file name, and variables such as `%USERPROFILE%` are expanded. Answer files go in `2 Practice` when the course has one. Set `notes folder = ...` to change that.

`desktop = 150` switches to the virtual desktop named 150 before anything opens, so each subject can keep its own desktop. Cramstation presses Ctrl+Win+Left or Right as many times as it takes and reads the desktop names from the registry.

A setup starts with every line of its profile, so lines shared by all setups can go under the profile heading.

## Build

The build needs MSYS2 with the UCRT64 g++. Run `build.cmd`. PdfCraft is expected at `C:\Program Files\PdfCraft`.

`tests\test.cmd` builds and runs the checks. They drop files on the picture in a hidden window, so nothing appears on screen.
