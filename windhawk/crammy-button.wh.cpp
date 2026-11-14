// ==WindhawkMod==
// @id              crammy-button
// @name            Crammy button
// @description     Replaces the Widgets button with a button that opens the Crammy panel
// @version         0.1
// @author          Justin Gu
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lole32 -loleaut32 -lruntimeobject -lshell32 -lgdi32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Crammy button

Hides the Widgets button at the left end of the taskbar and puts a button with
the Crammy icon in its place. A click opens the Crammy panel. If Crammy is not
running, the click starts `crammy.exe --panel`.

Switch the mod off to bring the Widgets button back.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- crammy_path: C:\Users\Justin\Desktop\Projects\Crammy\crammy.exe
  $name: Path to crammy.exe
  $description: The program to start when Crammy is not running. The button's icon is read from it.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <atomic>
#include <string>
#include <vector>

#undef GetCurrentTime

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/base.h>

using namespace winrt::Windows::UI::Xaml;

const wchar_t button_name[] = L"CrammyButton";
const int icon_pixels = 48;

struct placed_button {
    winrt::weak_ref<Controls::Button> button;
    Controls::Primitives::ButtonBase::Click_revoker click;
};

std::wstring g_crammy_path;
std::atomic<bool> g_unloading;
std::atomic<bool> g_taskbar_view_hooked;
std::vector<placed_button> g_buttons;
std::vector<winrt::weak_ref<FrameworkElement>> g_hidden_widgets;
FrameworkElement::Loaded_revoker g_loaded_revoker;

void show_panel() {
    static const UINT show_message = RegisterWindowMessageW( L"CrammyShowPanel" );
    if ( HWND crammy_window = FindWindowW( L"CrammyBackground", nullptr ) ) {
        PostMessageW( crammy_window, show_message, 0, 0 );
        return;
    }
    size_t slash = g_crammy_path.find_last_of( L'\\' );
    std::wstring folder = slash == std::wstring::npos ? L"" : g_crammy_path.substr( 0, slash );
    ShellExecuteW( nullptr, L"open", g_crammy_path.c_str(), L"--panel",
                   folder.empty() ? nullptr : folder.c_str(), SW_SHOWNORMAL );
}

Media::ImageSource load_icon( const std::wstring &path ) {
    HICON icon = nullptr;
    if ( PrivateExtractIconsW( path.c_str(), 0, icon_pixels, icon_pixels, &icon, nullptr, 1, 0 ) != 1 || !icon ) {
        return nullptr;
    }
    ICONINFO info{};
    GetIconInfo( icon, &info );
    BITMAPINFO bitmap_info{};
    bitmap_info.bmiHeader = { sizeof( BITMAPINFOHEADER ), icon_pixels, -icon_pixels, 1, 32, BI_RGB };
    std::vector<BYTE> pixels( icon_pixels * icon_pixels * 4 );
    HDC screen = GetDC( nullptr );
    int lines = info.hbmColor ? GetDIBits( screen, info.hbmColor, 0, icon_pixels, pixels.data(), &bitmap_info, DIB_RGB_COLORS ) : 0;
    ReleaseDC( nullptr, screen );
    if ( info.hbmColor ) DeleteObject( info.hbmColor );
    if ( info.hbmMask ) DeleteObject( info.hbmMask );
    DestroyIcon( icon );
    if ( lines != icon_pixels ) {
        return nullptr;
    }
    // WriteableBitmap expects premultiplied alpha; icons store it straight.
    for ( size_t i = 0; i < pixels.size(); i += 4 ) {
        for ( size_t k = 0; k < 3; k++ ) {
            pixels[i + k] = (BYTE)( pixels[i + k] * pixels[i + 3] / 255 );
        }
    }
    Media::Imaging::WriteableBitmap bitmap( icon_pixels, icon_pixels );
    memcpy( bitmap.PixelBuffer().data(), pixels.data(), pixels.size() );
    bitmap.Invalidate();
    return bitmap;
}

FrameworkElement find_child( FrameworkElement element, PCWSTR name, bool by_class ) {
    int count = Media::VisualTreeHelper::GetChildrenCount( element );
    for ( int i = 0; i < count; i++ ) {
        auto child = Media::VisualTreeHelper::GetChild( element, i ).try_as<FrameworkElement>();
        if ( child && ( by_class ? winrt::get_class_name( child ) == name : child.Name() == name ) ) {
            return child;
        }
    }
    return nullptr;
}

