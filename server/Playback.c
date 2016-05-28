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
#include <string.h>
#include <errno.h>
#include "Main.h"
#include "Playback.h"
#include "Debug.h"

static FILE *Playback_Stream = NULL;	/* Pointer to the file stream */
static struct playback_h Header;	/* The file header */
static unsigned long Event_Count = 0;	/* Keep track of # of events */
static PlaybackMode Mode;				/* Current playback mode */
static int Use_Playback = 0;		/* Default to not using playback */
static int Loopback_Flag = 0;		/* Used loopback while recording? */

extern int errno;

/* Turn the playback mode on if m isn't null. Return whether it's on
 * in all cases */
int PlaybackModeOn (int *t)
{
	if (t)
		Use_Playback = *t;

	return Use_Playback;
}

/* Set the playback mode if the mode is not null. If m is null,
 * we have just a query for the playback mode */
PlaybackMode PlaybackSetMode (PlaybackMode * m)
{
	if (m)
		Mode = *m;

	return Mode;
}

/* Initialize all of the playback routines */
int PlaybackFileInit (char *file)
{

#if DEBUG_LEVEL & DBG_PLBK
	Log (DBG_PLBK, "Playback routines using file: %s, ", file);
	if (PlaybackSetMode (NULL) == RECORD_MODE)
		Log (DBG_PLBK, "mode: 'RECORD_MODE'\n");
	else
		Log (DBG_PLBK, "mode: 'PLAYBACK_MODE'\n");
#endif

	switch (PlaybackSetMode (NULL)) {

	case PLAYBACK_MODE:
		if ((Playback_Stream = fopen (file, "r")) == NULL) {
			char *s = strerror (errno);
			Log (DBG_GEN, "Uh Oh! Couldn't open playback file: %s\n", s);
			Shutdown ();
		}

		/* Lead the header into memory */
		if (fread (&Header, sizeof (struct playback_h), 1, Playback_Stream) == 0) {
			char *s = strerror (errno);
			Log (DBG_GEN, "Error reading in playback header: %s\n", s);
			return 0;
		}

		break;

	case RECORD_MODE:
		if ((Playback_Stream = fopen (file, "w")) == NULL) {
			char *s = strerror (errno);
			Log (DBG_GEN, "Uh Oh! Couldn't create recoding file: %s\n", s);
			Shutdown ();
		}

		bzero ((char *)&Header, sizeof (struct playback_h));

		Header.major_ver = PLAYBACK_MAJOR_VER;
		Header.minor_ver = PLAYBACK_MINOR_VER;
		Header.max_events = MAX_PLAYBACK_EVENTS;

		/* Our initial start position can be found right after
		 * the header */
		Header.start_pos = sizeof (struct playback_h);

		gettimeofday (&Header.start_t, NULL);

		/* Initialize the file with the header. Note that end_t is initially
		 * left at zero */
		if (fwrite (&Header, sizeof (struct playback_h), 1, Playback_Stream) == 0) {
			char *s = strerror (errno);
			Log (DBG_GEN, "Error initializing file with playback header: %s\n", s);
			return 0;
		}

		/* Flush out the header */
		fflush (Playback_Stream);
		break;

	default:
		Log
			(DBG_DEF,
			 "Uh Oh! peepd asked to use invalid mode for playback. Please specify: {PLAYBACK_MODE, RECORD_MODE}\n");
		Shutdown ();
	}

#if DEBUG_LEVEL & DBG_PLBK
	Log (DBG_PLBK,
			 "The playback header is:\n    major ver => %d\n    minor ver => %d\n    max events => %d\n",
			 Header.major_ver, Header.minor_ver, Header.max_events);
	Log (DBG_PLBK,
			 "    events written => %d\n    start pos => %ld\n    start time =>  %lf\n    end time => %lf\n\n",
			 Header.written, Header.start_pos, TP_IN_FP_SECS (Header.start_t),
			 TP_IN_FP_SECS (Header.end_t));
#endif

	return 1;
}

