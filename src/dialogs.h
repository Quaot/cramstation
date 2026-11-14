#pragma once

#include <windows.h>

#include "profiles.h"

struct name_answer {
    bool accepted = false;
    wstring subject;
    wstring name;
};

// Asks for a subject and a setup name. subjects fills the subject list; it can be typed over.
name_answer ask_for_name( HWND owner, const wchar_t *title, const vector<wstring> &subjects, const wstring &subject,
                          const wstring &name, bool subject_fixed );

struct pick_answer {
    bool accepted = false;
    vector<vector<wstring>> picks;  // picks[i] holds the PDFs ticked for setup item i
    bool separate = false;
};

// Lets the user tick which PDFs the setup's pick windows should open.
pick_answer ask_for_picks( HWND owner, const setup &chosen );