FrameworkElement find_parent_by_class( FrameworkElement element, PCWSTR class_name ) {
    while ( element ) {
        element = Media::VisualTreeHelper::GetParent( element ).try_as<FrameworkElement>();
        if ( element && winrt::get_class_name( element ) == class_name ) {
            return element;
        }
    }
    return nullptr;
}

void add_button( Controls::Grid root_grid ) {
    for ( auto child : root_grid.Children() ) {
        auto element = child.try_as<FrameworkElement>();
        if ( element && element.Name() == button_name ) {
            return;
        }
    }
    Controls::Image image;
    image.Source( load_icon( g_crammy_path ) );
    image.Width( 24 );
    image.Height( 24 );

    Controls::Button button;
    button.Name( button_name );
    button.Content( image );
    button.Width( 40 );
    button.Height( 40 );
    button.Padding( Thickness{ 0, 0, 0, 0 } );
    button.Margin( Thickness{ 4, 0, 0, 0 } );
    button.BorderThickness( Thickness{ 0, 0, 0, 0 } );
    button.Background( Media::SolidColorBrush( winrt::Windows::UI::Color{ 0, 0, 0, 0 } ) );
    button.HorizontalAlignment( HorizontalAlignment::Left );
    button.VerticalAlignment( VerticalAlignment::Center );
    root_grid.Children().Append( button );

    auto click = button.Click( winrt::auto_revoke, []( auto &&, auto && ) { show_panel(); } );
    g_buttons.push_back( { winrt::make_weak( button ), std::move( click ) } );
}

// Returns false when the button is not in the taskbar's tree yet.
bool replace_widgets( FrameworkElement widgets ) {
    auto frame = find_parent_by_class( widgets, L"Taskbar.TaskbarFrame" );
    auto root_grid = frame ? find_child( frame, L"RootGrid", false ).try_as<Controls::Grid>() : nullptr;
    if ( !root_grid ) {
        return false;
    }
    if ( Automation::AutomationProperties::GetAutomationId( widgets ) != L"WidgetsButton" ) {
        return true;
    }
    widgets.Visibility( Visibility::Collapsed );
    bool known = false;
    for ( auto &hidden : g_hidden_widgets ) {
        known = known || hidden.get() == widgets;
    }
    if ( !known ) {
        g_hidden_widgets.push_back( winrt::make_weak( widgets ) );
    }
    add_button( root_grid );
    return true;
}

void restore_all() {
    g_loaded_revoker.revoke();
    for ( auto &placed : g_buttons ) {
        placed.click.revoke();
        auto button = placed.button.get();
        auto panel = button ? Media::VisualTreeHelper::GetParent( button ).try_as<Controls::Panel>() : nullptr;
        uint32_t index = 0;
        if ( panel && panel.Children().IndexOf( button, index ) ) {
            panel.Children().RemoveAt( index );
        }
    }
    g_buttons.clear();
    for ( auto &hidden : g_hidden_widgets ) {
        if ( auto widgets = hidden.get() ) {
            widgets.Visibility( Visibility::Visible );
        }
    }
    g_hidden_widgets.clear();
}

void apply_all( XamlRoot xaml_root ) {
    FrameworkElement element = xaml_root.Content().try_as<FrameworkElement>();
    if ( element && ( element = find_child( element, L"Taskbar.TaskbarFrame", true ) ) &&
         ( element = find_child( element, L"RootGrid", false ) ) &&
         ( element = find_child( element, L"TaskbarFrameRepeater", false ) ) ) {
        int count = Media::VisualTreeHelper::GetChildrenCount( element );
        for ( int i = 0; i < count; i++ ) {
            auto child = Media::VisualTreeHelper::GetChild( element, i ).try_as<FrameworkElement>();
            if ( child && winrt::get_class_name( child ) == L"Taskbar.AugmentedEntryPointButton" ) {
                replace_widgets( child );
            }
        }
    }
}

