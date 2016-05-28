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
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <values.h>
#include <math.h>
#include "VoiceMixer.h"
#include "MixerQueue.h"
#include "Engine.h"
#include "Sound.h"
#include "Thread.h"
#include "Debug.h"

/* Mixer dynamic channel data structure */
static short **Sound_Bufs;					/* Pointers to the arrays of sounds to be played */
static unsigned int *Buf_Length;		/* Lengths of the Sound arrays */
static unsigned char *Stereo_Left;	/* The array of stereo locations for the sound */
static unsigned int *Sound_Pos;			/* The current position in a given sound array */
static unsigned int No_Event_Chans;	/* The Number of Channels for an event
																		 * Remaining after State calculation */

/* Mixer constant state data structure */
static short ***State_Sounds;
static unsigned int *Snds_per_State;		/* The number of sounds used for 1 state */
static unsigned int **State_Snd_Length;	/* The length of the sounds for a given state sound */
static double *State_Volumes;						/* Volume mul 0.0-1.0 */
static unsigned char *State_Stereo;			/* The stereo location of the state sounds */
static unsigned int No_State_Chans;			/* Calculated as 1/8 of the number of voices */
static unsigned int No_Actual_States;		/* The total number of state sounds actually loaded */

static unsigned int *Cur_Snd;			/* Current sound playing for a given state */
static unsigned int *Cur_Snd_Pos;	/* Sound position within a given sound */

/* Number of mixing voices */
static unsigned int Voices = DEFAULT_VOICES;

/* Add on data structure for fade out and in */
static double *State_Fade;					/* The time remaining at which to begin fading */
static unsigned int *Next_Snd;			/* The next random sound to play */
static unsigned int *Next_Snd_Pos;	/* The position in the next random sound */

/* Structures to manage mixer output */
static int Chunk_Size;					/* Size of a chunk for output (1/10 of a sec) */
static short *Output;						/* A chunk of sound output to be fed to snd card */
static void *Handle;						/* Handle for the Sound Card */

#ifdef DYNAMIC_VOLUME
/* Structure for mixing together dynamic event sounds */
static double *Dynamic_Mult;				/* The dynamic multipliers to control sound volume */
static unsigned int No_Cur_Voices;	/* The number of current voices playing */
#endif

pthread_mutex_t Vlock;					/* Mutext lock for modifying the mixer data structure */
pthread_mutex_t Tlock;					/* Lock the timing datastructure shared by the mixer and
																 * the engine */

void MixerInit (int card, char *device)
{
	extern unsigned int Sound_Port;	/* For the Suns */
	int i;

	/* Open the Sound Device and set the sound format to CD quality
	 * This version now uses stereo sound, so 2 channels */

#ifdef __USING_ALSA_DRIVER__
	Handle = InitSoundCard (&card, 0, SOUND_WRONLY);
#endif

#ifdef __USING_DEVICE_DRIVER__
	Handle = InitSoundCard (device, 0, SOUND_WRONLY);
#endif

	SetSoundFormat (Handle, SIGNED_16_BIT, SAMPLE_RATE, 2, Sound_Port);

	/* Compute chunksize with the following formula for 1/2 sec of sound:
	 * Chunksize = No_secs*frequency in kHz*no_bytes(8 or 16)/8*channels*1024
	 * The array output goes up to ChunkSize since using short data */
	Chunk_Size =
		2 * (((SAMPLE_RATE / 1000) * 2 * (2 /*Stereo */ ) * 1024) / 10);
	Output = (short *)calloc (Chunk_Size, sizeof (short));

	/* Verify that Voices is a proper power of 2 (so we don't crash
	 * or get distortion) and round it to the nearest power of 2 if it isn't */
	Voices =
		(int)pow (2.0, ceil (log ((double)MixerVoices (NULL)) / log (2.0)));

	/* Calculate the number of Event and State Channels */
	No_State_Chans = (int)(PERCENT_STATES * (double)(MixerVoices (NULL)));
	No_Event_Chans = MixerVoices (NULL) - No_State_Chans;

	/* Initialize the Event Datastructure - callocate the arrays: Note
	 * that SoundBufs start out as NULL so that they have zero effect on the
	 * voice addition */
	Sound_Bufs = (short **)calloc (sizeof (short **), No_Event_Chans);
	Buf_Length = (unsigned int *)calloc (sizeof (unsigned int), No_Event_Chans);
	Sound_Pos = (unsigned int *)calloc (sizeof (unsigned int), No_Event_Chans);
	Stereo_Left =
		(unsigned char *)calloc (sizeof (unsigned char), No_Event_Chans);

#ifdef DYNAMIC_VOLUME
	Dynamic_Mult = (double *)calloc (sizeof (double), No_Event_Chans);
	No_Cur_Voices = 0;
#endif

	/* Initalize the State Datastructure -callocate the arrays. */
	State_Sounds			= (short ***)calloc (sizeof (short **), No_State_Chans);
	Snds_per_State		= (unsigned int *)calloc (sizeof (unsigned int), No_State_Chans);
	State_Snd_Length	= (unsigned int **)calloc (sizeof (short *), No_State_Chans);
	State_Volumes			= (double *)calloc (sizeof (double), No_State_Chans);
	State_Stereo			= (unsigned char *)calloc (sizeof (unsigned char), No_State_Chans);
	Cur_Snd						= (unsigned int *)calloc (sizeof (unsigned int), No_State_Chans);
	Cur_Snd_Pos				= (unsigned int *)calloc (sizeof (unsigned int), No_State_Chans);
	State_Fade				= (double *)calloc (sizeof (double), No_State_Chans);
	Next_Snd					= (unsigned int *)calloc (sizeof (unsigned int), No_State_Chans);
	Next_Snd_Pos			= (unsigned int *)calloc (sizeof (unsigned int), No_State_Chans);

	/* Seed Random for playing state sounds */
	srand (1);

	/* Init the mutex lock */
	pthread_mutex_init (&Vlock, NULL);
	pthread_mutex_init (&Tlock, NULL);
}

