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
#include "MixerQueue.h"
#include "Engine.h"
#include "Thread.h"
#include "Debug.h"

/* Event priority queue datastructure */
static Event **Heap; /* Array of pointers to the IncomingEvents */
static int Top; /* Current Top of the Heap */
static int Size; /* Size of Heap */

extern pthread_mutex_t Qlock;

/* Initialize the Heap to some Size s */
void MixerQueueInit(int s) {
    Top = 0;
    Size = s;
    Heap = (Event **) calloc(sizeof (Event *), Size);
}

/* Swap two elements */
void Swap(int i, int j) {
    Event *temp;

    temp = Heap[i];
    Heap[i] = Heap[j];
    Heap[j] = temp;
}

/* Put an Event into the queue */
void MixerEnqueue(Event * new_event) {
    int pos = Top;

    ASSERT(new_event != NULL);

    /* Do a mutex lock when modifying the queue */
    ThreadLock(&Qlock);

    Heap[Top] = new_event; /* Put the event in the Heap */
    BubbleUp(pos); /* Adjust position of the event  */
    Top++;

    ThreadUnlock(&Qlock);

#if DEBUG_LEVEL & DBG_QUE
    Log(DBG_QUE, "Event was enqueued. Events in queue now: %d\n", Top);
#endif

}

/* Move an element up in the list until it fits its position */
int BubbleUp(int pos) {
    int done = 0;
    int parent;

    while (!done) {
        /* Correct for offset 0 rather than 1 */
        parent = (pos + 1) / 2;

        if (parent == 0)
            done = 1;

        if (parent < 0) {
            Swap(0, pos);
            done = 1;
        }

        if (Heap[parent]->priority > Heap[pos]->priority) {
            Swap(parent, pos);
            pos = parent;
        } else
            done = 1;

    }

    return pos;
}

/* Removes an element from the queue */
Event *MixerDequeue(void) {
    Event *res = Heap[0]; /* Save the Top for the return */

    ASSERT(res != NULL);
    ASSERT(Top > 0);

    /* Do a mutex lock when modifying the queue */
    ThreadLock(&Qlock);

    Heap[0] = Heap[--Top]; /* Copy last element to Top, decrease Size */
    BubbleDown(0); /* Adjust element 0 */

    ThreadUnlock(&Qlock);

#if DEBUG_LEVEL & DBG_QUE
    Log(DBG_QUE, "Event was Dequeued. Events remaining: %d\n", Top);
#endif

    return (res);
}

/* Move an element down through the list until it fits */
int BubbleDown(int pos) {
    int done = 0;
    int lchild, rchild;

    while (!done) {
        lchild = (pos + 1) * 2 - 1;
        rchild = (pos + 1) * 2;

        if (lchild < Top && rchild < Top) {

            if (Heap[lchild]->priority <= Heap[rchild]->priority
                    && Heap[lchild]->priority < Heap[pos]->priority) {
                Swap(pos, lchild);
                pos = lchild;
            } else if (Heap[rchild]->priority <= Heap[lchild]->priority
                    && Heap[rchild]->priority < Heap[pos]->priority) {
                Swap(pos, rchild);
                pos = rchild;
            } else
                done = 1;

        } else if (lchild < Top) {
            done = 1;

            if (Heap[lchild]->priority < Heap[pos]->priority) {
                Swap(pos, lchild);
                pos = lchild;
            }

        } else
            done = 1;
    }

    return pos;
}

/* Is the Queue Empty? */
int MixerQueueEmpty(void) {
    return (Top == 0);
}

/* Is the Queue Full? */
int MixerQueueFull(void) {
    return (Top == Size);
}
