# Spec: settings and the Youtonomous command

The tests go in `tests\test_settings.cpp`, in `void test_settings();`, made of `check()` calls from `tests\check.h`. Temporary files go under `%TEMP%\crammy settings checks`.

## Interface (`src\settings.h`)

```cpp
struct crammy_settings {
    UINT shortcut_modifiers;   // MOD_CONTROL, MOD_ALT, MOD_SHIFT, MOD_WIN, combined with |
    UINT shortcut_key;         // a virtual-key code, such as 'C' or VK_F9
    wstring youtonomous;       // path to youtonomous.exe
    wstring pomodoro_long;     // the 3-hour video
    wstring pomodoro_short;    // the 1.5-hour video
    bool start_at_sign_in;
};

crammy_settings default_settings();
crammy_settings read_settings( const wstring &text, vector<wstring> &errors );
wstring write_settings( const crammy_settings &settings );
bool parse_shortcut( const wstring &text, UINT &modifiers, UINT &key );
wstring shortcut_text( UINT modifiers, UINT key );
bool youtonomous_command( const crammy_settings &settings, bool long_video, wstring &command, wstring &problem );
wstring settings_path();   // settings.txt next to the running program
```

## Behaviour

`default_settings()` gives:

- shortcut Ctrl+Alt+C, which is `MOD_CONTROL | MOD_ALT` with key `'C'`;
- youtonomous `C:\Code\Youtonomous\build\youtonomous.exe`;
- pomodoro long `C:\Users\Justin\Videos\Youtonomous\74cOUSKXMz0.mp4`;
- pomodoro short `C:\Users\Justin\Videos\Youtonomous\sdhh7AYzsTY.mp4`;
- start at sign in true.

`settings.txt` uses `key = value` lines; `#` lines and blank lines are ignored. The keys are `shortcut`, `youtonomous`, `pomodoro long`, `pomodoro short` and `start at sign in`. Key names ignore case, and spaces around `=` and around the value are trimmed.

- A key that is missing keeps its default.
- An unknown key, a shortcut that does not parse, or a `start at sign in` value other than `yes` or `no` adds an error starting `line N: `. The setting keeps its default.

`parse_shortcut`:

| Text | Result |
|---|---|
| `Ctrl+Alt+C` | true, `MOD_CONTROL\|MOD_ALT`, `'C'` |
| `ctrl + alt + c` | the same |
| `Win+Shift+F9` | true, `MOD_WIN\|MOD_SHIFT`, `VK_F9` |
| `Alt+1` | true, `MOD_ALT`, `'1'` |
| `Ctrl+Space` | true, `MOD_CONTROL`, `VK_SPACE` |
| `C` | false (a modifier is required) |
| `Ctrl+` | false |
| `Alt+Foo` | false |
| `Ctrl+Alt` | false (no key) |

The modifier names are Ctrl, Alt, Shift and Win. The key names are single letters A-Z, digits 0-9, F1-F24 and Space.

`shortcut_text` writes the modifiers in the order Ctrl, Alt, Shift, Win, then the key, joined by `+`: `shortcut_text( MOD_CONTROL|MOD_ALT, 'C' )` is `Ctrl+Alt+C`, and `shortcut_text( MOD_WIN|MOD_SHIFT, VK_F9 )` is `Shift+Win+F9`. Parsing that text gives the same values back.

`write_settings` writes all five keys. `read_settings( write_settings( s ) )` gives back the same values with no errors, for the defaults and for a changed set: shortcut Win+Shift+F9, other paths, start at sign in false. `start at sign in` is written as `yes` or `no`.

`youtonomous_command`:

- When the program and the chosen video both exist, it returns true, with the command `"<youtonomous>" "<video>"` (each path in double quotes, separated by one space) and an empty problem. `long_video` true picks `pomodoro_long`; false picks `pomodoro_short`.
- When the program file does not exist, it returns false, the command is empty, and the problem is `Youtonomous was not found at <path>`.
- When the video does not exist, it returns false, and the problem is `The video was not found at <path>`.
- Check these with files made in the temporary folder.

`settings_path()` ends with `\settings.txt`.
