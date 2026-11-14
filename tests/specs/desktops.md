# Spec: reading desktops and windows

The tests go in `tests\test_desktops.cpp`, in `void test_desktops();`, made of `check()` calls from `tests\check.h`. They only read. They never switch, create or rename desktops.

## Interface (`src\desktops.h`, added to what is already there)

```cpp
vector<wstring> desktop_names();    // already exists: names in Task View order; unnamed ones are "Desktop N"
int current_desktop_index();        // index into desktop_names() of the desktop showing, or -1

struct window_on_desktop {
    HWND window;
    int desktop_index;              // index into desktop_names()
    wstring program_path;           // full path of the program that owns the window
};

vector<window_on_desktop> windows_by_desktop();
bool create_desktop( const wstring &name, wstring &problem );   // not called by tests
```

**`windows_by_desktop`** lists top-level windows that are visible, have a title and have no owner. It gives each one's desktop, using `IVirtualDesktopManager::GetWindowDesktopId` matched against the desktop ids Windows keeps in the registry. Windows whose desktop cannot be found are left out.

## Behaviour

- `desktop_names()` is not empty, and has no empty names.
- `current_desktop_index()` is between 0 and the number of names minus 1.
- The test makes a visible, titled top-level window placed off screen, at x = -32000 and y = -32000, sized 200 by 100, with a unique title. Then it calls `windows_by_desktop()`:
  - the result includes that window;
  - its `desktop_index` equals `current_desktop_index()`;
  - its `program_path` matches the test program's own path, compared without regard to case.

  The test destroys the window straight away.
- A window made hidden (never shown) is not included.
- Every entry's `desktop_index` is a valid index into `desktop_names()`.
