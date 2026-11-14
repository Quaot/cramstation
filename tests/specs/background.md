# Spec: background mode and start at sign-in

The tests go in `tests\test_background.cpp`, in `void test_background();`, made of `check()` calls from `tests\check.h`. They make only hidden windows, and they put the startup shortcut in a temporary folder, never the real Startup folder.

## Interface (`src\background.h`)

```cpp
extern const wchar_t *background_class_name;   // L"CrammyBackground"
UINT show_panel_message();                      // RegisterWindowMessageW( L"CrammyShowPanel" )
bool background_is_running();                   // a top-level window of class CrammyBackground exists
bool ask_running_crammy_to_toggle_panel();      // posts show_panel_message() to it; false if there is none
bool set_start_at_sign_in( bool on );           // creates or removes Crammy.lnk in the Startup folder
wstring startup_shortcut_path();                // <Startup folder>\Crammy.lnk
int run_background( bool show_panel_now );      // not called by tests
```

**The Startup folder** is the user's Startup folder (`FOLDERID_Startup`). When the environment variable `CRAMMY_STARTUP_FOLDER` is set, that folder is used in its place, so tests never touch the real one.

## Behaviour

**Sending the toggle message**

- Before the test makes any window of class `CrammyBackground`, `background_is_running()` is false and `ask_running_crammy_to_toggle_panel()` returns false. (The test first checks that no real Crammy is running with `FindWindowW( L"CrammyBackground", nullptr )`. If one is, it skips these checks with a plain line saying why.)
- The test registers a window class named `CrammyBackground` and makes a hidden top-level window of it, whose window procedure counts the messages equal to `show_panel_message()`.
  - `background_is_running()` is then true.
  - `ask_running_crammy_to_toggle_panel()` returns true.
  - After pumping the thread's messages, the count is 1. Asking twice more gives a count of 3.
- `show_panel_message()` gives the same non-zero value on every call.
- After the test destroys the window, `background_is_running()` is false again.

**Start at sign-in**

- With `CRAMMY_STARTUP_FOLDER` set to `%TEMP%\crammy startup check`, `startup_shortcut_path()` is `<that folder>\Crammy.lnk`.
- `set_start_at_sign_in( true )` returns true and creates that file. It is a shortcut (read it with `IShellLinkW`) whose target is the running test program's own folder followed by `\crammy.exe`, and whose arguments are `--background`.
- Calling `set_start_at_sign_in( true )` again returns true and still leaves one file.
- `set_start_at_sign_in( false )` returns true and removes the file. Calling it again when there is no file also returns true.
- The test unsets `CRAMMY_STARTUP_FOLDER` at the end.
