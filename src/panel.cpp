#include <windows.h>

#include "panel.h"
#include "ui.h"

// Temporary: opens the main window until the real panel is built.
void toggle_panel() {
    HWND open_window = FindWindowW( L"CrammyMain", nullptr );
    if ( !open_window ) {
        show_main_window( L"" );
        return;
    }
    if ( IsIconic( open_window ) ) ShowWindow( open_window, SW_RESTORE );
    SetForegroundWindow( open_window );
}
