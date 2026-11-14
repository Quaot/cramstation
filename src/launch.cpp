#include "launch.h"

#include "desktops.h"
#include "notes_tabs.h"
#include "tiling.h"

vector<wstring> open_setup( const setup &chosen, const vector<vector<wstring>> &picks, bool separate ) {
    vector<planned_window> plan;
    vector<wstring> problems;
    wstring first_pdf;
    vector<std::pair<size_t, const layout_item *>> notes_windows;

    for ( size_t i = 0; i < chosen.items.size(); ++i ) {
        const layout_item &item = chosen.items[i];
        vector<wstring> pdfs;

        switch ( item.kind ) {
        case item_kind::notes:
            notes_windows.push_back( { plan.size(), &item } );
            plan.push_back( { item.side, program::notepad, {} } );
            continue;
        case item_kind::folder:
            if ( is_folder( item.paths[0] ) ) plan.push_back( { item.side, program::explorer, item.paths } );
            else problems.push_back( L"Folder not found: " + item.paths[0] );
            continue;
        case item_kind::pdf:
            if ( is_file( item.paths[0] ) ) pdfs = item.paths;
            else problems.push_back( L"PDF not found: " + item.paths[0] );
            break;
        case item_kind::tabs:
            for ( const pdf_file &pdf : find_pdfs( item.paths, false ) ) pdfs.push_back( pdf.path );
            if ( pdfs.empty() ) problems.push_back( L"No PDFs found for the tabs on the " + item.side );
            break;
        case item_kind::pick:
            if ( i < picks.size() ) pdfs = picks[i];
            break;
        case item_kind::newest: {
            vector<pdf_file> matches = find_pdfs( item.paths, true );
            if ( matches.empty() ) {
                problems.push_back( L"No PDF fits " + file_name( item.paths[0] ) );
                break;
            }
            pdfs = { matches[0].path };
            if ( date_text( matches[0].modified ) != today_text() ) {
                problems.push_back( L"The newest " + file_name( item.paths[0] ) + L" is " + matches[0].name +
                                    L", from " + date_text( matches[0].modified ) + L". Today's may not be downloaded yet." );
            }
            break;
        }
        }
        if ( pdfs.empty() ) continue;
        if ( first_pdf.empty() ) first_pdf = pdfs[0];

        // Acrobat gets a window per PDF. PdfCraft shows several as tabs unless asked to separate them.
        bool one_window_each = item.app == viewer::acrobat || ( item.kind == item_kind::pick && separate );
        program app = item.app == viewer::acrobat ? program::acrobat : program::pdfcraft;
        if ( one_window_each ) {
            for ( const wstring &pdf : pdfs ) plan.push_back( { item.side, app, { pdf } } );
        } else {
            plan.push_back( { item.side, app, pdfs } );
        }
    }

    if ( plan.size() == notes_windows.size() ) {
        problems.push_back( L"Nothing to open. Tick at least one PDF." );
        return problems;
    }

    // A notes file is named after the first PDF, which is only known now
    vector<wstring> notes_paths;
    for ( const auto &[plan_index, notes_item] : notes_windows ) {
        plan[plan_index].paths = { notes_file( chosen, *notes_item, first_pdf ) };
        notes_paths.push_back( plan[plan_index].paths[0] );
    }

    // Save and close the notes tabs from earlier setups, so Notepad does not fill up with them
    close_old_notes( notes_paths );
    for ( const wstring &path : notes_paths ) remember_notes( path );

    if ( !chosen.desktop.empty() ) {
        wstring problem = switch_to_desktop( chosen.desktop );
        if ( !problem.empty() ) problems.push_back( problem );
    }

    vector<wstring> window_problems = open_windows( plan );
    problems.insert( problems.end(), window_problems.begin(), window_problems.end() );
    return problems;
}