/* Figure out which event is the starting event in the round-robin
 * event file because events have incremental times */
long FindFirstOffset (void)
{
	playback_t cur, prev;
	long initial_pos = ftell (Playback_Stream);

	fread (&prev, sizeof (playback_t), 1, Playback_Stream);

	while (!feof (Playback_Stream)) {
		fread (&cur, sizeof (playback_t), 1, Playback_Stream);

		/* Check if the event we're reading in was earlier than the
		 * previous event. If so, return the position right before 
		 * the read */
		if (TP_IN_FP_SECS (cur.record.mix_in_time) <
				TP_IN_FP_SECS (prev.record.mix_in_time)) {
			return (ftell (Playback_Stream) - sizeof (playback_t));
		}
	}

	/* If we got here, then we didn't record enough events
	 * to round-robin */
	return (initial_pos);
}

/* Write out an event into the recording file */
int PlaybackRecordEvent (Event e)
{
	/* The record to write out */
	playback_t rec;

	/* Record the time when the event occurred */
	gettimeofday (&e.mix_in_time, NULL);

#if DEBUG_LEVEL & DBG_PLBK
	Log (DBG_PLBK, "Recording event at time: %lf, file offset: %d\n",
			 TP_IN_FP_SECS (e.mix_in_time), ftell (Playback_Stream));
#endif

	/* Create a record */
	rec.record = e;

	/* Have we written our maximum events?  */
	if (Event_Count != MAX_PLAYBACK_EVENTS) {
		if (fwrite (&rec, sizeof (playback_t), 1, Playback_Stream) == 0) {
			char *s = strerror (errno);
			Log (DBG_GEN, "Error writing to recording file: %s\n", s);
			return 0;
		}
	}
	else {
		Loopback_Flag = 1;

		/* Else, reset our position, right after the header */
		if (fseek (Playback_Stream, sizeof (struct playback_h), SEEK_SET) < 0) {
			char *s = strerror (errno);
			Log (DBG_GEN, "Error seeking in file: %s\n", s);
			return 0;
		}

		if (fwrite (&rec, sizeof (playback_t), 1, Playback_Stream) == 0) {
			char *s = strerror (errno);
			Log (DBG_GEN, "Error writing to recording file: %s\n", s);
			return 0;
		}
	}

	fflush (Playback_Stream);

	Event_Count++;
	return 1;
}

/* Read the events from the playback file. User can optionally
 * specify the starting and ending time to listen to. If these
 * aren't sepcified (i.e, start_t == NULL and end_t == NULL),
 * then playback starts at the earlier events in the file, which
 * should already be pointed at in the file stream by the time
 * this function is called */