unsigned int MixerEventChans (void)
{
	return No_Event_Chans;
}

unsigned int MixerStateChans (void)
{
	return No_State_Chans;
}

void SetStateSound (unsigned char sound, double volume,
										unsigned char stereo, double fade)
{

	State_Volumes[sound] = volume;
	State_Stereo[sound] = stereo;
	MixerStateFade (sound, &fade);
}

unsigned int MixerLoadedStates (unsigned int *s)
{
	ASSIGN_IF_NOT_NULL_AND_RET (s, No_Actual_States)
}

unsigned int MixerVoices (unsigned int *v)
{
	ASSIGN_IF_NOT_NULL_AND_RET (v, Voices)
}

double MixerStateFade (unsigned char sound, double *fade)
{

	if (fade) {

		if (*fade > MAX_FADE_TIME)
			*fade = MAX_FADE_TIME;

		return State_Fade[sound] = *fade;
	}
	else
		return State_Fade[sound];
}

int MixerExistsStateSound (int index)
{
	return State_Sounds[index] != NULL;
}

int MixerLoadedStateSound (int index, int num)
{
	return State_Sounds[index][num] != NULL;
}

int MixerSetSoundsPerState (unsigned char state, unsigned int snds)
{
	if (Snds_per_State == NULL) {
		return MIXER_NOT_YET_ALLOC;
	}

	Snds_per_State[state] = snds;

	return MIXER_SUCCESS;
}

unsigned int MixerGetSoundsPerState (unsigned char state)
{
	return Snds_per_State[state];
}

int MixerCreateState (unsigned char state, unsigned int snds)
{
	if (State_Sounds[state] != NULL)
		return MIXER_ALREADY_ALLOC;

	State_Sounds[state] = (short **)calloc (sizeof (short *), snds);
	State_Snd_Length[state] = (unsigned int *)calloc (sizeof (unsigned int),
																										snds);

	if (State_Sounds[state] == NULL || State_Snd_Length[state] == NULL)
		return MIXER_ALLOC_FAILED;

	return MIXER_SUCCESS;
}

int MixerAddStateSound (unsigned char state, unsigned int no_sound,
												short *sound, unsigned int length)
{
	if (State_Sounds[state][no_sound] != NULL)
		return MIXER_ALREADY_ALLOC;

	State_Sounds[state][no_sound] = sound;
	State_Snd_Length[state][no_sound] = length;

	return MIXER_SUCCESS;
}

