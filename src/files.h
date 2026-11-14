#pragma once

#include "profiles.h"

struct pdf_file {
    wstring path;
    wstring name;
    wstring folder;                 // name of the folder it is in
    unsigned long long modified;
};

// Every PDF under these paths. Folders are searched all the way down.
vector<pdf_file> find_pdfs( const vector<wstring> &paths, bool newest_first );
bool is_answer_key( const wstring &name );

// "ECE190-W10-Slides.pdf" becomes "ECE190-W*-Slides.pdf", or "" when the name has no week or lecture number.
wstring newest_pattern( const wstring &path );

// The path of the text file Notepad++ opens for a "notes" window. Creates nothing.
wstring notes_file_name( const setup &chosen, const layout_item &notes_item, const wstring &first_pdf );

// Writes the starting text of a notes file, unless the file exists.
void create_notes_file( const wstring &path );

// notes_file_name, then create_notes_file.
wstring notes_file( const setup &chosen, const layout_item &notes_item, const wstring &first_pdf );
