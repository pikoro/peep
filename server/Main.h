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

#ifndef __PEEP_MAIN_H__
#define __PEEP_MAIN_H__

/* Definition of default constants */

/* This is now defined via configure
#define DEFAULT_CONFIG_PATH                     "/etc/peep.conf"
*/

#define DEFAULT_DEVICE_PATH                      "/dev/audio"
#define DEFAULT_PORT_NO                          2001

#define MIXER_THREAD_USLEEP                      50000	/* 0.05 secs */
#define ENGINE_POLL_USLEEP                       10000	/* 0.01 secs */

#define DEFAULT_PID_FILE                         "/var/run/peepd.pid"

#define DEFAULT_PLAYBACK_FILE                    "/var/log/peeplog"

/* Argument manipulation macros */
#define GET_STRING_ARG(x) \
	{ if (argv[1][2] != '\0') \
			x = (char *)&(argv[1][2]); \
		else { \
			argc--; argv++; \
			x = argv[1]; \
		} \
	}
#define GET_INT_ARG(x) \
	{ if (argv[1][2] != '\0') \
			(void)sscanf ((char *)&(argv[1][2]), "%d", &x); \
		else { \
			argc--; argv++; \
			(void)sscanf (argv[1], "%d", &x); \
		} \
	}
#define GET_NEXT_ARG \
	argc--; argv++;

/* Fuction Declarations */
void *DoMixing (void *data);
void *EnginePoll (void *data);
void Shutdown(void);
void HandleInterrupt ();
void Usage (void);

#endif __PEEP_MAIN_H__
