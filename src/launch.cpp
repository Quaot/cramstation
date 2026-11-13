#include "launch.h"

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
        }
        if ( pdfs.empty() ) continue;
        if ( first_pdf.empty() ) first_pdf = pdfs[0];

        if ( item.kind == item_kind::pick && separate ) {
            for ( const wstring &pdf : pdfs ) plan.push_back( { item.side, program::pdfcraft, { pdf } } );
        } else {
            plan.push_back( { item.side, program::pdfcraft, pdfs } );
        }
    }

    if ( plan.size() == notes_windows.size() ) {
        problems.push_back( L"Nothing to open. Tick at least one PDF." );
        return problems;
    }

    // A notes file is named after the first PDF, which is only known now
    for ( const auto &[plan_index, notes_item] : notes_windows ) {
        plan[plan_index].paths = { notes_file( chosen, *notes_item, first_pdf ) };
    }

    vector<wstring> window_problems = open_windows( plan );
    problems.insert( problems.end(), window_problems.begin(), window_problems.end() );
    return problems;
}
