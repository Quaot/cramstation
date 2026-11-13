#include "ui.h"

#include <commctrl.h>

int scaled( int value, UINT dpi ) {
    return MulDiv( value, static_cast<int>( dpi ), 96 );
}

HFONT make_font( UINT dpi, const wchar_t *face, int points ) {
    LOGFONTW font{};
    font.lfHeight = -MulDiv( points, static_cast<int>( dpi ), 72 );
    font.lfWeight = FW_NORMAL;
    font.lfQuality = CLEARTYPE_QUALITY;
    lstrcpynW( font.lfFaceName, face, LF_FACESIZE );
    return CreateFontIndirectW( &font );
}

HWND add_control( HWND parent, const wchar_t *class_name, const wchar_t *text, DWORD style, int id, HFONT font,
                  DWORD ex_style ) {
    HWND control = CreateWindowExW( ex_style, class_name, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, parent,
                                    reinterpret_cast<HMENU>( static_cast<INT_PTR>( id ) ), GetModuleHandleW( nullptr ),
                                    nullptr );
    SendMessageW( control, WM_SETFONT, reinterpret_cast<WPARAM>( font ), TRUE );
    return control;
}

RECT centred_on_screen( int width, int height ) {
    RECT screen;
    SystemParametersInfoW( SPI_GETWORKAREA, 0, &screen, 0 );
    int left = screen.left + ( screen.right - screen.left - width ) / 2;
    int top = screen.top + ( screen.bottom - screen.top - height ) / 2;
    return { left, top, left + width, top + height };
}

void run_message_loop() {
    MSG message;
    while ( GetMessageW( &message, nullptr, 0, 0 ) > 0 ) {
        HWND top = GetAncestor( message.hwnd, GA_ROOT );
        if ( message.message == WM_KEYDOWN && message.wParam == 'S' && ( GetKeyState( VK_CONTROL ) & 0x8000 ) ) {
            SendMessageW( top, WM_COMMAND, save_shortcut, 0 );
            continue;
        }
        if ( top && IsDialogMessageW( top, &message ) ) continue;
        TranslateMessage( &message );
        DispatchMessageW( &message );
    }
}