int MixerAddSound (short *sound, unsigned int length,
									 unsigned char location, unsigned int channel)
{

	ASSERT (channel >= 0 && channel < No_Event_Chans);

	/* Is Channel Free? */
	if (Sound_Bufs[channel] != NULL)
		return 0;

	/* Do a Mutext Lock */
	ThreadLock (&Vlock);

	Sound_Bufs[channel] = sound;
	Buf_Length[channel] = length;
	Stereo_Left[channel] = location;

#ifdef DYNAMIC_VOLUME
	/* Increment the number of voices in use */
	No_Cur_Voices++;
	Dynamic_Mult[channel] = DynamicNewVol ();

#if DEBUG_LEVEL & DBG_MXR
	Log (DBG_MXR, "For new channel, using Dynamic Multiplier of: %f\n",
			 DYNAMIC_MULT (channel));
#endif
#endif

	ThreadUnlock (&Vlock);

#if DEBUG_LEVEL & DBG_MXR
	{
		int i, j, chans;
		double sum = 0;

		Log (DBG_MXR, "Adding sound to channel: %d...\n", channel);

		for (i = 0; i < No_Event_Chans; i += 5) {
			for (j = 0; i + j < No_Event_Chans && j < 5; j++) {
				Log (DBG_MXR, "%2d(%6d) ", i + j, Sound_Pos[i + j]);
			}
			Log (DBG_MXR, "\n");
		}

		for (i = 0, chans = 0; i < No_Event_Chans; i++) {
			if (Dynamic_Mult[i] != 0.0) {
				chans++;
			}
		}

		Log (DBG_MXR, "Total dynamic volume for %d channels are: %f \n", chans,
				 DynamicTotVolume ());
	}
#endif

	return 1;
}

void MixerRemoveSound (unsigned int j)
{
	ThreadLock (&Vlock);

	Sound_Bufs[j] = NULL;
	Buf_Length[j] = Sound_Pos[j] = Stereo_Left[j] = 0;

#ifdef DYNAMIC_VOLUME
	Dynamic_Mult[j] = 0.0;
	No_Cur_Voices--;
#endif

	ThreadUnlock (&Vlock);

	ThreadLock (&Tlock);

	EngineSchedulerInit (j, 0, 0, 0);

	ThreadUnlock (&Tlock);
}

void MixerInterruptChannel (int channel)
{

	/* Do a Mutex Lock */
	ThreadLock (&Vlock);

	ASSERT (channel >= 0 && channel < No_Event_Chans);

	Sound_Bufs[channel] = NULL;
	Buf_Length[channel] = Sound_Pos[channel] = Stereo_Left[channel] = 0;

	ThreadUnlock (&Vlock);

	ThreadLock (&Tlock);

	EngineSchedulerInit (channel, 0, 0, 0);

	ThreadUnlock (&Tlock);
}

#ifdef DYNAMIC_VOLUME

/* Functions for dynamic volume mixing */

static double DynamicGroupMult (void)
{
	unsigned int voices = MixerVoices (NULL);
	return (1.0 - (1.0 - 1.0 / voices) / (voices - 1) * (No_Cur_Voices - 1));
}

static double DynamicIndivMult (void)
{
	return (0.5 + ((No_Cur_Voices - 1) / (MixerVoices (NULL) - 1)) * 0.5);
}

static double DynamicTotVolume (void)
{
	double sum = 0.0;
	unsigned int i;

	for (i = 0; i < No_Event_Chans; i++)
		sum += DYNAMIC_MULT (i);

	return sum;
}

static double DynamicNewVol (void)
{
	double totvol = DynamicTotVolume ();
	double im = DynamicIndivMult ();
	double gm = DynamicGroupMult ();

	return (im * gm * (1.0 - totvol));
}

static double DynamicMin (double a, double b)
{
	if (a < b)
		return a;
	else
		return b;
}

/* End function for dynamic volume mixing */
#endif

