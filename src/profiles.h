#pragma once

#include "text.h"

enum class item_kind { notes, pdf, tabs, pick, folder, newest };
enum class viewer { pdfcraft, acrobat };

// One window line as written in profiles.txt. Paths stay as written.
struct written_item {
    wstring side;           // left, right or full
    item_kind kind;
    vector<wstring> paths;
    viewer app = viewer::pdfcraft;
};

struct setup_block {
    wstring name;
    wstring notes_folder;
    wstring desktop;
    vector<written_item> items;
};

// A subject, such as ECE 150, and its setups.
struct profile_block {
    wstring name;
    wstring root;
    wstring notes_folder;
    wstring desktop;
    vector<written_item> items;     // shared by every setup
    vector<setup_block> setups;
};

struct profile_book {
    vector<profile_block> profiles;
    vector<wstring> errors;         // "line 12: ..."
};

// A setup with paths joined to its root, ready to open.
struct layout_item {
    wstring side;
    item_kind kind;
    vector<wstring> paths;
    viewer app;
};

struct setup {
    wstring title;                  // "ECE 150: Exam", or just "ECE 198"
    size_t profile_index = 0;
    int setup_index = -1;           // -1 when the profile has no setups and opens as it is
    wstring root;
    wstring notes_folder;
    wstring desktop;
    vector<layout_item> items;
};

struct profile_file {
    vector<setup> setups;
    vector<wstring> errors;
};

wstring profiles_path();
profile_book read_book( const wstring &text );
profile_book load_book();
bool save_book( const profile_book &book );
wstring write_book( const profile_book &book );

vector<setup> all_setups( const profile_book &book );
profile_file load_profiles();
const setup *find_setup( const profile_file &profiles, const wstring &title );

wstring root_of( const profile_block &profile );
wstring relative_to( const wstring &root, const wstring &path );
vector<written_item> &items_of( profile_book &book, size_t profile_index, int setup_index );
