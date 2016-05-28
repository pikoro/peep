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

#ifndef __PEEP_DEBUG_H__
#define __PEEP_DEBUG_H__

/* Logging levels, defined as flags */
#define DBG_DEF		(1 << 0)			/* Default, standard output */
#define DBG_GEN		(1 << 2)			/* General operational info */
#define DBG_SRVR	(1 << 4)			/* Server interactions */
#define DBG_ENG		(1 << 6)			/* Engine information */
#define DBG_MXR		(1 << 8)			/* Mixer info */
#define DBG_QUE		(1 << 10)			/* Queuing operations */
#define DBG_PLBK	(1 << 12)			/* Debugging about playback and recording */

#define DBG_ASSRT	(1 << 31)			/* Debug with assertions. This will replace the old
																 * __DEBUG_WITH_ASSERT__ */

/* Debugging level definitions */
#define DBG_LOWEST				(DBG_DEF | DBG_GEN)
#define DBG_LOWER					(DBG_LOWEST | DBG_SRVR)
#define DBG_MEDIUM				(DBG_LOWER | DBG_PLBK | DBG_ENG )
#define DBG_HIGHER				(DBG_MEDIUM | DBG_MXR)
#define DBG_HIGHEST				(DBG_HIGHER | DBG_QUE)
#define DBG_ALL_W_ASSERT	(DBG_HIGHEST | DBG_ASSRT)

/* Variable definition to compile in code assertions.
 * Note that compiling this in will greatly reduce the speed
 * of code execution */
#if DEBUG_LEVEL & DBG_ASSRT
#define  ASSERT(X) assert((X),#X,__FILE__,__LINE__)
static int assert (int boolean, char *boolstr, char *file, int line);
#else
#define ASSERT(X)
#endif

/* Logging initialization and shutdown routines */
int LogInit (char *log_file);
int LogClose (void);

#define LOG_BUF 1024

/* Include stdio so we don't have problems with using
 * a FILE * type */
#include <stdio.h>

/* Variable argument logging function */
void Log (int level, char *s, ...);

#endif __PEEP_DEBUG_H__