void Mixer ()
{
	int i, j;
	Event *old_event;
	struct timeval tp;						/* Get time of day */
	double tp_conv;

	/* Do two at a time for stereo sound */
	for (i = 0; i < Chunk_Size; i += 2) {
		Output[i] = Output[i + 1] = 0;

		/* Handle Event Mixing */
		for (j = 0; j < No_Event_Chans; j++) {

			ASSERT (j >= 0 && j < No_Event_Chans);

			/* Should we skip over this sound? */
			if (Sound_Bufs[j] == NULL)
				continue;

			/* Sum the Sound and change its volume accordingly */
			Output[i] += (short)((double)Sound_Bufs[j][(Sound_Pos[j])++] *
													 DYNAMIC_MULT (j) * EVENT_MULT *
													 ((double)Stereo_Left[j] / 255.0));

			Output[i + 1] += (short)((double)Sound_Bufs[j][(Sound_Pos[j])++] *
															 DYNAMIC_MULT (j) * EVENT_MULT *
															 (1.0 - ((double)Stereo_Left[j] / 255.0)));

			/* Check and see if Sound is done. If sound is done, Check and see if
			 * there is sound in the queue. If so, Dequeue a sound from the queue
			 * Check whether it it's time limit hasn't expired. If it's ok, then
			 * Mix in the sound */
			if (Sound_Pos[j] == Buf_Length[j]) {

				/* Clean up after the old sound */
				MixerRemoveSound (j);

				/* If there's an event in the queue, let's put it in now */
				if (!MixerQueueEmpty ()) {
					old_event = MixerDequeue ();

					ASSERT (old_event != NULL);

					gettimeofday (&tp, NULL);
					tp_conv = TP_IN_FP_SECS (tp);
					if (tp_conv - TP_IN_FP_SECS (old_event->mix_in_time)
							< (double)QUEUE_EXPIRED) {

						int next_sound = (unsigned int)
							((double)EngineGetNoEventSnds (old_event->sound) * rand () /
							 (RAND_MAX + 1.0));

						/* Add the sound into the mixer for play */
						MixerAddSound (EngineGetEventSound (old_event->sound, next_sound),
													 EngineGetEventSoundLength (old_event->sound,
																											next_sound),
													 old_event->location, j);

						/* Update Sound data structures */
						gettimeofday (&tp, NULL);
						tp_conv = TP_IN_FP_SECS (tp);

						ThreadLock (&Tlock);

						ASSERT ((j + 1) >= 0 && (j + 1) < No_Event_Chans);

						EngineSchedulerInit (j, tp_conv,
																 tp_conv
																 /* + EngineGetIntelliMap(old_event->sound) */
																 ,
																 old_event->priority);

						ThreadUnlock (&Tlock);

						/* Free the event memory after done */
						cfree (old_event);

						/* Break outside the inner while */
						break;
					}

				}
			}
		}

		/* Handle State Mixing */
		for (j = 0; j < No_State_Chans; j++) {

			/* Check if the state volume is zero 'cause if it is, we don't want
			 * to increase the number we use to average the voices              */

#if DEBUG_LEVEL & DBG_ASSRT
			ASSERT (j >= 0 && j < No_State_Chans);
			ASSERT (Cur_Snd[j] >= 0 && Cur_Snd[j] <= Snds_per_State[j]);
			ASSERT (Cur_Snd_Pos[j] >= 0 && (State_Snd_Length[j][Cur_Snd[j]] == 0
																			|| Cur_Snd_Pos[j] <
																			State_Snd_Length[j][Cur_Snd[j]]));

			/* Log(DBG_MXR, "Cur_Snd_Pos: %d and State_Snd_Length: %d\nCur_Snd: %d and Snds_per_State: %d\n", 
			 * Cur_Snd_Pos[j], State_Snd_Length[j][Cur_Snd[j]], Cur_Snd[j], Snds_per_State[j]); */

#endif /* __DEBUG_WITH_ASSERT__ */

			if (State_Volumes[j] == 0.0 || State_Sounds[j][Cur_Snd[j]] == NULL)
				continue;
			else {

				/* Check to see if we need to start fading sounds in and out. This is
				 * set by the dither parameter, either in the configuration file or as
				 * send over the network.
				 * We compute the time remaining and check to see if we've reached the
				 * dither time with:
				 *     StateSndLength[j][CurSnd[j]]/2/44100      #Seconds in whole sound
				 *     -  CurSndPos[j]/2/44100                   #Seconds played so far
				 *     <  StateFade[j]                           #Seconds to start fading
				 * We simplify and do the following comparison:
				 * StateSndLength[j][CurSnd[j]] - CurSndPos[j] < StateFade[j]*2*SAMPLE_RATE 
				 *
				 * As long as we're above dither time, mix normally, else, incoporate the fade
				 * NOTE: the fade_point is calculated as: fade_point + (fade_point % 2). This
				 * is because we require an EVEN fade point for our comparisons since the 
				 * "distance" left to play is always an even number (because of stereo sound)
				 */
				int fade_point = (int)(State_Fade[j] * 2.0 * (double)SAMPLE_RATE);

				if ((State_Snd_Length[j][Cur_Snd[j]] - Cur_Snd_Pos[j]) >
						(fade_point + (fade_point % 2))) {

					Output[i] += (short)(State_Volumes[j] *
															 ((double)State_Stereo[j] / 255.0) *
															 (double)(State_Sounds[j][Cur_Snd[j]]
																				[(Cur_Snd_Pos[j]++)]));

					Output[i + 1] += (short)(State_Volumes[j] *
																	 (1.0 - ((double)State_Stereo[j] / 255.0)) *
																	 (double)(State_Sounds[j][Cur_Snd[j]]
																						[(Cur_Snd_Pos[j]++)]));

				}
				else {

					/* Begin fade out of current sound and fade in of new one. Grab the
					 * next random sound and begin mixing it in */
					if ((State_Snd_Length[j][Cur_Snd[j]] - Cur_Snd_Pos[j]) ==
							(fade_point + (fade_point % 2))) {

						Next_Snd[j] = (unsigned int)((double)Snds_per_State[j] * rand () /
																				 (RAND_MAX + 1.0));
						Next_Snd_Pos[j] = 0;
					}

					/* Now do the fade in mixing */
					Output[i] += (short)(State_Volumes[j] *
															 ((double)State_Stereo[j] / 255.0) *
															 MixerLinearFade (j, fade_point));

					Output[i + 1] += (short)(State_Volumes[j] *
																	 (1.0 - ((double)State_Stereo[j] / 255.0)) *
																	 MixerLinearFade (j, fade_point));

				}

				if (Cur_Snd_Pos[j] == State_Snd_Length[j][Cur_Snd[j]]) {

					/* The current sound and the current sound position become that of the next
					 * sound and sound position of the sound that we were fading in */
					Cur_Snd[j] = Next_Snd[j];
					Cur_Snd_Pos[j] = Next_Snd_Pos[j];
				}

			}
		}
	}

	/* Write out to Sound card */
	(void)PlaySoundChunk (Handle, (char *)Output, Chunk_Size * sizeof (short));
}

