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

#ifndef __PEEP_VOICEMIXER_H__
#define __PEEP_VOICEMIXER_H__

#define DEFAULT_VOICES 32
#define SAMPLE_RATE 44100

#define PERCENT_STATES                   (1.0/8.0)
#define MAX_FADE_TIME                       2.0

#define ASSIGN_IF_NOT_NULL_AND_RET(x,y) \
        { if (x) y = *x; return y; }

#define MIXER_SUCCESS 1
#define MIXER_ALLOC_FAILED -1
#define MIXER_ALREADY_ALLOC -2
#define MIXER_NOT_YET_ALLOC -3

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

/* Initialize the Mixer datastructure and set up the sound card
 * for play */
void MixerInit(int card, char *device);

/* Returns the number of event and state channels
 * respectively */
unsigned int MixerEventChans(void);
unsigned int MixerStateChans(void);

/* Assigns new volume, stereo, and fade time parameters
 * to the sound indexed by 'sound' */
void SetStateSound(unsigned char sound, double volume,
        unsigned char stereo, double fade);

/* Sets the number of actual states loaded (different
 * from the number of allocated channels) if s isn't
 * NULL. Always returns the number of actuals stats */
unsigned int MixerLoadedStates(unsigned int *s);

/* Sets the number of mixing voices used if v isn't
 * NULL. Always returns the number of mixing voices */
unsigned int MixerVoices(unsigned int *v);

/* Sets the fade time between state sounds for a given
 * sound. The fade time is a floating number of secs
 * ranging from 0.0 to MAX_FADE_TIME */
double MixerStateFade(unsigned char sound, double *fade);

/* Check if the mixer knows if a given state sound (specified
 * by 'index' exists */
int MixerExistsStateSound(int index);

/* Check if the mixer has loaded the sound segment for the
 * state sound specified by 'index' */
int MixerLoadedStateSound(int index, int num);

/* Set the number of sound segments for a given state */
int MixerSetSoundsPerState(unsigned char state, unsigned int snds);

/* Returns the number of sound segments for a given state */
unsigned int MixerGetSoundsPerState(unsigned char state);

/* Allocate the necessary resources for the state indexed by
 * 'state' */
int MixerCreateState(unsigned char state, unsigned int snds);

/* Add a sound segment to the state indexed by 'state' */
int MixerAddStateSound(unsigned char state, unsigned int no_sound,
        short *sound, unsigned int length);

/* Add a sound into the mixing buffers. Accepts an array of the sound
 * to play and the channel to assign the sound to. Returns bool true
 * if the sound was added, or bool false if channel in use */
int MixerAddSound(short *sound, unsigned int length,
        unsigned char location, unsigned int channel);

/* Remove an event from the mixer */
void MixerRemoveSound(unsigned int j);

/* Fades the next sound in as the old one nears completion
 * Stop a sound from playing on a given channel */
void MixerInterruptChannel(int channel);

#ifdef DYNAMIC_VOLUME
#define STATE_MULT 0.4
#define EVENT_MULT 0.6

/* Declarations for DYNAMIC_VOLUME */

/* Returns the percentage available as a group multiplier to
 * determine the volume of the sound about to be played */
static double DynamicGroupMult(void);

/* Returns the individual multipler, which multiplies the
 * group multiplier so that the end result has a logarithmic
 * characteristic. At some point I'll revamp the comments on
 * these but if anyone is looking at this, check the website
 * docs of dynamic mixing for this and DynamicGroupMult */
static double DynamicIndivMult(void);

/* Computes the total of the volume multipliers current in use */
static double DynamicTotVolume(void);

/* Return a new multiplier for a new channel just being added */
static double DynamicNewVol(void);

/* Return the minimum volume (integer) of the two
 * Currently unused due to change in dynamic mixing */
static double DynamicMin(double a, double b);

#define DYNAMIC_MULT(x) (Dynamic_Mult[x])
#else
#define DYNAMIC_MULT(x) 1.0
#define STATE_MULT 1.0
#define EVENT_MULT 1.0
#endif DYNAMIC_VOLUME

/* Performs the actual mixing of the sounds and plays them */
void Mixer();

/* Fades out a near ending event linearly and fades in a new random
 * sound linearly */
double MixerLinearFade(int n, int fade_point);

/* Clean up function for when Mixer shuts down */
void MixerShutdown(void);

#endif __PEEP_VOICEMIXER_H__
