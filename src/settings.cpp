#include "settings.h"

namespace {

struct modifier_name {
    const wchar_t *name;
    UINT flag;
};

// In the order shortcut_text writes them
const modifier_name modifier_names[] = {
    { L"ctrl", MOD_CONTROL }, { L"alt", MOD_ALT }, { L"shift", MOD_SHIFT }, { L"win", MOD_WIN },
};

bool parse_key( const wstring &name, UINT &key ) {
    if ( name.size() == 1 && name[0] >= L'a' && name[0] <= L'z' ) {
        key = (UINT)( name[0] - L'a' + L'A' );
        return true;
    }
    if ( name.size() == 1 && name[0] >= L'0' && name[0] <= L'9' ) {
        key = name[0];
        return true;
    }
    if ( name == L"space" ) {
        key = VK_SPACE;
        return true;
    }
    if ( name.size() >= 2 && name.size() <= 3 && name[0] == L'f' ) {
        int number = 0;
        for ( size_t i = 1; i < name.size(); i++ ) {
            if ( name[i] < L'0' || name[i] > L'9' ) return false;
            number = number * 10 + ( name[i] - L'0' );
        }
        if ( number < 1 || number > 24 ) return false;
        key = VK_F1 + number - 1;
        return true;
    }
    return false;
}

wstring key_text( UINT key ) {
    if ( key == VK_SPACE ) return L"Space";
    if ( key >= VK_F1 && key <= VK_F24 ) return L"F" + std::to_wstring( key - VK_F1 + 1 );
    return wstring( 1, (wchar_t)key );
}

}  // namespace

crammy_settings default_settings() {
    crammy_settings settings;
    settings.shortcut_modifiers = MOD_CONTROL | MOD_ALT;
    settings.shortcut_key = 'C';
    settings.youtonomous = L"C:\\Code\\Youtonomous\\build\\youtonomous.exe";
    settings.pomodoro_long = L"C:\\Users\\Justin\\Videos\\Youtonomous\\74cOUSKXMz0.mp4";
    settings.pomodoro_short = L"C:\\Users\\Justin\\Videos\\Youtonomous\\sdhh7AYzsTY.mp4";
    settings.start_at_sign_in = true;
    return settings;
}

bool parse_shortcut( const wstring &text, UINT &modifiers, UINT &key ) {
    UINT found_modifiers = 0;
    UINT found_key = 0;
    bool has_key = false;
    size_t start = 0;
    while ( true ) {
        size_t plus = text.find( L'+', start );
        wstring part = lower( trim( text.substr( start, plus == wstring::npos ? wstring::npos : plus - start ) ) );
        if ( has_key ) return false;
        bool is_modifier = false;
        for ( const modifier_name &entry : modifier_names ) {
            if ( part == entry.name ) {
                found_modifiers |= entry.flag;
                is_modifier = true;
            }
        }
        if ( !is_modifier ) {
            if ( plus != wstring::npos || !parse_key( part, found_key ) ) return false;
            has_key = true;
        }
        if ( plus == wstring::npos ) break;
        start = plus + 1;
    }
    if ( !has_key || found_modifiers == 0 ) return false;
    modifiers = found_modifiers;
    key = found_key;
    return true;
}

wstring shortcut_text( UINT modifiers, UINT key ) {
    wstring text;
    const wchar_t *shown[] = { L"Ctrl", L"Alt", L"Shift", L"Win" };
    for ( int i = 0; i < 4; i++ )
        if ( modifiers & modifier_names[i].flag ) text += wstring( shown[i] ) + L"+";
    return text + key_text( key );
}

crammy_settings read_settings( const wstring &text, vector<wstring> &errors ) {
    crammy_settings settings = default_settings();
    size_t line_number = 0;
    size_t start = 0;
    while ( start <= text.size() ) {
        size_t end = text.find( L'\n', start );
        if ( end == wstring::npos ) end = text.size();
        wstring line = trim( text.substr( start, end - start ) );
        start = end + 1;
        line_number++;
        if ( line.empty() || line[0] == L'#' ) continue;

        wstring where = L"line " + std::to_wstring( line_number ) + L": ";
        size_t equals = line.find( L'=' );
        if ( equals == wstring::npos ) {
            errors.push_back( where + L"expected key = value" );
            continue;
        }
        wstring key = lower( trim( line.substr( 0, equals ) ) );
        wstring value = trim( line.substr( equals + 1 ) );

        if ( key == L"shortcut" ) {
            UINT modifiers = 0;
            UINT shortcut_key = 0;
            if ( parse_shortcut( value, modifiers, shortcut_key ) ) {
                settings.shortcut_modifiers = modifiers;
                settings.shortcut_key = shortcut_key;
            } else {
                errors.push_back( where + L"the shortcut " + value + L" is not understood" );
            }
        } else if ( key == L"youtonomous" ) {
            settings.youtonomous = value;
        } else if ( key == L"pomodoro long" ) {
            settings.pomodoro_long = value;
        } else if ( key == L"pomodoro short" ) {
            settings.pomodoro_short = value;
        } else if ( key == L"start at sign in" ) {
            if ( lower( value ) == L"yes" ) settings.start_at_sign_in = true;
            else if ( lower( value ) == L"no" ) settings.start_at_sign_in = false;
            else errors.push_back( where + L"start at sign in must be yes or no" );
        } else {
            errors.push_back( where + L"unknown setting " + key );
        }
    }
    return settings;
}

wstring write_settings( const crammy_settings &settings ) {
    wstring text;
    text += L"shortcut = " + shortcut_text( settings.shortcut_modifiers, settings.shortcut_key ) + L"\r\n";
    text += L"youtonomous = " + settings.youtonomous + L"\r\n";
    text += L"pomodoro long = " + settings.pomodoro_long + L"\r\n";
    text += L"pomodoro short = " + settings.pomodoro_short + L"\r\n";
    text += wstring( L"start at sign in = " ) + ( settings.start_at_sign_in ? L"yes" : L"no" ) + L"\r\n";
    return text;
}

bool youtonomous_command( const crammy_settings &settings, bool long_video, wstring &command, wstring &problem ) {
    command.clear();
    problem.clear();
    const wstring &video = long_video ? settings.pomodoro_long : settings.pomodoro_short;
    if ( !is_file( settings.youtonomous ) ) {
        problem = L"Youtonomous was not found at " + settings.youtonomous;
        return false;
    }
    if ( !is_file( video ) ) {
        problem = L"The video was not found at " + video;
        return false;
    }
    command = L"\"" + settings.youtonomous + L"\" \"" + video + L"\"";
    return true;
}

wstring settings_path() {
    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW( nullptr, exe_path, MAX_PATH );
    return join_path( parent_folder( exe_path ), L"settings.txt" );
}
