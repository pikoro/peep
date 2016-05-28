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

#ifdef __USING_ALSA_DRIVER__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "Sound.h"
#include "Main.h"
#include "Debug.h"

extern int errno;

/* Initializes the sound card for a specific mode.
 * Returns a Handle that refers to the soundcard */
void *InitSoundCard(void *card, int device, int mode) {
    void *handle; /* The Handle that refers to the soundcard */
    int err;

    if ((err = snd_pcm_open((void *) &handle, *(int *) card, device, mode)) < 0) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Couldn't open sound Device: %s\n", s);
        Shutdown();
    }

    return (handle);
}

/* Gets information concerning the capabilities of the sound card
 * Returns a ptr to an info structure if successful, otherwise NULL */
struct sound_card_info *GetSoundCardInfo(void *handle) {
    snd_pcm_playback_info_t *info; /* Structure for call to Sound API */
    static struct sound_card_info s;
    int err;

    if ((err = snd_pcm_playback_info(handle, info)) < 0) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Couldn't get sound info: %s\n", s);
        return (NULL);
    }

    s.min_rate = info->min_rate;
    s.max_rate = info->max_rate;
    s.max_channels = info->max_channels;
    s.buffer_size = info->buffer_size;

    return (&s);
}

/* Gets current status information about the sound queue of the sound card
 * Returns a ptr to a sound status structure if successful, else NULL */
struct sound_status *GetSoundStatus(void *handle) {
    snd_pcm_playback_status_t *status; /* Structure for call to Sound API */
    static struct sound_status s;
    int err;

    if ((err = snd_pcm_playback_status(handle, status)) < 0) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Couldn't get status info: %s\n", s);
        return (NULL);
    }

    s.actual_rate = status->rate;
    s.free_byte_count = status->count;
    s.bytes_in_queue = status->queue;
    s.future_play_time = status->time;
    s.start_time = status->stime;

    return (&s);
}

/* Sets the parameters on the soundcard for sampling rate, as well as what
 * voice to use to play the sound. This function must be called everytime
 * a different sound file type/sampled rate is used or when the sound must
 * be played through a different channel. 
 * Returns true if successful, false otherwise */
boolean SetSoundFormat(void *handle, unsigned int format_type,
        unsigned int sample_rate, unsigned int channels,
        unsigned int port) {
    int err;
    snd_pcm_format_t format; /* Structure to hold format information
																 * for playback of sound sample */
    bzero(&format, sizeof (format));

    format.format = format_type;
    format.rate = sample_rate;
    format.channels = channels;

    if ((err = snd_pcm_playback_format(handle, &format)) < 0) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Couldn't set format: %s", s);
        return 0; /* Return False */
    } else
        return 1; /* Return True - No error */
}

/* Sends a chunk of buffer into the sound queue to be played. Sound plays
 * On the channel and at the sample rate specified by the SetSoundFormat
 * Function.
 * Returns number of bytes successfully written to device or err (neg.)   */
ssize_t PlaySoundChunk(void *handle, char *buffer, unsigned long size) {
    return (snd_pcm_write(handle, buffer, size));
}

/* Closes a sound file handle                                             */
void CloseSoundDevice(void *handle) {
    snd_pcm_close(handle);
}

#endif
