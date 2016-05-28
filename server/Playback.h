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

#ifndef __PEEP_PLAYBACK_H__
#define __PEEP_PLAYBACK_H__

#define PLAYBACK_MAJOR_VER               1
#define PLAYBACK_MINOR_VER               0

#define MAX_PLAYBACK_EVENTS              3200

/* Time to sleep before exiting after trailing event */
#define PLAYBACK_TRAIL_TIME              5

/* For time definitions */
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#include <unistd.h>

struct playback_h {
    char major_ver; /* The version of the playback format */
    char minor_ver;
    unsigned int max_events; /* The number of events in the file */
    unsigned long written; /* The number of events written to the file */
    long start_pos; /* The position of the starting event */
    struct timeval start_t; /* The start time of the recording */
    struct timeval end_t; /* The last time recorded */
};

/* Format is:
 * "<day of week> <month> <day of month> <24 hr>:<min>:<sec> <year>" */
#define PLAYBACK_TIME_FORMAT "%a %b %d %H:%M:%S %Y"

/* For the meaning of an Event type */
#include "Engine.h"

typedef struct {
    /* The playback record includes the event with the mix-in time filled
     * in so we can determine when to play back an event */
    Event record;
}
playback_t;

typedef enum mode {
    PLAYBACK_MODE, RECORD_MODE
}
PlaybackMode;

/* Accessor function to toggle playback mode on and to set the type of mode */
int PlaybackModeOn(int *t);
PlaybackMode PlaybackSetMode(PlaybackMode *m);

/* Func decl. */
int PlaybackFileInit(char *file);
long FindFirstOffset(void);
int PlaybackRecordEvent(Event e);
void GoPlaybackMode(char *start_t, char *end_t);
long FindPlaybackTime(struct timeval t, long start_pos);
int PlaybackFileShut(void);

#endif __PEEP_PLAYBACK_H__
