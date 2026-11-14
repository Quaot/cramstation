#pragma once

#include "files.h"
#include "tiling.h"

// Works out the windows to open without opening them or creating any file. A notes window's
// path is the name notes_file would choose. Returns no windows when there is nothing to open.
vector<planned_window> plan_setup( const setup &chosen, const vector<vector<wstring>> &picks, bool separate,
                                   vector<wstring> &problems );

// picks[i] holds the PDFs ticked for setup item i (only pick items use it).
// separate puts each picked PDF in its own PdfCraft window.
// Returns problems to show the user; empty when everything opened.
vector<wstring> open_setup( const setup &chosen, const vector<vector<wstring>> &picks, bool separate );
