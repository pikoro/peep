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

#ifndef __PEEP_MIXERQUEUE_H__
#define __PEEP_MIXERQUEUE_H__

/* So we can declare an Event function */
#include "Engine.h"

/* Priority Queue Functions */
void HeapInit (int s);
void Swap (int i, int j);
void MixerEnqueue (Event * new_event);
Event *MixerDequeue (void);
int BubbleUp (int pos);
int BubbleDown (int pos);
int MixerQueueEmpty (void);
int MixerQueueFull (void);

#endif __PEEP_MIXERQUEUE_H__
