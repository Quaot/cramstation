# Cramstation

Cramstation opens the files for a study session and puts each window in its place. A typical setup puts notes in Notepad on the left half of the screen and a past exam in PdfCraft on the right. Setups are written in a plain text file, so changing what opens takes a few lines.

## Use

Run `cramstation.exe` and pick a setup from the list. If the setup lets you choose PDFs, tick the ones you want and press Open. Double-clicking a PDF opens it straight away. The last setup you opened is selected next time.

Edit profiles opens `profiles.txt` in a small editor. Saving checks the file and reports the first mistake with its line number.

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
| `tabs "a.pdf" "b.pdf"` | one PdfCraft window with a tab for each PDF |
| `pick "folder" "file"` | the PDFs you tick at launch |
| `folder "folder"` | File Explorer |

Paths start from the root unless they are absolute. `*` matches any text in a file name, and variables such as `%USERPROFILE%` are expanded. Answer files go in `2 Practice` when the course has one. Set `notes folder = ...` to change that.

`desktop = 150` switches to the virtual desktop named 150 before anything opens, so each subject can keep its own desktop. Cramstation presses Ctrl+Win+Left or Right as many times as it takes and reads the desktop names from the registry.

A setup starts with every line of its profile, so lines shared by all setups can go under the profile heading.

## Build

The build needs MSYS2 with the UCRT64 g++. Run `build.cmd`. PdfCraft is expected at `C:\Program Files\PdfCraft`.
