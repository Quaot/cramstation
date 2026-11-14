// Written from tests\specs\settings.md.

#include <windows.h>

#include "check.h"
#include "../src/settings.h"

namespace {

wstring scratch_folder() {
    return join_path( expand_variables( L"%TEMP%" ), L"crammy settings checks" );
}

wstring make_file( const wstring &name ) {
    CreateDirectoryW( scratch_folder().c_str(), nullptr );
    wstring path = join_path( scratch_folder(), name );
    write_text_file( path, L"x" );
    return path;
}

void remove_scratch() {
    const wchar_t *names[] = { L"program.exe", L"long.mp4", L"short.mp4" };
    for ( const wchar_t *name : names )
        DeleteFileW( join_path( scratch_folder(), name ).c_str() );
    RemoveDirectoryW( scratch_folder().c_str() );
}

bool parses_as( const wstring &text, UINT want_modifiers, UINT want_key ) {
    UINT modifiers = 0;
    UINT key = 0;
    return parse_shortcut( text, modifiers, key ) && modifiers == want_modifiers && key == want_key;
}

bool fails_to_parse( const wstring &text ) {
    UINT modifiers = 0;
    UINT key = 0;
    return !parse_shortcut( text, modifiers, key );
}

bool same_settings( const crammy_settings &a, const crammy_settings &b ) {
    return a.shortcut_modifiers == b.shortcut_modifiers && a.shortcut_key == b.shortcut_key
        && a.youtonomous == b.youtonomous && a.pomodoro_long == b.pomodoro_long
        && a.pomodoro_short == b.pomodoro_short && a.start_at_sign_in == b.start_at_sign_in;
}

bool is_default_shortcut( const crammy_settings &settings ) {
    return settings.shortcut_modifiers == ( MOD_CONTROL | MOD_ALT ) && settings.shortcut_key == 'C';
}

void check_defaults() {
    crammy_settings settings = default_settings();
    check( is_default_shortcut( settings ), "The default shortcut is Ctrl+Alt+C." );
    check( settings.youtonomous == L"C:\\Code\\Youtonomous\\build\\youtonomous.exe", "The default Youtonomous path is the build folder in C:\\Code." );
    check( settings.pomodoro_long == L"C:\\Users\\Justin\\Videos\\Youtonomous\\74cOUSKXMz0.mp4", "The default long video is 74cOUSKXMz0.mp4." );
    check( settings.pomodoro_short == L"C:\\Users\\Justin\\Videos\\Youtonomous\\sdhh7AYzsTY.mp4", "The default short video is sdhh7AYzsTY.mp4." );
    check( settings.start_at_sign_in, "Starting at sign in is on by default." );
}

void check_reading() {
    crammy_settings defaults = default_settings();
    vector<wstring> errors;

    crammy_settings empty = read_settings( L"", errors );
    check( same_settings( empty, defaults ), "Empty text gives the default settings." );
    check( errors.empty(), "Empty text gives no errors." );

    errors.clear();
    crammy_settings comments = read_settings( L"# a comment\n\n   \n# another\n", errors );
    check( same_settings( comments, defaults ) && errors.empty(), "Comment lines and blank lines are ignored." );

    errors.clear();
    crammy_settings full = read_settings(
        L"shortcut = Win+Shift+F9\n"
        L"youtonomous = D:\\tools\\you.exe\n"
        L"pomodoro long = D:\\v\\long.mp4\n"
        L"pomodoro short = D:\\v\\short.mp4\n"
        L"start at sign in = no\n", errors );
    check( errors.empty(), "A full valid file gives no errors." );
    check( full.shortcut_modifiers == ( MOD_WIN | MOD_SHIFT ) && full.shortcut_key == VK_F9, "The shortcut line is read." );
    check( full.youtonomous == L"D:\\tools\\you.exe", "The youtonomous line is read." );
    check( full.pomodoro_long == L"D:\\v\\long.mp4", "The pomodoro long line is read." );
    check( full.pomodoro_short == L"D:\\v\\short.mp4", "The pomodoro short line is read." );
    check( !full.start_at_sign_in, "The value no turns off starting at sign in." );

    errors.clear();
    crammy_settings yes = read_settings( L"start at sign in = yes\n", errors );
    check( yes.start_at_sign_in && errors.empty(), "The value yes turns on starting at sign in." );

    errors.clear();
    crammy_settings partial = read_settings( L"youtonomous = E:\\only.exe\n", errors );
    check( partial.youtonomous == L"E:\\only.exe", "A key that is present is read when others are missing." );
    check( partial.pomodoro_long == defaults.pomodoro_long && partial.pomodoro_short == defaults.pomodoro_short, "Missing video keys keep their defaults." );
    check( is_default_shortcut( partial ) && partial.start_at_sign_in, "A missing shortcut and a missing sign-in key keep their defaults." );
    check( errors.empty(), "Missing keys are not errors." );

    errors.clear();
    crammy_settings loose = read_settings( L"  YouTonomous   =   F:\\spaced.exe   \nPOMODORO LONG=G:\\l.mp4\n", errors );
    check( loose.youtonomous == L"F:\\spaced.exe", "Key names ignore case and spaces around the value are trimmed." );
    check( loose.pomodoro_long == L"G:\\l.mp4" && errors.empty(), "A line with no spaces around the equals sign is read." );
}

void check_read_errors() {
    crammy_settings defaults = default_settings();
    vector<wstring> errors;

    crammy_settings unknown = read_settings( L"# top\ncolour = red\n", errors );
    check( errors.size() == 1, "An unknown key adds one error." );
    check( errors.size() == 1 && starts_with( errors[0], L"line 2: " ), "The unknown key error starts with its line number." );
    check( same_settings( unknown, defaults ), "An unknown key leaves the settings at their defaults." );

    errors.clear();
    crammy_settings bad_shortcut = read_settings( L"shortcut = Banana\n", errors );
    check( errors.size() == 1 && starts_with( errors[0], L"line 1: " ), "A shortcut that does not parse adds an error for line 1." );
    check( is_default_shortcut( bad_shortcut ), "A shortcut that does not parse keeps the default shortcut." );

    errors.clear();
    crammy_settings bad_flag = read_settings( L"start at sign in = maybe\n", errors );
    check( errors.size() == 1 && starts_with( errors[0], L"line 1: " ), "A sign-in value other than yes or no adds an error for line 1." );
    check( bad_flag.start_at_sign_in, "A bad sign-in value keeps the default, which is on." );

    errors.clear();
    crammy_settings mixed = read_settings( L"youtonomous = H:\\ok.exe\nwhat = 1\nshortcut = Ctrl+\nstart at sign in = no\n", errors );
    check( errors.size() == 2, "Two bad lines in one file add two errors." );
    check( errors.size() == 2 && starts_with( errors[0], L"line 2: " ) && starts_with( errors[1], L"line 3: " ), "Each error names its own line." );
    check( mixed.youtonomous == L"H:\\ok.exe" && !mixed.start_at_sign_in, "Valid lines are still read beside bad ones." );
}

void check_parse_shortcut() {
    check( parses_as( L"Ctrl+Alt+C", MOD_CONTROL | MOD_ALT, 'C' ), "Ctrl+Alt+C parses to control and alt with the key C." );
    check( parses_as( L"ctrl + alt + c", MOD_CONTROL | MOD_ALT, 'C' ), "Lower case with spaces around the plus signs parses the same." );
    check( parses_as( L"Win+Shift+F9", MOD_WIN | MOD_SHIFT, VK_F9 ), "Win+Shift+F9 parses to win and shift with F9." );
    check( parses_as( L"Alt+1", MOD_ALT, '1' ), "Alt+1 parses to alt with the key 1." );
    check( parses_as( L"Ctrl+Space", MOD_CONTROL, VK_SPACE ), "Ctrl+Space parses to control with the space key." );
    check( parses_as( L"Ctrl+F24", MOD_CONTROL, VK_F24 ), "Ctrl+F24 parses, since F1 to F24 are keys." );
    check( parses_as( L"Shift+Z", MOD_SHIFT, 'Z' ), "Shift+Z parses to shift with the key Z." );
    check( parses_as( L"Ctrl+Alt+Shift+Win+0", MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_WIN, '0' ), "All four modifiers can be combined." );
    check( fails_to_parse( L"C" ), "A key with no modifier does not parse." );
    check( fails_to_parse( L"Ctrl+" ), "Ctrl+ with no key does not parse." );
    check( fails_to_parse( L"Alt+Foo" ), "An unknown key name does not parse." );
    check( fails_to_parse( L"Ctrl+Alt" ), "Modifiers with no key do not parse." );
}

void check_shortcut_text() {
    check( shortcut_text( MOD_CONTROL | MOD_ALT, 'C' ) == L"Ctrl+Alt+C", "Control and alt with C are written as Ctrl+Alt+C." );
    check( shortcut_text( MOD_WIN | MOD_SHIFT, VK_F9 ) == L"Shift+Win+F9", "Win and shift with F9 are written as Shift+Win+F9, in the fixed order." );
    check( shortcut_text( MOD_ALT, '1' ) == L"Alt+1", "Alt with the key 1 is written as Alt+1." );
    check( shortcut_text( MOD_CONTROL, VK_SPACE ) == L"Ctrl+Space", "Control with space is written as Ctrl+Space." );

    UINT modifiers = 0;
    UINT key = 0;
    bool parsed = parse_shortcut( shortcut_text( MOD_WIN | MOD_SHIFT, VK_F9 ), modifiers, key );
    check( parsed && modifiers == ( MOD_WIN | MOD_SHIFT ) && key == VK_F9, "Parsing the written text gives the same modifiers and key back." );

    modifiers = 0;
    key = 0;
    parsed = parse_shortcut( shortcut_text( MOD_CONTROL | MOD_ALT | MOD_SHIFT, 'Q' ), modifiers, key );
    check( parsed && modifiers == ( MOD_CONTROL | MOD_ALT | MOD_SHIFT ) && key == 'Q', "A shortcut with several modifiers survives writing and parsing." );
}

void check_round_trip() {
    vector<wstring> errors;
    crammy_settings defaults = default_settings();
    crammy_settings back = read_settings( write_settings( defaults ), errors );
    check( same_settings( back, defaults ), "The default settings survive writing and reading." );
    check( errors.empty(), "Reading the written default settings gives no errors." );

    crammy_settings changed = defaults;
    changed.shortcut_modifiers = MOD_WIN | MOD_SHIFT;
    changed.shortcut_key = VK_F9;
    changed.youtonomous = L"D:\\other place\\you.exe";
    changed.pomodoro_long = L"D:\\other place\\long.mp4";
    changed.pomodoro_short = L"D:\\other place\\short.mp4";
    changed.start_at_sign_in = false;
    errors.clear();
    back = read_settings( write_settings( changed ), errors );
    check( same_settings( back, changed ), "A changed set of settings survives writing and reading." );
    check( errors.empty(), "Reading the written changed settings gives no errors." );

    wstring text = write_settings( changed );
    check( contains_ci( text, L"shortcut" ) && contains_ci( text, L"youtonomous" ) && contains_ci( text, L"pomodoro long" )
        && contains_ci( text, L"pomodoro short" ) && contains_ci( text, L"start at sign in" ), "The written text has all five keys." );
    check( contains_ci( text, L"= no" ), "The written text holds the word no when starting at sign in is off." );
    check( contains_ci( write_settings( defaults ), L"= yes" ), "The written text holds the word yes when starting at sign in is on." );
}

void check_command() {
    wstring program = make_file( L"program.exe" );
    wstring long_video = make_file( L"long.mp4" );
    wstring short_video = make_file( L"short.mp4" );

    crammy_settings settings = default_settings();
    settings.youtonomous = program;
    settings.pomodoro_long = long_video;
    settings.pomodoro_short = short_video;

    wstring command = L"stale";
    wstring problem = L"stale";
    bool ok = youtonomous_command( settings, true, command, problem );
    check( ok, "The command is made when the program and the long video exist." );
    check( command == L"\"" + program + L"\" \"" + long_video + L"\"", "The long command is the program and the long video, each in quotes." );
    check( problem.empty(), "A made command leaves the problem empty." );

    command.clear();
    problem = L"stale";
    ok = youtonomous_command( settings, false, command, problem );
    check( ok && command == L"\"" + program + L"\" \"" + short_video + L"\"", "The short command is the program and the short video, each in quotes." );
    check( problem.empty(), "A made short command leaves the problem empty." );

    crammy_settings no_program = settings;
    no_program.youtonomous = join_path( scratch_folder(), L"missing program.exe" );
    command = L"stale";
    problem.clear();
    ok = youtonomous_command( no_program, true, command, problem );
    check( !ok, "No command is made when the program is missing." );
    check( command.empty(), "The command is empty when the program is missing." );
    check( problem == L"Youtonomous was not found at " + no_program.youtonomous, "The problem names the missing program." );

    crammy_settings no_long = settings;
    no_long.pomodoro_long = join_path( scratch_folder(), L"missing long.mp4" );
    command = L"stale";
    problem.clear();
    ok = youtonomous_command( no_long, true, command, problem );
    check( !ok && command.empty(), "No command is made when the long video is missing." );
    check( problem == L"The video was not found at " + no_long.pomodoro_long, "The problem names the missing long video." );

    command.clear();
    problem.clear();
    ok = youtonomous_command( no_long, false, command, problem );
    check( ok && problem.empty(), "A missing long video does not stop the short command." );

    crammy_settings no_short = settings;
    no_short.pomodoro_short = join_path( scratch_folder(), L"missing short.mp4" );
    command = L"stale";
    problem.clear();
    ok = youtonomous_command( no_short, false, command, problem );
    check( !ok && command.empty(), "No command is made when the short video is missing." );
    check( problem == L"The video was not found at " + no_short.pomodoro_short, "The problem names the missing short video." );

    remove_scratch();
}

void check_path() {
    check( ends_with( settings_path(), L"\\settings.txt" ), "The settings path ends with a backslash and settings.txt." );
}

}  // namespace

void test_settings() {
    check_defaults();
    check_reading();
    check_read_errors();
    check_parse_shortcut();
    check_shortcut_text();
    check_round_trip();
    check_command();
    check_path();
    remove_scratch();
}
