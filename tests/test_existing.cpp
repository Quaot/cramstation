// Written from tests\specs\existing.md.

#include <windows.h>
#include <ole2.h>
#include <shlobj.h>

#include <cstdio>
#include <filesystem>

#include "check.h"
#include "../src/canvas.h"
#include "../src/files.h"
#include "../src/launch.h"
#include "../src/profiles.h"
#include "../src/text.h"
#include "../src/tiling.h"

namespace {

namespace fs = std::filesystem;

wstring temp_root;

wstring today() {
    SYSTEMTIME now;
    GetLocalTime( &now );
    wchar_t text[16];
    swprintf( text, 16, L"%04d-%02d-%02d", int( now.wYear ), int( now.wMonth ), int( now.wDay ) );
    return text;
}

void make_folder( const wstring &path ) {
    std::error_code ignored;
    fs::create_directories( fs::path( path ), ignored );
}

void make_file( const wstring &path ) {
    make_folder( parent_folder( path ) );
    write_text_file( path, L"x\r\n" );
}

// Sets the modified time to a number of seconds before now
void set_age( const wstring &path, long long seconds_ago ) {
    FILETIME now;
    GetSystemTimeAsFileTime( &now );
    ULARGE_INTEGER stamp;
    stamp.LowPart = now.dwLowDateTime;
    stamp.HighPart = now.dwHighDateTime;
    stamp.QuadPart -= static_cast<unsigned long long>( seconds_ago ) * 10000000ULL;
    FILETIME moved = { stamp.LowPart, stamp.HighPart };
    HANDLE file = CreateFileW( path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( file != INVALID_HANDLE_VALUE ) {
        SetFileTime( file, nullptr, nullptr, &moved );
        CloseHandle( file );
    }
}

int count_entries( const wstring &folder ) {
    std::error_code ignored;
    int count = 0;
    for ( auto it = fs::recursive_directory_iterator( fs::path( folder ), ignored ); it != fs::recursive_directory_iterator();
          it.increment( ignored ) )
        ++count;
    return count;
}

bool same_item( const written_item &first, const written_item &second ) {
    return first.side == second.side && first.kind == second.kind && first.paths == second.paths && first.app == second.app;
}

bool same_items( const vector<written_item> &first, const vector<written_item> &second ) {
    if ( first.size() != second.size() ) return false;
    for ( size_t at = 0; at < first.size(); ++at )
        if ( !same_item( first[at], second[at] ) ) return false;
    return true;
}

bool same_books( const profile_book &first, const profile_book &second ) {
    if ( first.profiles.size() != second.profiles.size() ) return false;
    for ( size_t at = 0; at < first.profiles.size(); ++at ) {
        const profile_block &one = first.profiles[at], &two = second.profiles[at];
        if ( one.name != two.name || one.root != two.root || one.desktop != two.desktop ||
             one.notes_folder != two.notes_folder || !same_items( one.items, two.items ) ||
             one.setups.size() != two.setups.size() )
            return false;
        for ( size_t each = 0; each < one.setups.size(); ++each ) {
            const setup_block &left = one.setups[each], &right = two.setups[each];
            if ( left.name != right.name || left.desktop != right.desktop || left.notes_folder != right.notes_folder ||
                 !same_items( left.items, right.items ) )
                return false;
        }
    }
    return true;
}

layout_item item_of( const wchar_t *side, item_kind kind, vector<wstring> paths, viewer app = viewer::pdfcraft ) {
    return layout_item{ side, kind, std::move( paths ), app };
}

const wchar_t *profile_a =
    L"[ECE 150]\n"
    L"root = %TEMP%\\crammy existing checks\\ECE 150\n"
    L"desktop = 150\n"
    L"\n"
    L"[ECE 150: Exam]\n"
    L"left = notes\n"
    L"right = pick \"6 Exams\" \"2 Practice\"\n"
    L"\n"
    L"[ECE 150: Lecture]\n"
    L"full = newest \"1 Lectures\\PDF\\ECE150-L*.pdf\" with acrobat\n";

void check_reading() {
    profile_book book = read_book( profile_a );
    check( book.errors.empty(), "profile text A reads without errors" );
    check( book.profiles.size() == 1 && book.profiles[0].name == L"ECE 150", "profile text A has one subject named ECE 150" );
    if ( book.profiles.size() != 1 ) return;
    const profile_block &subject = book.profiles[0];
    check( subject.root == L"%TEMP%\\crammy existing checks\\ECE 150", "the subject root is kept as written" );
    check( subject.desktop == L"150", "the subject desktop is 150" );
    check( subject.setups.size() == 2 && subject.setups[0].name == L"Exam" && subject.setups[1].name == L"Lecture",
           "the subject has the setups Exam and Lecture in order" );
    if ( subject.setups.size() != 2 ) return;

    const vector<written_item> &exam = subject.setups[0].items;
    check( exam.size() == 2, "Exam has two items" );
    if ( exam.size() == 2 ) {
        check( exam[0].side == L"left" && exam[0].kind == item_kind::notes && exam[0].paths.empty(),
               "the first Exam item is a left notes window with no paths" );
        check( exam[1].side == L"right" && exam[1].kind == item_kind::pick && exam[1].paths.size() == 2 &&
                   exam[1].paths[0] == L"6 Exams" && exam[1].paths[1] == L"2 Practice",
               "the second Exam item is a right pick of 6 Exams then 2 Practice" );
        check( exam[1].app == viewer::pdfcraft, "an item without a viewer word uses PdfCraft" );
    }
    const vector<written_item> &lecture = subject.setups[1].items;
    check( lecture.size() == 1 && lecture[0].side == L"full" && lecture[0].kind == item_kind::newest &&
               lecture[0].paths.size() == 1 && lecture[0].paths[0] == L"1 Lectures\\PDF\\ECE150-L*.pdf" &&
               lecture[0].app == viewer::acrobat,
           "Lecture has one full newest item with its pattern, opened in Acrobat" );
}

void check_all_setups() {
    wstring root = expand_variables( L"%TEMP%\\crammy existing checks\\ECE 150" );
    make_folder( root );
    vector<setup> setups = all_setups( read_book( profile_a ) );
    check( setups.size() == 2 && setups[0].title == L"ECE 150: Exam" && setups[1].title == L"ECE 150: Lecture",
           "all_setups gives ECE 150: Exam then ECE 150: Lecture" );
    if ( setups.size() != 2 ) return;
    check( setups[0].desktop == L"150" && setups[1].desktop == L"150", "both setups are on desktop 150" );
    check( setups[0].root == root && setups[1].root == root, "the setup root is the expanded folder" );
    check( setups[0].items.size() == 2 && setups[0].items[1].paths.size() == 2 &&
               setups[0].items[1].paths[0] == root + L"\\6 Exams" && setups[0].items[1].paths[1] == root + L"\\2 Practice",
           "pick paths are joined to the root" );
    check( setups[0].notes_folder == root, "the notes folder is the root when 2 Practice does not exist" );

    make_folder( root + L"\\2 Practice" );
    setups = all_setups( read_book( profile_a ) );
    check( setups.size() == 2 && setups[0].notes_folder == root + L"\\2 Practice",
           "the notes folder is 2 Practice when that folder exists" );
    std::error_code ignored;
    fs::remove_all( fs::path( root + L"\\2 Practice" ), ignored );
    setups = all_setups( read_book( profile_a ) );
    check( setups.size() == 2 && setups[0].notes_folder == root, "the notes folder goes back to the root once 2 Practice is removed" );

    vector<setup> bare = all_setups( read_book( L"[ECE 198]\nroot = C:\\Courses\\ECE 198\nfull = pick \".\"\n" ) );
    check( bare.size() == 1 && bare[0].title == L"ECE 198" && bare[0].setup_index == -1,
           "a subject with no setups is one setup with its own name and index -1" );
    check( bare.size() == 1 && bare[0].items.size() == 1 && bare[0].items[0].paths.size() == 1 &&
               bare[0].items[0].paths[0] == L"C:\\Courses\\ECE 198",
           "a dot path means the root itself" );

    vector<setup> shared = all_setups(
        read_book( L"[Math 117]\nroot = C:\\Courses\\Math 117\nleft = notes\n\n[Math 117: Exam]\nright = pdf \"a.pdf\"\n" ) );
    check( shared.size() == 1 && shared[0].title == L"Math 117: Exam" && shared[0].items.size() == 2 &&
               shared[0].items[0].kind == item_kind::notes && shared[0].items[1].kind == item_kind::pdf,
           "shared lines come first and the setup's own lines after" );

    profile_book plain = read_book( L"[X]\nroot = C:\\X\nleft = folder 6 Exams\n" );
    check( plain.errors.empty() && plain.profiles.size() == 1 && plain.profiles[0].items.size() == 1 &&
               plain.profiles[0].items[0].paths.size() == 1 && plain.profiles[0].items[0].paths[0] == L"6 Exams",
           "an unquoted path is read as one path" );

    vector<setup> own = all_setups(
        read_book( L"[X]\nroot = C:\\X\ndesktop = 1\n\n[X: A]\ndesktop = 117\nleft = notes\n\n[X: B]\nleft = notes\n" ) );
    check( own.size() == 2 && own[0].desktop == L"117" && own[1].desktop == L"1",
           "a setup's own desktop applies to that setup only" );
}

void check_errors() {
    struct bad_case {
        const wchar_t *line;
        const char *what;
    };
    const bad_case cases[] = {
        { L"left = magic \"x\"", "an unknown window kind gives a line 3 error" },
        { L"size = 5", "an unknown setting gives a line 3 error" },
        { L"[Broken", "a heading without a closing bracket gives a line 3 error" },
        { L"left = pdf", "a pdf line with no path gives a line 3 error" },
        { L"left = pdf \"a.pdf\" \"b.pdf\"", "a pdf line with two paths gives a line 3 error" },
    };
    for ( const bad_case &each : cases ) {
        profile_book book = read_book( wstring( L"[X]\n\n" ) + each.line + L"\n" );
        check( book.errors.size() == 1 && starts_with( book.errors[0], L"line 3: " ), each.what );
    }
    profile_book after = read_book( L"[X]\n\nleft = magic \"x\"\nright = notes\n" );
    check( after.errors.size() == 1 && after.profiles.size() == 1 && after.profiles[0].items.size() == 1,
           "reading goes on after a bad line and skips only that line" );
    profile_book early = read_book( L"left = notes\n" );
    check( !early.errors.empty() && starts_with( early.errors[0], L"line 1: " ),
           "a window line before any heading gives a line 1 error" );
}

void check_writing() {
    profile_book first = read_book( profile_a );
    wstring written = write_book( first );
    check( starts_with( written, L"# Crammy profiles. The Crammy window writes this file, and you can also edit it by hand." ),
           "the written file starts with the header line" );
    profile_book second = read_book( written );
    check( second.errors.empty() && same_books( first, second ), "writing and reading again keeps every subject, setup and item" );
    check( write_book( second ) == written, "writing the second book gives exactly the same text" );

    profile_book acrobat = read_book( L"[X]\nroot = C:\\X\nleft = pdf \"a.pdf\" with acrobat\n" );
    wstring text = write_book( acrobat );
    size_t at = text.find( L"left = pdf" );
    size_t end = at == wstring::npos ? at : text.find_first_of( L"\r\n", at );
    wstring line = at == wstring::npos ? L"" : text.substr( at, end - at );
    check( ends_with( line, L" with acrobat" ), "a pdf item opened in Acrobat is written with 'with acrobat' at the end of its line" );
}

void check_paths() {
    check( relative_to( L"C:\\A\\B", L"C:\\A\\B\\c\\d.pdf" ) == L"c\\d.pdf", "a path inside the root is made relative" );
    check( relative_to( L"C:\\A\\B", L"c:\\a\\b\\X.pdf" ) == L"X.pdf", "the root comparison ignores case" );
    check( relative_to( L"C:\\A\\B", L"D:\\e.pdf" ) == L"D:\\e.pdf", "a path outside the root comes back unchanged" );
    check( relative_to( L"C:\\A\\B", L"C:\\A\\B" ) == L".", "the root itself gives a dot" );

    profile_file file;
    file.setups = all_setups( read_book( profile_a ) );
    const setup *exam = find_setup( file, L"ece 150: exam" );
    check( exam && exam->title == L"ECE 150: Exam", "find_setup matches a full title in any case" );
    const setup *lecture = find_setup( file, L"Lecture" );
    check( lecture && lecture->title == L"ECE 150: Lecture", "find_setup matches a unique partial name" );
    check( find_setup( file, L"ECE 150" ) == nullptr, "find_setup returns null when the name matches two setups" );
    check( find_setup( file, L"Biology" ) == nullptr, "find_setup returns null when nothing matches" );
}

void check_patterns() {
    check( newest_pattern( L"ECE190-W10-Slides.pdf" ) == L"ECE190-W*-Slides.pdf", "a week number becomes a star" );
    check( newest_pattern( L"PDF\\MATH117-L23.pdf" ) == L"PDF\\MATH117-L*.pdf", "a lecture number becomes a star and the folder stays" );
    check( newest_pattern( L"Lec_23_M117.pdf" ) == L"Lec_*_M117.pdf", "an underscored number becomes a star" );
    check( newest_pattern( L"MATH115-W07-Shell.pdf" ) == L"MATH115-W*-Shell.pdf", "a number with a leading zero becomes a star" );
    check( newest_pattern( L"Formula sheet.pdf" ).empty(), "a name without a number has no pattern" );
    check( newest_pattern( L"ECE150-Mock-1112a.pdf" ).empty(), "a run of four digits is kept so the name has no pattern" );
}

void check_keys() {
    for ( const wchar_t *name : { L"ECE150-Mock-1112a-S.pdf", L"F25-M-S.pdf", L"F25-M-S (answer key, rebuilt).pdf",
                                  L"Week 3 solutions.pdf", L"Practice exam - ANSWER KEY.pdf" } )
        check( is_answer_key( name ), ( "this is an answer key: " + to_utf8( name ) ).c_str() );
    for ( const wchar_t *name : { L"ECE190-W10-Slides.pdf", L"ECE190-W10-Sum.pdf", L"F25-M.pdf", L"ECE150-Mock-1112a.pdf" } )
        check( !is_answer_key( name ), ( "this is not an answer key: " + to_utf8( name ) ).c_str() );
}

bool has_names( const vector<pdf_file> &found, const vector<wstring> &names ) {
    if ( found.size() != names.size() ) return false;
    for ( size_t at = 0; at < names.size(); ++at )
        if ( found[at].name != names[at] ) return false;
    return true;
}

void check_find_pdfs() {
    wstring tree = temp_root + L"\\tree";
    for ( const wchar_t *name : { L"a.pdf", L"x.txt", L"sub\\b.pdf", L"sub\\deeper\\c.pdf", L"build\\skip1.pdf",
                                  L"Source\\skip2.pdf", L".hidden\\skip3.pdf" } )
        make_file( tree + L"\\" + name );
    set_age( tree + L"\\a.pdf", 7200 );
    set_age( tree + L"\\sub\\b.pdf", 3600 );
    set_age( tree + L"\\sub\\deeper\\c.pdf", 10800 );

    vector<pdf_file> by_name = find_pdfs( { tree }, false );
    check( has_names( by_name, { L"a.pdf", L"b.pdf", L"c.pdf" } ),
           "find_pdfs finds a, b and c sorted by name and skips build, Source, dot folders and text files" );
    vector<pdf_file> by_date = find_pdfs( { tree }, true );
    check( has_names( by_date, { L"b.pdf", L"a.pdf", L"c.pdf" } ), "find_pdfs with newest first gives b, a, c" );
    check( by_name.size() == 3 && by_name[0].folder == L"tree" && by_name[1].folder == L"sub",
           "each result names the folder it is in" );
    check( has_names( find_pdfs( { tree + L"\\*.pdf" }, false ), { L"a.pdf" } ), "a pattern only searches its own folder" );
    vector<pdf_file> single = find_pdfs( { tree + L"\\sub\\b.pdf" }, false );
    check( single.size() == 1 && single[0].path == tree + L"\\sub\\b.pdf", "a single file path gives that file" );
    check( find_pdfs( { tree, tree }, false ).size() == 3, "the same folder given twice gives each file once" );
}

void check_notes_names() {
    wstring notes = temp_root + L"\\notes";
    make_folder( notes );
    setup chosen;
    chosen.title = L"Test";
    chosen.notes_folder = notes;
    layout_item notes_item = item_of( L"left", item_kind::notes, {} );
    wstring date = today();

    check( notes_file_name( chosen, notes_item, L"C:\\x\\F26-M (blank, for redo).pdf" ) ==
               notes + L"\\F26-M - my answers (" + date + L").txt",
           "a bracketed note after the paper name is dropped from the notes file name" );
    check( notes_file_name( chosen, notes_item, L"C:\\x\\F25-M (mock version).pdf" ) ==
               notes + L"\\F25-M - my answers (" + date + L").txt",
           "a mock version bracket is dropped from the notes file name" );
    check( notes_file_name( chosen, notes_item, L"" ) == notes + L"\\Notes - my answers (" + date + L").txt",
           "with no first PDF the notes file is called Notes" );

    wstring newer = notes + L"\\F26-M - my answers (2026-11-10).txt";
    wstring older = notes + L"\\F26-M - my answers (2026-11-12).txt";
    make_file( older );
    set_age( older, 7200 );
    check( notes_file_name( chosen, notes_item, L"C:\\x\\F26-M.pdf" ) == older,
           "an existing notes file for the paper is returned in place of a new one" );
    make_file( newer );
    set_age( newer, 60 );
    int before = count_entries( notes );
    check( notes_file_name( chosen, notes_item, L"C:\\x\\F26-M.pdf" ) == newer,
           "with two existing notes files the one modified last is returned" );
    notes_file_name( chosen, notes_item, L"C:\\x\\Other.pdf" );
    check( count_entries( notes ) == before, "notes_file_name creates no file" );
    layout_item with_path = item_of( L"left", item_kind::notes, { L"D:\\somewhere\\mine.txt" } );
    check( notes_file_name( chosen, with_path, L"C:\\x\\F26-M.pdf" ) == L"D:\\somewhere\\mine.txt",
           "a notes item with a path returns that path as it is" );
}

bool has_problem( const vector<wstring> &problems, const wstring &text ) {
    for ( const wstring &each : problems )
        if ( each == text ) return true;
    return false;
}

bool window_is( const planned_window &window, const wchar_t *side, program app, const vector<wstring> &paths ) {
    return window.side == side && window.app == app && window.paths == paths;
}

void check_plan() {
    wstring plan = temp_root + L"\\plan";
    wstring one = plan + L"\\one.pdf", two = plan + L"\\two.pdf", labs = plan + L"\\Labs";
    make_file( one );
    make_file( two );
    make_folder( labs );
    set_age( one, 7200 );
    set_age( two, 3600 );
    wstring date = today();
    int before = count_entries( plan );

    auto make_setup = [&]( vector<layout_item> items ) {
        setup made;
        made.title = L"Test";
        made.notes_folder = plan;
        made.root = plan;
        made.items = std::move( items );
        return made;
    };
    auto plan_of = [&]( const setup &chosen, vector<vector<wstring>> picks, bool separate, vector<wstring> &problems ) {
        picks.resize( chosen.items.size() );
        return plan_setup( chosen, picks, separate, problems );
    };

    {
        vector<wstring> problems;
        setup chosen = make_setup( { item_of( L"left", item_kind::notes, {} ), item_of( L"right", item_kind::pdf, { one } ) } );
        vector<planned_window> windows = plan_of( chosen, {}, false, problems );
        check( windows.size() == 2, "notes and a PDF give two windows" );
        if ( windows.size() == 2 ) {
            check( window_is( windows[0], L"left", program::notepad_pp, { plan + L"\\one - my answers (" + date + L").txt" } ),
                   "the notes window is left, in Notepad++, named after the PDF" );
            check( window_is( windows[1], L"right", program::pdfcraft, { one } ), "the PDF window is right, in PdfCraft" );
        }
        check( problems.empty(), "notes and a PDF give no problems" );
    }
    {
        vector<wstring> problems;
        setup chosen = make_setup( { item_of( L"right", item_kind::tabs, { one, two } ) } );
        vector<planned_window> windows = plan_of( chosen, {}, false, problems );
        check( windows.size() == 1 && window_is( windows[0], L"right", program::pdfcraft, { one, two } ),
               "a tabs item gives one PdfCraft window with both PDFs" );
    }
    {
        vector<wstring> problems;
        setup chosen = make_setup( { item_of( L"right", item_kind::pdf, { one }, viewer::acrobat ) } );
        vector<planned_window> windows = plan_of( chosen, {}, false, problems );
        check( windows.size() == 1 && window_is( windows[0], L"right", program::acrobat, { one } ),
               "a PDF item in Acrobat gives one Acrobat window" );
        setup tabs = make_setup( { item_of( L"right", item_kind::tabs, { one, two }, viewer::acrobat ) } );
        windows = plan_of( tabs, {}, false, problems );
        check( windows.size() == 2 && window_is( windows[0], L"right", program::acrobat, { one } ) &&
                   window_is( windows[1], L"right", program::acrobat, { two } ),
               "a tabs item in Acrobat gives one Acrobat window per PDF" );
    }
    {
        vector<wstring> problems;
        setup chosen = make_setup( { item_of( L"right", item_kind::pick, { plan } ) } );
        vector<planned_window> windows = plan_of( chosen, { { one, two } }, false, problems );
        check( windows.size() == 1 && window_is( windows[0], L"right", program::pdfcraft, { one, two } ),
               "ticked PDFs of a pick item open in one PdfCraft window" );
        windows = plan_of( chosen, { { one, two } }, true, problems );
        check( windows.size() == 2 && window_is( windows[0], L"right", program::pdfcraft, { one } ) &&
                   window_is( windows[1], L"right", program::pdfcraft, { two } ),
               "ticked PDFs open in separate windows when separate is set" );
        windows = plan_of( chosen, { {} }, false, problems );
        check( windows.empty(), "a pick item with nothing ticked gives no window" );
    }
    {
        vector<wstring> problems;
        setup chosen = make_setup( { item_of( L"left", item_kind::folder, { labs } ) } );
        vector<planned_window> windows = plan_of( chosen, {}, false, problems );
        check( windows.size() == 1 && window_is( windows[0], L"left", program::explorer, { labs } ),
               "a folder item gives an Explorer window with that path" );
    }
    {
        vector<wstring> problems;
        wstring missing = plan + L"\\missing.pdf";
        setup chosen = make_setup( { item_of( L"left", item_kind::pdf, { missing } ) } );
        vector<planned_window> windows = plan_of( chosen, {}, false, problems );
        check( windows.empty() && has_problem( problems, L"PDF not found: " + missing ),
               "a missing PDF gives no window and a PDF not found problem" );
        vector<wstring> folder_problems;
        setup folder = make_setup( { item_of( L"left", item_kind::folder, { plan + L"\\NoSuchFolder" } ) } );
        windows = plan_of( folder, {}, false, folder_problems );
        bool found = false;
        for ( const wstring &each : folder_problems ) found = found || starts_with( each, L"Folder not found: " );
        check( windows.empty() && found, "a missing folder gives a Folder not found problem" );
    }
    {
        vector<wstring> problems;
        setup chosen = make_setup( { item_of( L"right", item_kind::newest, { plan + L"\\*.pdf" } ) } );
        set_age( one, 5 * 86400 );
        set_age( two, 4 * 86400 );
        vector<planned_window> windows = plan_of( chosen, {}, false, problems );
        check( windows.size() == 1 && window_is( windows[0], L"right", program::pdfcraft, { two } ),
               "a newest item opens the PDF modified last" );
        int matches = 0;
        for ( const wstring &each : problems ) matches += contains_ci( each, L"may not be downloaded yet" ) ? 1 : 0;
        check( problems.size() == 1 && matches == 1,
               "a newest PDF from an earlier day gives one 'may not be downloaded yet' problem" );

        set_age( two, 60 );
        problems.clear();
        windows = plan_of( chosen, {}, false, problems );
        check( windows.size() == 1 && problems.empty(), "a newest PDF modified today gives no problems" );
    }
    {
        vector<wstring> problems;
        setup chosen = make_setup( { item_of( L"left", item_kind::notes, {} ) } );
        vector<planned_window> windows = plan_of( chosen, {}, false, problems );
        check( windows.empty() && problems.size() == 1 && problems[0] == L"Nothing to open. Tick at least one PDF.",
               "a setup with only notes plans nothing and says to tick a PDF" );
    }
    {
        vector<wstring> problems;
        setup chosen = make_setup( { item_of( L"left", item_kind::notes, {} ), item_of( L"right", item_kind::pick, { plan } ) } );
        vector<planned_window> windows = plan_of( chosen, { {}, { two } }, false, problems );
        check( windows.size() == 2 && windows[0].paths.size() == 1 &&
                   windows[0].paths[0] == plan + L"\\two - my answers (" + date + L").txt",
               "the notes file is named after the first PDF opened, including picked ones" );
    }
    check( count_entries( plan ) == before, "plan_setup creates no file" );
}

IDataObject *files_object( const vector<wstring> &paths ) {
    wstring list;
    for ( const wstring &path : paths ) list += path + L'\0';
    list += L'\0';
    size_t bytes = sizeof( DROPFILES ) + list.size() * sizeof( wchar_t );
    HGLOBAL memory = GlobalAlloc( GHND, bytes );
    auto *drop = static_cast<DROPFILES *>( GlobalLock( memory ) );
    drop->pFiles = sizeof( DROPFILES );
    drop->fWide = TRUE;
    memcpy( reinterpret_cast<BYTE *>( drop ) + sizeof( DROPFILES ), list.data(), list.size() * sizeof( wchar_t ) );
    GlobalUnlock( memory );

    IDataObject *data = nullptr;
    SHCreateDataObject( nullptr, 0, nullptr, nullptr, IID_PPV_ARGS( &data ) );
    FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium{};
    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = memory;
    data->SetData( &format, &medium, TRUE );
    return data;
}

void drop_at( HWND canvas, const vector<wstring> &paths, double across, double down ) {
    RECT client;
    GetClientRect( canvas, &client );
    POINT point = { LONG( client.right * across ), LONG( client.bottom * down ) };
    ClientToScreen( canvas, &point );
    POINTL at = { point.x, point.y };
    IDataObject *data = files_object( paths );
    DWORD effect = DROPEFFECT_COPY;
    IDropTarget *target = canvas_drop_target();
    target->DragEnter( data, MK_LBUTTON, at, &effect );
    target->DragOver( MK_LBUTTON, at, &effect );
    target->Drop( data, MK_LBUTTON, at, &effect );
    data->Release();
}

void check_drops( HWND canvas ) {
    wstring plan = temp_root + L"\\plan";
    wstring one = plan + L"\\one.pdf", two = plan + L"\\two.pdf", labs = plan + L"\\Labs";
    wstring note = temp_root + L"\\drop note.txt", paper = temp_root + L"\\drop paper.docx";
    make_file( one );
    make_file( two );
    make_folder( labs );
    make_file( note );
    make_file( paper );

    const struct {
        double across;
        const wchar_t *side;
        const char *what;
    } thirds[] = {
        { 0.1, L"left", "on an empty setup the left third gives the left side" },
        { 0.5, L"full", "on an empty setup the middle third gives the whole screen" },
        { 0.9, L"right", "on an empty setup the right third gives the right side" },
    };
    for ( const auto &each : thirds ) {
        vector<written_item> items;
        show_items( &items, plan );
        drop_at( canvas, { one }, each.across, 0.5 );
        check( items.size() == 1 && items[0].side == each.side, each.what );
    }

    {
        vector<written_item> items = { { L"left", item_kind::pdf, { L"one.pdf" } } };
        show_items( &items, plan );
        drop_at( canvas, { two }, 0.4, 0.5 );
        check( items.size() == 2 && items[1].side == L"left", "with a left window already there, x 0.4 gives the left half" );
    }
    {
        vector<written_item> items = { { L"left", item_kind::pdf, { L"one.pdf" } } };
        show_items( &items, plan );
        drop_at( canvas, { two }, 0.6, 0.5 );
        check( items.size() == 2 && items[1].side == L"right", "with a left window already there, x 0.6 gives the right half" );
    }
    {
        vector<written_item> items = { { L"right", item_kind::pdf, { L"one.pdf" } } };
        show_items( &items, plan );
        drop_at( canvas, { two }, 0.4, 0.5 );
        check( items.size() == 2 && items[1].side == L"left", "with a right window already there, x 0.4 gives the left half" );
    }
    {
        vector<written_item> items = { { L"full", item_kind::pdf, { L"one.pdf" } } };
        show_items( &items, plan );
        drop_at( canvas, { two }, 0.1, 0.5 );
        check( items.size() == 2 && items[1].side == L"left" && items[0].side == L"right",
               "a drop on the left moves the whole-screen window to the right" );
    }
    {
        vector<written_item> items = { { L"full", item_kind::pdf, { L"one.pdf" } } };
        show_items( &items, plan );
        drop_at( canvas, { two }, 0.9, 0.5 );
        check( items.size() == 2 && items[1].side == L"right" && items[0].side == L"left",
               "a drop on the right moves the whole-screen window to the left" );
    }
    {
        vector<written_item> items;
        show_items( &items, plan );
        drop_at( canvas, { labs }, 0.5, 0.5 );
        check( items.size() == 1 && items[0].kind == item_kind::pick && items[0].paths.size() == 1 &&
                   items[0].paths[0] == L"Labs",
               "a dropped folder adds a pick item with its path relative to the root" );
    }
    {
        vector<written_item> items;
        show_items( &items, plan );
        drop_at( canvas, { one }, 0.5, 0.5 );
        check( items.size() == 1 && items[0].kind == item_kind::pdf && items[0].paths.size() == 1,
               "one dropped PDF adds a pdf item" );
        check( items.size() == 1 && items[0].paths.size() == 1 && items[0].paths[0] == L"one.pdf",
               "a dropped PDF inside the root is stored relative" );
    }
    {
        vector<written_item> items;
        show_items( &items, plan );
        drop_at( canvas, { one, two }, 0.5, 0.5 );
        check( items.size() == 1 && items[0].kind == item_kind::tabs && items[0].paths.size() == 2,
               "two PDFs dropped together add one tabs item with both paths" );
    }
    {
        vector<written_item> items;
        show_items( &items, plan );
        drop_at( canvas, { note }, 0.5, 0.5 );
        check( items.size() == 1 && items[0].kind == item_kind::notes && items[0].paths.size() == 1 && items[0].paths[0] == note,
               "a dropped text file adds a notes item with that path" );
    }
    {
        vector<written_item> items;
        show_items( &items, plan );
        drop_at( canvas, { paper }, 0.5, 0.5 );
        check( items.empty(), "a dropped .docx adds nothing" );
    }
    show_items( nullptr, L"" );
    drop_at( canvas, { one }, 0.5, 0.5 );
    check( true, "a drop with no setup chosen does nothing and does not crash" );
}

}  // namespace

void test_existing( HWND canvas ) {
    temp_root = join_path( expand_variables( L"%TEMP%" ), L"crammy existing checks" );
    std::error_code ignored;
    fs::remove_all( fs::path( temp_root ), ignored );
    make_folder( temp_root );

    check_reading();
    check_all_setups();
    check_errors();
    check_writing();
    check_paths();
    check_patterns();
    check_keys();
    check_find_pdfs();
    check_notes_names();
    check_plan();
    check_drops( canvas );

    show_items( nullptr, L"" );
    fs::remove_all( fs::path( temp_root ), ignored );
}