using AugmentedEntryPointButton_UpdateButtonPadding_t = void( WINAPI * )( void *pThis );
AugmentedEntryPointButton_UpdateButtonPadding_t AugmentedEntryPointButton_UpdateButtonPadding_Original;
void WINAPI AugmentedEntryPointButton_UpdateButtonPadding_Hook( void *pThis ) {
    AugmentedEntryPointButton_UpdateButtonPadding_Original( pThis );
    if ( g_unloading ) {
        return;
    }
    FrameworkElement widgets = nullptr;
    ( (IUnknown **)pThis )[1]->QueryInterface( winrt::guid_of<FrameworkElement>(), winrt::put_abi( widgets ) );
    if ( !widgets || replace_widgets( widgets ) ) {
        return;
    }
    g_loaded_revoker = widgets.Loaded( winrt::auto_revoke, []( auto const &sender, auto && ) {
        if ( auto element = sender.template try_as<FrameworkElement>(); element && !g_unloading ) {
            replace_widgets( element );
        }
    } );
}

// The taskbar's XamlRoot is reached the way m417z's "Taskbar tray icon
// spacing and grid" mod does it, through taskbar.dll's TaskbarHost.
void *CTaskBand_ITaskListWndSite_vftable;
using CTaskBand_GetTaskbarHost_t = void *( WINAPI * )( void *pThis, void **result );
CTaskBand_GetTaskbarHost_t CTaskBand_GetTaskbarHost_Original;
void *TaskbarHost_FrameHeight_Original;
using std__Ref_count_base__Decref_t = void( WINAPI * )( void *pThis );
std__Ref_count_base__Decref_t std__Ref_count_base__Decref_Original;

XamlRoot taskbar_xaml_root( HWND taskbar_window ) {
    HWND task_switch_window = (HWND)GetProp( taskbar_window, L"TaskbandHWND" );
    if ( !task_switch_window ) {
        return nullptr;
    }
    void *task_band = (void *)GetWindowLongPtr( task_switch_window, 0 );
    for ( int i = 0; *(void **)task_band != CTaskBand_ITaskListWndSite_vftable; i++ ) {
        if ( i == 20 ) {
            return nullptr;
        }
        task_band = (void **)task_band + 1;
    }
    void *host[2]{};
    CTaskBand_GetTaskbarHost_Original( task_band, host );
    if ( !host[0] && !host[1] ) {
        return nullptr;
    }
    // The offset of the frame inside TaskbarHost is read from FrameHeight:
    // sub rsp,28 then add rcx,<offset>.
    size_t frame_offset = 0x10;
    const BYTE *code = (const BYTE *)TaskbarHost_FrameHeight_Original;
    if ( code[0] == 0x48 && code[1] == 0x83 && code[2] == 0xEC && code[4] == 0x48 &&
         code[5] == 0x83 && code[6] == 0xC1 && code[7] <= 0x7F ) {
        frame_offset = code[7];
    }
    FrameworkElement frame = nullptr;
    ( *(IUnknown **)( (BYTE *)host[0] + frame_offset ) )
        ->QueryInterface( winrt::guid_of<FrameworkElement>(), winrt::put_abi( frame ) );
    auto result = frame ? frame.XamlRoot() : nullptr;
    std__Ref_count_base__Decref_Original( host[1] );
    return result;
}

HWND find_taskbar_window() {
    HWND found = nullptr;
    EnumWindows( []( HWND window, LPARAM param ) -> BOOL {
        DWORD process_id = 0;
        wchar_t class_name[32];
        if ( GetWindowThreadProcessId( window, &process_id ) && process_id == GetCurrentProcessId() &&
             GetClassNameW( window, class_name, ARRAYSIZE( class_name ) ) &&
             _wcsicmp( class_name, L"Shell_TrayWnd" ) == 0 ) {
            *(HWND *)param = window;
            return FALSE;
        }
        return TRUE;
    }, (LPARAM)&found );
    return found;
}

// XAML objects may only be touched from the taskbar's own thread.
void run_on_taskbar_thread( void ( *proc )( HWND ) ) {
    static const UINT run_message = RegisterWindowMessageW( L"Windhawk_RunFromWindowThread_" WH_MOD_ID );
    struct run_request {
        void ( *proc )( HWND );
        HWND window;
    };
    HWND window = find_taskbar_window();
    if ( !window ) {
        return;
    }
    DWORD thread_id = GetWindowThreadProcessId( window, nullptr );
    HHOOK hook = SetWindowsHookExW( WH_CALLWNDPROC, []( int code, WPARAM wparam, LPARAM lparam ) -> LRESULT {
        const CWPSTRUCT *message = (const CWPSTRUCT *)lparam;
        if ( code == HC_ACTION && message->message == run_message ) {
            auto *request = (run_request *)message->lParam;
            request->proc( request->window );
        }
        return CallNextHookEx( nullptr, code, wparam, lparam );
    }, nullptr, thread_id );
    if ( !hook ) {
        return;
    }
    run_request request{ proc, window };
    SendMessageW( window, run_message, 0, (LPARAM)&request );
    UnhookWindowsHookEx( hook );
}

