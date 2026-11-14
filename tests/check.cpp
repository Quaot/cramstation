#include "check.h"

#include <cstdio>

namespace {

int failures = 0;

}  // namespace

void check( bool passed, const char *what ) {
    std::printf( "%s  %s\n", passed ? "pass" : "FAIL", what );
    if ( !passed ) ++failures;
}

int check_failures() {
    return failures;
}
