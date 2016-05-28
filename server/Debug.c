/*
PEEP: The Network Auralizer
Copyright (C) 2000 Michael Gilfix

This file is part of PEEP.

You should have received a file COPYING containing license terms
along with this program; if not, write to Michael Gilfix
(mgilfix@eecs.tufts.edu) for a copy.

This version of PEEP is open source; you can redistribute it and/or
modify it under the terms listed in the file COPYING.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include "Debug.h"

static FILE *Log_Handle = NULL;

/* The following assertion function is thanks to Prof. Alva Couch
 * at Tufts university */
#if DEBUG_LEVEL & DBG_ASSRT

static int assert(int boolean, char *boolstr, char *file, int line) {
    if (!boolean)
        Log(DBG_ASSRT, "Assertion %s failed in line %d of file %s\n", boolstr, line, file);
    /* else Log(DBG_ASSRT, "Assertion %s succeeded in line %d of file %s\n",
     * boolstr,line,file); */
    return boolean != 0;
}
#endif

/* Function to initialize the Logging routines */
int LogInit(char *log_file) {
    if (log_file) {
        if ((Log_Handle = fopen(log_file, "a")) == NULL) {
            char *s = "Couldn't open server log file: ";

            strcat(s, log_file);
            perror(s);
            return 0;
        }
    } else {
        Log_Handle = stderr;
    }

    return 1;
}

/* Function to close the log file cleanly */
int LogClose(void) {
    if (fclose(Log_Handle) != 0)
        perror("Error closing server log file");
}

/* A log wrapper function */
void Log(int level, char *s, ...) {
    va_list ap;
    char output[LOG_BUF]; /* Output buffer */

    if (level & DEBUG_LEVEL) {
        /* Grab the formatted args into the va_list */
        va_start(ap, s);
        /* Use vsnprintf with a specific buffer length to avoid buffer overflows */
        vsnprintf(output, LOG_BUF, s, ap);
        va_end(ap);

        /* Output and flush the stream so we don't hang onto logging info */
        fprintf(Log_Handle, output);
        fflush(Log_Handle);
    }
}
