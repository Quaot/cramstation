#pragma once

#include "text.h"

enum class item_kind { notes, pdf, tabs, pick, folder };

struct layout_item {
    wstring side;           // left, right or full
    item_kind kind;
    vector<wstring> paths;  // already joined to the root
};

struct setup {
    wstring title;          // "ECE 150: Exam", or just "ECE 198"
    wstring root;
    wstring notes_folder;
    vector<layout_item> items;
};

struct profile_file {
    vector<setup> setups;
    vector<wstring> errors; // "line 12: ..."
};

wstring profiles_path();
profile_file read_profiles( const wstring &text );
profile_file load_profiles();
const setup *find_setup( const profile_file &profiles, const wstring &title );
wstring describe( const setup &chosen );
