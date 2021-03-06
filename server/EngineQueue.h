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

#ifndef __PEEP_ENGINEQUEUE_H__
#define __PEEP_ENGINEQUEUE_H__

/* For definition of an Event */
#include "Engine.h"

/* Engine Queue datastructure.
 *  As events are read in via udp, they are enqueued into this datastructure
 *  until the engine thread discovers the awaiting events and processes them
 */
struct Queued_Event {
    Event Incoming_Event;
    struct Queued_Event *next;
    struct Queued_Event *prev;
};

typedef struct Queued_Event Queued_Event;

void EngineEnqueue(Event d);
Event EngineDequeue(void);
int EngineQueueEmpty(void);

#endif __PEEP_ENGINEQUEUE_H__
