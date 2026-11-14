# Design: the Crammy panel

Written 2026-11-14 for the owner to approve before anything is built.

## What it is for

Switching subjects should take one click from the bottom-left corner of the taskbar. The panel shows today's classes from the Quest timetable and every virtual desktop as a tile with the icons of what is open on it. It also starts one of the two Pomodoro study videos in Youtonomous. Clicking a class goes to that subject's desktop, creating the desktop first if it does not exist, and opens the subject's setup there if nothing is open yet.

## Goals

- A Crammy button in the bottom-left corner of the taskbar, where the Widgets weather button is now, that opens the panel.
- The same panel from a keyboard shortcut and from a tray icon, so it still works if the taskbar button ever breaks.
- Today's classes in time order, with the current or next one marked.
- A tile for every virtual desktop, not only subject ones. Clicking a tile switches to that desktop.
- A named desktop for every subject that has a course folder, created when first needed.
- Two buttons that start Youtonomous on the 3-hour or the 1.5-hour Pomodoro video.
- The look of PdfCraft and the Crammy icon: black ink on cream paper, with PdfCraft's red for highlights.

## Not part of this

- Live previews of other desktops. Windows hides windows that are not on the current desktop, so a preview would be blank; icons are used instead.
- More than one monitor.
- Editing the timetable inside Crammy. A new term means pasting the Quest page again (see Timetable).

## What would make it a failure even if it worked

- A taskbar that crashes or misbehaves. Code that runs inside Explorer is kept as small as possible.
- Focus or desktops changing when the owner did not click anything.
- A machine-made look, such as pastel cards or paragraphs of instructions.
- Testing that takes over the owner's mouse, keyboard or screen.

## Approaches considered

1. A Windhawk mod that does everything inside Explorer. It puts the panel in the exact spot, but all of Crammy's logic would run inside the process that draws the taskbar, so any bug crashes the taskbar.
2. Crammy alone, in the background with a shortcut and a tray icon. Safest, but nothing in the bottom-left corner.
3. **Chosen by the owner:** both. Crammy runs in the background with a shortcut and a tray icon, and a small Windhawk mod draws only the button and asks Crammy to show its panel.

## How it works

### Crammy in the background

- `crammy.exe --background` starts Crammy with no window: a tray icon, a keyboard shortcut and a hidden message window of class `CrammyBackground`. It is started at sign-in by a shortcut in the Startup folder, through `explorer.exe` so that it is not tied to whatever started it.
- Only one background Crammy runs. Starting `crammy.exe --panel` while it runs tells it to show the panel and exits; if none is running, it starts one and shows the panel.
- The shortcut is Ctrl+Alt+C (my call; the owner can change it). It is registered with `RegisterHotKey`, so nothing else is listening to the keyboard.
- The tray icon opens the panel on a left click. A right click gives a menu with Setups…, Edit timetable, Start at sign-in (on or off) and Quit.
- The existing main window (setups and the screen picture) stays as it is and opens from the panel's Setups… button or from `crammy.exe` with no arguments.

### The taskbar button (Windhawk mod)

- The mod's source is `windhawk/crammy-button.wh.cpp` in the repo. The owner reads it and adds it to Windhawk himself; nothing is installed or switched on without his yes.
- It hides the Widgets button and puts a button with the Crammy icon in its place, at the left end of the taskbar. A click posts one registered message (`CrammyShowPanel`) to the `CrammyBackground` window, or runs `crammy.exe --panel` when that window does not exist. That is all the mod does.
- If Windhawk or a Windows update breaks the mod, switching it off in Windhawk brings the Widgets button back. The shortcut and the tray icon keep working either way.

### The panel

A borderless window that opens just above the bottom-left corner of the taskbar, the way the Start menu does, and closes when it loses focus or Esc is pressed. Top to bottom:

1. **Header:** the date and time.
2. **Today:** one row per class, with time, course, kind (lecture, tutorial, lab, test, seminar) and room. The class happening now, or the next one today, is marked in red. Classes for a course without a folder in Downloads (GENE 119, MTHEL 99) are shown in grey and do nothing. With no classes today the section says so in one line.
3. **Desktops:** a tile for each virtual desktop in Task View order, labelled with its name. The tile shows the icons of the windows open on that desktop, up to eight. The current desktop's tile has a red outline. Clicking a tile switches to it.
4. **Buttons:** "Pomodoro 3 h" and "Pomodoro 1.5 h" start Youtonomous; "Setups…" opens the main window.

Clicking a class row:

1. Finds the subject's desktop. Desktops are matched by name: `150` for ECE 150, `192` for ENGL 192, and so on (the course number). The existing `150` and `192` desktops are used as they are.
2. If the desktop does not exist, creates it at the end of the list and names it with the course number.
3. Switches to it.
4. If no windows are open on it, opens the subject's setup for that kind of class: a lecture opens `<Subject>: Lecture`, a tutorial opens `<Subject>: Tutorial` if one exists, and other kinds only switch. With no matching setup it only switches. If the setup has `pick` lines, the tick list appears on the new desktop, as it does after pressing Open (my call: the owner clicked the class, so a question about which PDFs is expected).

### Style

