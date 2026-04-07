#include <stdio.h>
#include <stdlib.h>
#include "structs.h"
#include "error.h"


// ==========================================================================
// fatal error
void ds1edit_error(const char * text)
{
   // Log error to stderr (non-fatal in OpenD2 — the standalone editor
   // calls exit() here, but the game must continue running).
   fprintf(stderr, "ds1edit_error: %s\n", text);
   fflush(stderr);
}
