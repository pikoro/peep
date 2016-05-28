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
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <values.h>
#include "VoiceMixer.h"
#include "Engine.h"
#include "Sound.h"
#include "Playback.h"
#include "Thread.h"
#include "Debug.h"

/* Create an event sound map */
static unsigned int Event_Map_Size; /* The size of the Map array */
static short ***Event_Sounds; /* Array of Samples loaded into memory */
static unsigned int **Event_Lengths; /* Lengths of the samples */
static unsigned int *No_Event_Snds; /* The number of event sounds associated with an event */

/* Engine Scheduling Data Structure (Calloc'd to half the number of channels) */

static double *Intelli_Map; /* Mappings of minimal playtimes (Currently considered obsolete) */
static double *Startt; /* Arry of START times for each chan */
static long *Priorit; /* Arry of priorities of each scheduled event */
static double *Minendt; /* Arry of Min playtimes for intelligibility */

/* End Engine Data Structure */

pthread_mutex_t Qlock; /* Lock for modifying the queue */
extern pthread_mutex_t Tlock; /* Lock for modifying the timing structure
																 * shared by the mixer and the Engine */

void EngineInit(int card, char *device) {
    int i;

    /* Start the mixer, which initializes the void *Handle to point to
     * the sound card */
    MixerInit(card, device);

    /* Allocate Event arrays based on the number of voices */
    Startt = (double *) calloc(MixerEventChans(), sizeof (double));
    Priorit = (long *) calloc(MixerEventChans(), sizeof (long));
    Minendt = (double *) calloc(MixerEventChans(), sizeof (double));

    /* Initialize the Heap for 64 events */
    MixerQueueInit(64);

    /* Init the mutext lock */
    pthread_mutex_init(&Qlock, NULL);
}

unsigned int EngineGetEventMapSize(void) {
    return Event_Map_Size;
}

unsigned int EngineSetEventMapSize(unsigned int e) {
    return Event_Map_Size = e;
}

int EngineCreateIntelliMap(int size) {
    if ((Intelli_Map = (double *) calloc(size, sizeof (double))) == NULL)
        return ENGINE_ALLOC_FAILED;
    else
        return ENGINE_SUCCESS;
}

int EngineAllocEventSounds(int size) {
    Event_Sounds = (short ***) calloc(size, sizeof (short **));
    Event_Lengths = (unsigned int **) calloc(size, sizeof (unsigned int *));
    No_Event_Snds = (unsigned int *) calloc(size, sizeof (unsigned int));

    if (Event_Sounds && Event_Lengths && No_Event_Snds)
        return ENGINE_SUCCESS;
    else
        return ENGINE_ALLOC_FAILED;
}

unsigned int EngineGetNoEventSnds(int index) {
    return No_Event_Snds[index];
}

int EngineSetNoEventSnds(int index, unsigned int value) {
    if (No_Event_Snds == NULL)
        return ENGINE_NOT_YET_ALLOC;

    No_Event_Snds[index] = value;
    return ENGINE_SUCCESS;
}

double EngineGetIntelliMap(int index) {
    return Intelli_Map[index];
}

int EngineSetIntelliMap(int index, double value) {
    if (Intelli_Map == NULL)
        return ENGINE_NOT_YET_ALLOC;

    Intelli_Map[index] = value;
    return ENGINE_SUCCESS;
}

int EngineCreateEventSoundEntry(int index, int no_events) {
    if (Event_Sounds == NULL)
        return ENGINE_NOT_YET_ALLOC;

    Event_Sounds[index] = (short **) calloc(sizeof (short *), no_events);
    Event_Lengths[index] = (unsigned int *) calloc(sizeof (unsigned int),
            no_events);

    if (Event_Sounds[index] && Event_Lengths[index])
        return ENGINE_SUCCESS;
    else
        return ENGINE_ALLOC_FAILED;
}

int EngineAssignEventSoundEntry(int index, int event_no,
        short *sound, unsigned int len) {
    if (Event_Sounds == NULL || Event_Sounds[index] == NULL
            || Event_Lengths == NULL || Event_Lengths[index] == NULL)
        return ENGINE_NOT_YET_ALLOC;

    Event_Sounds[index][event_no] = sound;
    Event_Lengths[index][event_no] = len;
    return ENGINE_SUCCESS;
}

