#pragma once

#include "text.h"

enum class program { pdfcraft, notepad, explorer };

struct planned_window {
    wstring side;           // left, right or full
    program app;
    vector<wstring> paths;  // PDFs for PdfCraft, one file for Notepad, one folder for Explorer
};

// Starts every window, then moves each one to its part of the screen.
// Returns a message for each window that never appeared.
vector<wstring> open_windows( const vector<planned_window> &plan );