void GoPlaybackMode (char *start_t, char *end_t)
{
	struct timeval start, end, current, event_offset, time_offset;
	double plbk_offset, cur_time_offset;
	long start_pos, end_pos;
	char *Time_Format = PLAYBACK_TIME_FORMAT;
	playback_t rec;


	/* Check if we've been given a start time */
	if (start_t != NULL) {
		struct tm tm;

		/* We need to convert the ascii start and end times into
		 * seconds since the epoch */
		bzero ((char *)&tm, sizeof (struct tm));
		strptime (start_t, Time_Format, &tm);
		start.tv_sec = mktime (&tm);
		start.tv_usec = 0;

#if DEBUG_LEVEL & DBG_PLBK
		Log (DBG_PLBK, "Converted start time is: %lf\n", TP_IN_FP_SECS (start));
#endif

		/* Now find the starting time */
		Header.start_pos = FindFirstOffset ();
		if ((start_pos = FindPlaybackTime (start, Header.start_pos)) == 0) {
			Log
				(DBG_DEF,
				 "Couldn't find start time in playback file. Playing from beginning.");
			start_pos = sizeof (struct playback_h);
		}

		/* Now check if we also have an end time */
		if (end_t != NULL) {
			bzero ((char *)&tm, sizeof (struct tm));
			strptime (end_t, Time_Format, &tm);
			end.tv_sec = mktime (&tm);
			end.tv_usec = 0;

#if DEBUG_LEVEL & DBG_PLBK
			Log (DBG_PLBK, "Converted end time is: %lf\n", TP_IN_FP_SECS (end));
#endif

			/* Check if there's an ending time */
			if ((end_pos = FindPlaybackTime (end, FindFirstOffset ())) == 0) {
				/* Since we can't find the end time in the playback file, we want to
				 * end at the end of the round robin in the file which is either right
				 * after the header or the event just prior to the start position */
				end_pos = (Header.start_pos == sizeof (struct playback_h))
					? sizeof (struct playback_h)
					: start_pos - sizeof (playback_t);
			}
		}

		/* Now set ourselves at the start position to be sure we're there */
		if (fseek (Playback_Stream, start_pos, SEEK_SET) < 0) {
			char *s = strerror (errno);
			Log (DBG_GEN, "Error seeking in file: %s\n", s);
			Shutdown ();
		}
	}
	else {
		/* Otherwise, just play from the beginning */
		Log (DBG_DEF, "No start and end time given.\n");
		Log (DBG_DEF, "Starting playback from the earliest event in file...\n");

		/* Check if we last closed the file correctly */
		if (TP_IN_FP_SECS (Header.end_t) != 0.0) {
			if (fseek (Playback_Stream, Header.start_pos, SEEK_SET) < 0) {
				char *s = strerror (errno);
				Log (DBG_GEN, "Error seeking in file: %s\n", s);
				Shutdown ();
			}
		}
		else {
			/* The file wasn't closed correctly so we need to determine
			 * where the starting position is in the round robin */
			if (fseek (Playback_Stream, FindFirstOffset (), SEEK_SET) < 0) {
				char *s = strerror (errno);
				Log (DBG_GEN, "Error seeking in file: %s\n", s);
				Shutdown ();
			}
		}

		start_pos = ftell (Playback_Stream);
		end_pos = 0;
	}

	/* Figure out time diff with respect to our current time */
	fread (&rec, sizeof (playback_t), 1, Playback_Stream);
	event_offset = rec.record.mix_in_time;
	gettimeofday (&time_offset, NULL);
	fseek (Playback_Stream, -sizeof (playback_t), SEEK_CUR);

#if DEBUG_LEVEL & DBG_PLBK
	Log (DBG_PLBK, "Calculated event offset: %lf and time offset: %lf...\n",
			 TP_IN_FP_SECS (event_offset), TP_IN_FP_SECS (time_offset));
	Log (DBG_PLBK, "Expected start position: %ld and end position: %ld\n",
			 start_pos, end_pos);
#endif

	/* Do the actual playback */
	do {

#if DEBUG_LEVEL & DBG_PLBK
		Log (DBG_DEF, "We're currently looking at position: %ld\n",
				 ftell (Playback_Stream));
#endif

		if (fread (&rec, sizeof (playback_t), 1, Playback_Stream) == 0) {

			switch (errno) {

			case EINTR:
#if DEBUG_LEVEL & DBG_PLBK
				Log (DBG_PLBK, "Hit eof. Reseting to end of header...\n");
#endif
				fseek (Playback_Stream, sizeof (struct playback_h), SEEK_SET);

				continue;

			default:
				{
					char *s = strerror (errno);
					Log (DBG_GEN,
							 "Error reading in event: %s. Attempting to continue...\n", s);
					/* Attempting to continue */
				}
			}
		}

		gettimeofday (&current, NULL);
		plbk_offset =
			TP_IN_FP_SECS (rec.record.mix_in_time) - TP_IN_FP_SECS (event_offset);
		cur_time_offset = TP_IN_FP_SECS (current) - TP_IN_FP_SECS (time_offset);

#if DEBUG_LEVEL & DBG_PLBK
		Log (DBG_PLBK,
				 "Playback: Mix in time: %lf, current time: %lf, mix offset: %lf, time offset: %lf, sleeping: %lf\n",
				 TP_IN_FP_SECS (rec.record.mix_in_time), TP_IN_FP_SECS (current),
				 plbk_offset, cur_time_offset, plbk_offset - cur_time_offset);
#endif

		/* Sleep our event offset if necessary */
		if (plbk_offset > cur_time_offset)
			usleep ((unsigned long)((plbk_offset - cur_time_offset) * 1000000.0));

		EngineEnqueue (rec.record);

		Event_Count++;

	} while (ftell (Playback_Stream) != end_pos
					 && ftell (Playback_Stream) != start_pos);

	/* Now that we're done playback, let's wait a bit for the next sound to play
	 * and then shutdown */
	sleep (PLAYBACK_TRAIL_TIME);
	Shutdown ();
}

