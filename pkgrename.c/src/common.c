#include "../include/common.h"

#include <stdio.h>
#include <stdlib.h>

void exit_err(int err, const char *function_name, int line)
{
    fprintf(stderr, "Unrecoverable error %d in function %s, line %d.\n",
        err, function_name, line);
    fprintf(stderr, "Please report this bug at \"%s\".\n", SUPPORT_LINK);
    exit(err);
}
