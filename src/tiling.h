#pragma once

#include "text.h"

enum class program { pdfcraft, acrobat, notepad, explorer };

// Where each program's .exe is, for starting it and for its icon
const wchar_t *program_file( program app );

struct planned_window {
    wstring side;           // left, right or full
    program app;
    vector<wstring> paths;  // PDFs for PdfCraft, one file for Notepad, one folder for Explorer
};

#include <initializer_list>

// Presses the keys together, then lets them go in reverse order.
void press_keys( std::initializer_list<unsigned short> keys );

// Starts every window, then moves each one to its part of the screen.
// Returns a message for each window that never appeared.
vector<wstring> open_windows( const vector<planned_window> &plan );
