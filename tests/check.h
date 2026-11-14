#pragma once

// Prints one line for the check and counts it when it fails.
void check( bool passed, const char *what );

// How many checks have failed so far.
int check_failures();