double MixerLinearFade (int n, int fade_point)
{

	/* We want to fade in linearly. From the first time to the last time
	 *  we enter this function, we know that:
	 * 0 <= (State_Snd_Length[j][Cur_Snd[j]] - Cur_Snd_Pos[j])/(fade_point+fade_point%2) <= 1
	 *
	 * At the beginning, this value is 1 and then linearly moves to 0 while the new
	 * randomly sound linearly moves from 0 to 1.
	 *
	 * Error handling for the a proper dither parameter is done by the engine and
	 * the event receiver as they get new events and examine them
	 */
	double mult = (double)(State_Snd_Length[n][Cur_Snd[n]] - Cur_Snd_Pos[n]) /
		(double)(fade_point + (fade_point % 2));
	double old_snd = (double)(State_Sounds[n][Cur_Snd[n]][Cur_Snd_Pos[n]]) *
		mult;
	double new_snd = (double)(State_Sounds[n][Next_Snd[n]][Next_Snd_Pos[n]]) *
		(1.0 - mult);

	ASSERT (old_snd + new_snd < MAXSHORT && old_snd + new_snd > MINSHORT);

	/* Advance to the next position in both */
	Cur_Snd_Pos[n]++;
	Next_Snd_Pos[n]++;

	return (old_snd + new_snd);
}

void MixerShutdown (void)
{
	int i, j;											/* Looping variable */

	/* Free up the event mixer datastructure */
	for (i = 0; i < No_Event_Chans; i++)
		cfree (Sound_Bufs[i]);

	cfree (Sound_Bufs);
	cfree (Buf_Length);
	cfree (Sound_Pos);
	cfree (Stereo_Left);

#ifdef DYNAMIC_VOLUME
	cfree (Dynamic_Mult);
#endif

	/* Free up the State datastructure */
	for (i = 0; i < No_State_Chans; i++) {
		for (j = 0; j < Snds_per_State[i]; j++)
			cfree (State_Sounds[i][j]);

		cfree (State_Snd_Length[i]);
		cfree (State_Sounds[i]);
	}
	cfree (State_Sounds);
	cfree (State_Snd_Length);

	cfree (Snds_per_State);
	cfree (State_Volumes);
	cfree (State_Stereo);
	cfree (Cur_Snd);
	cfree (Cur_Snd_Pos);
	cfree (State_Fade);
	cfree (Next_Snd);
	cfree (Next_Snd_Pos);

	/* Free the output array */
	cfree (Output);

	/* Close the sound device so it doesn't hang */
	CloseSoundDevice (Handle);
}
