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

#ifndef __PEEP_ENGINE_H__
#define __PEEP_ENGINE_H__

/* Include time definitions so we can use the u_char and clock_t
 * definitions */
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <unistd.h>

typedef struct
{
	unsigned char type;						/* State or single event? */
	unsigned char sound;					/* Sound to map to */
	unsigned char location;				/* Stereo location in terms of left channel
																 * Right channel is simple 255 - location */
	unsigned char priority;				/* Priority of the event */
	unsigned char volume;					/* Playback volume  - Have to divide by 255 */
	unsigned char dither;					/* Adjustable parameter for sound dithering.
																 * 2 meanings -
																 * 1: Applies to states, sets the fade-in time
																 * when mixing between state sounds
																 * 2: Applies delay to handle logs which update
																 * in large spurts at set intervals */
	struct timeval mix_in_time;		/* Time at which an event (if it) was enqueued */
}
Event;

/* For convenience to differentiate events and states */
enum Event_Type
{ EVENT, STATE };

/* Performs the necesary functions to set up the sound engine
 * environment. Accepts the card number, determines the number
 * of voices and initializes the array Handles for each voices */
void EngineInit (int card, char *device);

#define ENGINE_SUCCESS 1
#define ENGINE_NOT_YET_ALLOC -1
#define ENGINE_ALLOC_FAILED -2

/* Returns and sets the site of the sound map, i.e
 * the number of events loaded */
unsigned int EngineGetEventMapSize(void);
unsigned int EngineSetEventMapSize(unsigned int e);

/* Creates an intelli_map array - an array which contains
 * the times in seconds for which a sound must be played for
 * its meaning to be intelligeable, which allows the sound
 * engine to make different subsequent decisions.
 * This feature is currently not used however. */
int EngineCreateIntelliMap(int size);

/* Allocate the event sounds data structure */
int EngineAllocEventSounds(int size);

/* Returns and sets the number of sounds associated with
 * a single event */
unsigned int EngineGetNoEventSnds(int index);
int EngineSetNoEventSnds(int index, unsigned int value);

/* Returns and sets the intelligeable time value for the event
 * specified by index (in floating point secs) */
double EngineGetIntelliMap(int index);
int EngineSetIntelliMap(int index, double value);

/* Creates an entry in the sound array */
int EngineCreateEventSoundEntry(int index, int no_events);

/* Assigns raw sounds data to a given sound entry. note
 * that this fuction references the event number (index)
 * and its sub-event sound (event_no) */
int EngineAssignEventSoundEntry(int index, int event_no,
																short *sound, unsigned int len);

/* Initializes the scheduling data structures that are internal
 * to the sound engine algorithm */
int EngineSchedulerInit (int index, double start, double minend,
																long prior);

/* Returns the raw sound data associated with an event (index)
 * and it's sub-sound (event_no) */
short *EngineGetEventSound (int index, int event_no);

/* Returns the length of the sound data array referenced by
 * the event (index) and sub-sound (event_no) */
unsigned int EngineGetEventSoundLength (int index, int event_no);

/* This function embodies the sound engine's algorithm for
 * finding a mixer channel or enqueuing an event based on the
 * event's priority. It is called every time the server receives
 * an event, so that the event can be directed to its appropriate
 * place. */
void EngineIO (Event * incoming_event);

/* Preform the necessary cleanup upon shutdown */
void EngineShutdown (void);

/* Define an expired queue time to be 5 seconds */
#define QUEUE_EXPIRED      (5 * CLOCKS_PER_SEC)

#define TP_IN_FP_SECS(x) \
	( (double)x.tv_sec + ( (double)x.tv_usec * 0.000001) )

#endif __PEEP_ENGINE_H__