short *EngineGetEventSound(int index, int event_no) {
    if (Event_Sounds == NULL || Event_Sounds[index] == NULL)
        return NULL;
    else
        return Event_Sounds[index][event_no];
}

unsigned int EngineGetEventSoundLength(int index, int event_no) {
    if (Event_Lengths == NULL || Event_Lengths[index] == NULL)
        return ENGINE_NOT_YET_ALLOC;
    else
        return Event_Lengths[index][event_no];
}

int EngineSchedulerInit(int index, double start, double minend, long prior) {
    if (Startt == NULL || Minendt == NULL | Priorit == NULL)
        return ENGINE_NOT_YET_ALLOC;

    Startt[index] = start;
    Minendt[index] = minend;
    Priorit[index] = prior;

    return ENGINE_SUCCESS;
}

void EngineIO(Event * incoming_event) {
    static int lastc = 0; /* Keep track of which we were working on last */
    int next_sound; /* The next event sound, to be chosen at random */
    int c = 0, bestc, ind; /* Counters for event algorythm */
    struct timeval tp; /* For get time of day call */

    /* Make a local copy for speed */
    unsigned int no_event_chans = MixerEventChans();

    /* Check if we should playback is active and whether we should record the event */
    if (PlaybackModeOn(NULL) && PlaybackSetMode(NULL) == RECORD_MODE) {
        if (!PlaybackRecordEvent(*incoming_event)) {
            Log(DBG_GEN,
                    "WARNING: Error recording an event. Event not recorded\n");
            /* Continue anyway */
        }
    }

    if (incoming_event->type == EVENT) {
        /*
         * The following Algorithm is applied here: 
         * When an event comes in:
         *   -try to pick a channel c that's idle
         *   -if that's not possible, pick a channel c with: 
         *     -already played for minimal time
         *      (distance > minendt[c])
         * distance is: TP_IN_FP_SECS(tp)-startt[c]
         *     -minimal priorit[c] - interrupt lowest priority snd
         *     -minimal startt[c] (for c's with the same priority)
         */

        /* If -1, we need temporal queueing */
        bestc = -1;

        for (ind = 0; ind < no_event_chans; ind++) {

            /* Round Robin starting at last c */
            c = (lastc + ind) % no_event_chans;

            ASSERT(ind >= 0 && ind < no_event_chans);
            ASSERT(c >= 0 && c < no_event_chans);

            /* Can we get a channel right away? */
            if (Startt[c] == 0) {

#if DEBUG_LEVEL & DBG_ENG
                Log(DBG_ENG, "We found a channel right away: %d\n", c);
#endif

                bestc = c;
                break;
            }

            /* Now choose sound with lowest priority and one past the point
             * of intelligable playing. This currently wont' happen because
             * the intelligable time has been max'd and is currently not in
             * use. */
            gettimeofday(&tp, NULL);
            if (TP_IN_FP_SECS(tp) - Startt[c] > Minendt[c]) {

                ASSERT(Startt[c] >= 0 && Minendt[c] >= 0);

#if DEBUG_LEVEL & DBG_ENG
                Log(DBG_ENG,
                        "We've past the intelligible playing time: Channel: %d\n", c);
#endif

                bestc = c;
                break;
            }

            if (bestc == 0 && (Priorit[bestc] < Priorit[c])) {

                ASSERT(Priorit[c] >= 0 && Priorit[bestc] >= 0);

#if DEBUG_LEVEL & DBG_ENG
                Log(DBG_ENG,
                        "Event with higher priority than channel: %d or c = 0\n", c);
#endif

                bestc = c;
                continue;
            }

            if (bestc == 0
                    && (Priorit[bestc] == Priorit[c] && (Startt[bestc] > Startt[c]))) {

                ASSERT(Priorit[c] >= 0 && Startt[bestc] >= 0 && Startt[c] >= 0);

#if DEBUG_LEVEL & DBG_ENG
                Log(DBG_ENG, "Hit a minimal startt[c] for channel: %d\n", c);
#endif

                bestc = c;
                continue;
            }
        }

        /*
         * At this point, bestc is either a blank channel or the lowes
         * priority one that started earliest. If bestc == -1, then we
         * don't have a channel yet and should put into the temporal
         * queue.
         * Note that the actual output to the mixer is one less than
         * bestc. This is because the 32 voices of the mixer actually
         * go from 0-31. If bestc != 0, then we have a sound to play 
         * and bestc is always positive */

        /* Should it get Enqueued? If so, do it and get out of the func */
        if (bestc == -1) {

            /* If the queue is full, discard the event anyway */
            if (MixerQueueFull()) {

#if DEBUG_LEVEL & DBG_QUE
                Log(DBG_QUE, "Heap was full. Event discarded...\n");
#endif

                return;
            }

            gettimeofday(&tp, NULL);
            incoming_event->mix_in_time = tp;
            MixerEnqueue(incoming_event);

            return;
        } else {

            /* Make sure that lastc gets assigned to the next one to start
             * searching at... */
            lastc = (bestc + 1) % no_event_chans;
        }

        /* Interrupt the channel if playing */
        if (Startt[bestc] != 0) {
            MixerInterruptChannel(bestc);

            ASSERT(bestc >= 0);

#if DEBUG_LEVEL & DBG_ENG
            Log(DBG_ENG, "Interrupted Channel: %d\n", bestc);
#endif

        }

        /* Pick a new random event sound and add it into the mixer */
#if DEBUG_LEVEL & DBG_ENG
        Log(DBG_ENG, "Mixing in sound on channel: %d\n", bestc);
#endif

        next_sound = (unsigned int) ((double) No_Event_Snds[incoming_event->sound] *
                rand() / (RAND_MAX + 1.0));
        MixerAddSound(Event_Sounds[incoming_event->sound][next_sound],
                Event_Lengths[incoming_event->sound][next_sound],
                incoming_event->location, bestc);

        /* Update Sound data structures */
        gettimeofday(&tp, NULL);

        /* Lock so we don't collide with the mixer */
        ThreadLock(&Tlock);

        ASSERT(bestc >= 0 && bestc < no_event_chans);

        Startt[bestc] = TP_IN_FP_SECS(tp);

#if DEBUG_LEVEL & DBG_ENG
        Log(DBG_ENG, "Assigned startt[bestc] to be %lf on channel: %d\n",
                Startt[bestc], bestc);
#endif

        Minendt[bestc] = Startt[bestc] + Intelli_Map[incoming_event->sound];
        Priorit[bestc] = incoming_event->priority;

        ThreadUnlock(&Tlock);

        /* Free the event */
        cfree(incoming_event);
    } else {
        /* Process a state sound. The mapping number corresponds directly to
         * the number of event channels. This is checked when loading so we
         * can do this: (Volumes must also be between 0.0 and 1.0 and double
         * 
         * Note that a dither of 255 (which is default for the sender 
         * package does not change the fade time for states. This means that
         * we can only set a parameter of 254 which is never exactly the max
         * fade_in time possible. Big deal. */

        double volume = ((double) incoming_event->volume / 255.0) *
                (STATE_MULT / (double) MixerLoadedStates(NULL));
        unsigned char stereo = incoming_event->location;
        double fade = MixerStateFade(incoming_event->sound, NULL);

        Log(DBG_ENG, "Mixer loaded states: %d\n", MixerLoadedStates(NULL));

        if (incoming_event->dither != 255)
            fade = (double) incoming_event->dither / (255.0 / MAX_FADE_TIME);
        else {

#if DEBUG_LEVEL & DBG_ENG
            Log(DBG_ENG, "Fade value was 255. Ignoring...\n");
#endif

        }

        /* Tell the mixer about the new event info */
        SetStateSound(incoming_event->sound, volume, stereo, fade);

        cfree(incoming_event);

#if DEBUG_LEVEL & DBG_ENG
        Log(DBG_ENG, "Set StateVolumes[%d] to %lf\n", incoming_event->sound,
                volume);
        Log(DBG_ENG, "Set StateFade[%d] to %lf\n", incoming_event->sound, fade);
#endif

    }
}

void EngineShutdown(void) {
    int i, j;

    for (i = 0; i < EngineGetEventMapSize(); i++) {
        for (j = 0; j < EngineGetNoEventSnds(i); j++) {
            cfree(Event_Sounds[i][j]);
            cfree(Event_Lengths[j]);
        }

        cfree(Event_Sounds[i]);
    }
    cfree(Event_Sounds);
    cfree(Event_Lengths);
    cfree(No_Event_Snds);

    /* Free the engine scheduler arrays */
    cfree(Startt);
    cfree(Priorit);
    cfree(Minendt);
    cfree(Intelli_Map);
}