Near-black ink text (11, 11, 12) on a cream background (239, 233, 220). PdfCraft red (183, 2, 40) marks what is current, such as the next class and the desktop showing, and fills the buttons. Thin ink rules separate the sections, and tiles have thin ink borders with square corners. Text is in Segoe UI, with the date and section headings in a serif face (Georgia) to match the engraved icon. There are no gradients or pastel fills, and no text explains how to use the panel.

### Timetable

`timetable.txt` next to `crammy.exe` holds one line per class block, copied from the Quest "My Class Schedule" list view:

```
ECE 150 | LEC | MWF | 13:30-14:20 | PSE 5353 | 2026-10-28 to 2026-12-08
ECE 150 | LAB | W   | 10:30-12:20 | E2 1792  | 2026-11-04 to 2026-11-11
```

Days use Quest's letters (M T W Th F). Kinds are Quest's component codes, and the Fall 2026 page uses LEC, TUT, LAB, TST and SEM. MTHEL 99 has no time ("TBA, online") and is left out. The owner's Fall 2026 page, pasted into the design conversation on 2026-11-14, is the source for the first `timetable.txt`. It is written into the repo as `timetable.txt` together with these rules, so the tests can use real cases from it:

- ECE 150 LAB on Wednesdays changes week to week. On Nov 4 it is 08:30-09:20 in PSE 5353 and also 10:30-12:20 in E2 1792; on Nov 11 it is 08:30-10:20 in PSE 5353 and also 10:30-12:20 in E2 1792; on Nov 18 only 08:30-09:20 in PSE 5353. Blocks at different times on the same day are separate classes, so both show.
- Evening tests: MATH 115 TST on Mon Oct 26 at 20:00-21:50, and ECE 105 TST on Tue Oct 27 at 16:30-18:20.
- Reading week: the ECE 150, ECE 105, ECE 190, MATH 115 and MATH 117 lecture ranges end on Oct 19 or 20 and restart on Oct 28, so those lectures do not show from Oct 21 to Oct 27. ENGL 192's lecture range runs Sep 9 to Dec 8 without a break, so it still shows that week.

A class is on a given day when the day matches and the date is inside the range. When a course has two blocks of the same kind at the same time on the same day, the block with the shorter date range wins, so one-off changes such as a room swap replace the usual block. The first version of the file is written by hand from the Quest page the owner pasted, and Crammy reads it on every panel open, so edits apply straight away.

### Desktops: creating, naming, switching and reading

- Reading names and switching stays as it is now: names from the registry, switching with Ctrl+Win+Left or Right.
- Creating and naming a desktop has no documented API. Summon's `src/virtual_desktops.cpp` (Quaot/summon) shows how to reach Explorer's `IVirtualDesktopManagerInternal` on build 26200, but it only moves windows; it has no create or rename calls. Crammy adds them in its own `desktops.cpp`: `CreateDesktop` and `SetDesktopName` on the build 26200 interface, with the method order checked against a published list for that build before use (my call). If the interface cannot be reached or a call fails, Crammy falls back to pressing Ctrl+Win+D, which creates a desktop and switches to it, and leaves it unnamed. In that case the panel says in one line that the desktop needs naming in Task View. These calls cannot be checked in a hidden test because they change the owner's real desktops, so the owner tries them once, after he agrees.
- Which desktop a window is on comes from the documented `IVirtualDesktopManager::GetWindowDesktopId`, which works for any program's windows. Tiles list visible top-level windows with a title, grouped by desktop, with each window's program icon.

### Youtonomous

Youtonomous starts only when the owner presses one of the two Pomodoro buttons. Nothing else starts it: clicking a class, opening a setup and the time of day never do. The buttons are always in the panel, so it can be started whenever he chooses, on whichever desktop is showing.

The Pomodoro buttons run `C:\Code\Youtonomous\build\youtonomous.exe` with the video file as its argument: `C:\Users\Justin\Videos\Youtonomous\74cOUSKXMz0.mp4` (3 h) or `sdhh7AYzsTY.mp4` (1.5 h). Both paths are settings, not built in.

### Settings

`settings.txt` next to `crammy.exe`, in the same `key = value` style as `profiles.txt`: the shortcut, the Youtonomous program path, the two video paths, and whether to start at sign-in.

## Testing

The tests come before the code and are written from this design, not from the program. Each behaviour below is written up as a short spec with exact inputs and expected results. An agent that has not seen the implementation then writes the tests from that spec alone, and they are run once to see them fail. Only then is the code written, and it is finished when every test passes. A failing test means the code is wrong; a test changes only if it disagrees with the spec, and then the spec is corrected first. The same is done for Crammy's existing behaviour: a spec of what it does now, and blind tests written from it.

Everything that can be checked without the owner's screen goes into `tests\test.cmd`:

- reading `timetable.txt` and listing the classes for chosen dates, including the one-off changes (the ECE 150 lab room on Nov 4 and the test evenings);
- matching class rows to desktop names and setups;
- the panel drawn into a picture with sample classes and desktops, to judge the look;
- the single-instance handover (`--panel` reaching a running hidden instance) inside the test process.

Creating desktops and the Windhawk mod cannot be checked in a hidden window. The mod is checked by compiling it, and the owner tries both once.

## Left to the builder

- Exact sizes and spacing, within the style above.
- How the tray menu and panel share code with the existing main window.
- The order of work and the commit split.