long FindPlaybackTime (struct timeval t, long start_pos)
{
	playback_t rec;
	struct timeval time;

	if (fseek (Playback_Stream, start_pos, SEEK_SET) < 0) {
		char *s = strerror (errno);
		Log (DBG_GEN, "Error seeking in file: %s\n", s);
		return 0;
	}

	do {
		fread (&rec, sizeof (playback_t), 1, Playback_Stream);
		time = rec.record.mix_in_time;

		if (TP_IN_FP_SECS (time) >= TP_IN_FP_SECS (t)) {
			return (ftell (Playback_Stream) - sizeof (playback_t));
		}

		/* Check if we need to round robing in front of the header */
		if (feof (Playback_Stream)) {
			if (fseek (Playback_Stream, sizeof (struct playback_h), SEEK_SET) < 0) {
				char *s = strerror (errno);
				Log (DBG_GEN, "Error seeking in file: %s\n", s);
				return 0;
			}
		}

	} while (ftell (Playback_Stream) != start_pos);

	return 0;
}

int PlaybackFileShut (void)
{
	switch (Mode) {

	case RECORD_MODE:
		/* Finish filling out the header and rewrite it to disk */
		Header.written = Event_Count;
		gettimeofday (&Header.end_t, NULL);

		/* At this point, either we're at the end of the file, or
		 * we should currently be pointing at it */
		if (Loopback_Flag)
			Header.start_pos = ftell (Playback_Stream);
		else
			Header.start_pos = sizeof (struct playback_h);

#if DEBUG_LEVEL & DBG_PLBK
		Log (DBG_PLBK,
				 "On shutdown, the playback header is:\n    major ver => %d\n    minor ver => %d\n    max events => %d\n",
				 Header.major_ver, Header.minor_ver, Header.max_events);
		Log (DBG_PLBK,
				 "    events written => %d\n    start pos => %ld\n    start time =>  %lf\n    end time => %lf\n\n",
				 Header.written, Header.start_pos, TP_IN_FP_SECS (Header.start_t),
				 TP_IN_FP_SECS (Header.end_t));
#endif

		/* Now seek to the start of the file and write out the header */
		if (fseek (Playback_Stream, 0, SEEK_SET) < 0) {
			char *s = strerror (errno);
			Log (DBG_GEN, "Error seeking in file: %s", s);
			return 0;
		}
		fwrite (&Header, sizeof (struct playback_h), 1, Playback_Stream);
		fflush (Playback_Stream);
		break;

	case PLAYBACK_MODE:
		Log (DBG_DEF, "Finished playback. You just heard %d events.\n",
				 Event_Count);
		break;

	default:
		Log (DBG_DEF, "Warning: peepd was in an invalid playback mode.");
		Log (DBG_DEF, "Shutdown may not have been successful\n");
	}
}
