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
#include "EngineQueue.h"
#include "Debug.h"

static Queued_Event *Head = NULL;
static Queued_Event *Tail = NULL;

/* Add an event into the queue */
void EngineEnqueue (Event d)
{
	Queued_Event *cur_pos = Head;

	if (Head == NULL) {
		Head = (Queued_Event *) malloc (sizeof (Queued_Event));
		Head->Incoming_Event = d;
		Head->next = Head->prev = NULL;
		Tail = Head;
	}
	else {

		/* Loop 'till we hit the end of the queue */
		while (cur_pos->next != NULL)
			cur_pos = cur_pos->next;

		cur_pos->next = (Queued_Event *) malloc (sizeof (Queued_Event));
		cur_pos->next->Incoming_Event = d;
		cur_pos->next->next = NULL;
		cur_pos->next->prev = cur_pos;
		Tail = cur_pos->next;
	}
}

/* Future work for dithering of events:
 *   a) add a playback timestamp to enqueue (time at which to play sound) 
 *   b) while playing, update playback time 
 *   c) when dequeue, SKIP elements at head with later playback time
 *      and splice out first element without later time.  
 *
 *   might want to consider for later....
 *   another mechanism: temporal priority queue
 *   a) make an array priority queue with a LIMITED number of events in it (say 1000)
 *   b) prioritize (sort) by time and then priority. 
 *   c) when full, replace lowest priority and bubble up (that's farthest time away from now) 
 *   d) top is always the next thing to do if timestamp is appropriate (in range).
 */

/* Get an event from the queue */
Event EngineDequeue (void)
{
	Event temporary;

	temporary = Tail->Incoming_Event;

	if (Tail->prev != NULL) {
		Tail = Tail->prev;
		free (Tail->next);
		Tail->next = NULL;
	}
	else {
		free (Tail);
		Tail = Head = NULL;
	}

	return (temporary);
}

/* Returns true if the engine queue is empty, else false */
int EngineQueueEmpty (void)
{
	return (Head == NULL);
}
