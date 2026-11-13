#pragma once

#include "files.h"

// picks[i] holds the PDFs ticked for setup item i (only pick items use it).
// separate puts each picked PDF in its own PdfCraft window.
// Returns problems to show the user; empty when everything opened.
vector<wstring> open_setup( const setup &chosen, const vector<vector<wstring>> &picks, bool separate );
