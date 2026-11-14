# Crammy button

`crammy-button.wh.cpp` is a Windhawk mod for the Windows 11 taskbar. It hides the Widgets button, which shows the weather at the left end of the taskbar. In its place it puts a button with the Crammy icon.

A click on the button opens the Crammy panel. If Crammy is running in the background, the mod posts the `CrammyShowPanel` message to its `CrammyBackground` window. If it is not running, the mod starts `crammy.exe --panel`. The mod does nothing more than that.

## Adding it

1. Open Windhawk and press Create a new mod.
2. Delete the sample code, then paste in the whole of `crammy-button.wh.cpp`.
3. Press Compile Mod. When it compiles, the mod is switched on, and the button appears on the taskbar.
4. Press Exit Editing Mode.

The mod has one setting: the path to `crammy.exe`. It starts as `C:\Users\Justin\Desktop\Projects\Crammy\crammy.exe`. The button's icon is read from that file, so change the setting if Crammy moves.

## Switching it off

In Windhawk, open the mod and turn off the Enabled switch, or remove the mod. The Crammy button goes away and the Widgets button comes back. Explorer does not need a restart. The Crammy shortcut and the tray icon keep working without the mod.

## How it works

The mod follows the approach of two published mods by m417z. They are "Taskbar height and icon size" (`taskbar-icon-size`) and "Taskbar tray icon spacing and grid" (`taskbar-notification-icon-spacing`). Like them, it hooks a function in `Taskbar.View.dll`, found by its debug symbol. That function is `AugmentedEntryPointButton::UpdateButtonPadding`, which the taskbar calls for the Widgets button. When the mod is switched on while the taskbar is already showing, it reaches the taskbar's XAML tree through `taskbar.dll` the way the tray spacing mod does.

A Windows update can rename these functions. If that happens, Windhawk reports that the mod failed to load, and the taskbar stays as Windows made it.