void apply_on_taskbar( HWND window ) {
    if ( auto xaml_root = taskbar_xaml_root( window ) ) {
        apply_all( xaml_root );
    }
}

void restore_on_taskbar( HWND ) {
    restore_all();
}

void load_settings() {
    PCWSTR path = Wh_GetStringSetting( L"crammy_path" );
    g_crammy_path = path;
    Wh_FreeStringSetting( path );
}

HMODULE taskbar_view_module() {
    HMODULE module = GetModuleHandleW( L"Taskbar.View.dll" );
    return module ? module : GetModuleHandleW( L"ExplorerExtensions.dll" );
}

bool hook_taskbar_view( HMODULE module ) {
    WindhawkUtils::SYMBOL_HOOK hooks[] = {
        {
            { LR"(protected: virtual void __cdecl winrt::Taskbar::implementation::AugmentedEntryPointButton::UpdateButtonPadding(void))" },
            &AugmentedEntryPointButton_UpdateButtonPadding_Original,
            AugmentedEntryPointButton_UpdateButtonPadding_Hook,
        },
    };
    return WindhawkUtils::HookSymbols( module, hooks, ARRAYSIZE( hooks ) );
}

bool find_taskbar_symbols() {
    HMODULE module = LoadLibraryExW( L"taskbar.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 );
    if ( !module ) {
        return false;
    }
    WindhawkUtils::SYMBOL_HOOK symbols[] = {
        { { LR"(const CTaskBand::`vftable'{for `ITaskListWndSite'})" }, &CTaskBand_ITaskListWndSite_vftable },
        { { LR"(public: virtual class std::shared_ptr<class TaskbarHost> __cdecl CTaskBand::GetTaskbarHost(void)const )" }, &CTaskBand_GetTaskbarHost_Original },
        { { LR"(public: int __cdecl TaskbarHost::FrameHeight(void)const )" }, &TaskbarHost_FrameHeight_Original },
        { { LR"(public: void __cdecl std::_Ref_count_base::_Decref(void))" }, &std__Ref_count_base__Decref_Original },
    };
    return WindhawkUtils::HookSymbols( module, symbols, ARRAYSIZE( symbols ) );
}

using LoadLibraryExW_t = decltype( &LoadLibraryExW );
LoadLibraryExW_t LoadLibraryExW_Original;
HMODULE WINAPI LoadLibraryExW_Hook( LPCWSTR file_name, HANDLE file, DWORD flags ) {
    HMODULE module = LoadLibraryExW_Original( file_name, file, flags );
    if ( module && !g_taskbar_view_hooked && module == taskbar_view_module() &&
         !g_taskbar_view_hooked.exchange( true ) && hook_taskbar_view( module ) ) {
        Wh_ApplyHookOperations();
    }
    return module;
}

BOOL Wh_ModInit() {
    load_settings();
    if ( !find_taskbar_symbols() ) {
        return FALSE;
    }
    if ( HMODULE module = taskbar_view_module() ) {
        g_taskbar_view_hooked = true;
        return hook_taskbar_view( module );
    }
    auto kernel_base_load = (LoadLibraryExW_t)GetProcAddress( GetModuleHandleW( L"kernelbase.dll" ), "LoadLibraryExW" );
    WindhawkUtils::SetFunctionHook( kernel_base_load, LoadLibraryExW_Hook, &LoadLibraryExW_Original );
    return TRUE;
}

void Wh_ModAfterInit() {
    if ( !g_taskbar_view_hooked ) {
        if ( HMODULE module = taskbar_view_module() ) {
            if ( !g_taskbar_view_hooked.exchange( true ) && hook_taskbar_view( module ) ) {
                Wh_ApplyHookOperations();
            }
        }
    }
    run_on_taskbar_thread( apply_on_taskbar );
}

void Wh_ModBeforeUninit() {
    g_unloading = true;
    run_on_taskbar_thread( restore_on_taskbar );
}

void Wh_ModSettingsChanged() {
    load_settings();
    run_on_taskbar_thread( restore_on_taskbar );
    run_on_taskbar_thread( apply_on_taskbar );
}
