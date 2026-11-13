#pragma once

#include "text.h"

// Switches to the virtual desktop with this name, as Ctrl+Win+Left/Right would.
// Returns an empty string on success, or what went wrong.
wstring switch_to_desktop( const wstring &name );
