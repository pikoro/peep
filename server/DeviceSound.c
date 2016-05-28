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

#ifdef __USING_DEVICE_DRIVER__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "Sound.h"
#include "Main.h"
#include "Debug.h"

extern int errno;

/* Initializes the sound card. Here, void card is really a pointer to
 * the device string. int device is not used. The function returns a
 * file handle number. */
void *InitSoundCard(void *card, int device, int mode) {
    char *device_name = (char *) card;
    static int audio_fd;

    if ((audio_fd = open(device_name, mode, 0)) == -1) {
        char *s = strerror(errno);
        /* Opening device failed */
        Log(DBG_GEN, "Couldn't open the sound device: %s\n", s);
        Shutdown();
    }

    return (&audio_fd);
}

/* Sets the parameters on the soundcard for sampling rate, as well as what
 * voice to use to play the sound. This function must be called everytime
 * a different sound file type/sampled rate is used or when the sound must
 * be played through a different channel.
 * Returns true if successful, false otherwise */
int SetSoundFormat(void *handle, unsigned int format_type,
        unsigned int sample_rate, unsigned int channels,
        unsigned int port) {

#ifdef __LINUX__

    /* Set Sound format */
    if (ioctl(*(int *) handle, SNDCTL_DSP_SETFMT, &format_type) == -1) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Couldn't set format: %s\n", s);
        return 0;
    }

    /* Select the number of channels */
    if (ioctl(*(int *) handle, SNDCTL_DSP_CHANNELS, &channels) == -1) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Couldn't select number of channels: %s\n", s);
        return 0;
    }

    /* Set the sample rate */
    if (ioctl(*(int *) handle, SNDCTL_DSP_SPEED, &sample_rate) == -1) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Couldn't set the sample rate: %s\n", s);
        return 0;
    }

#endif
#ifdef __SOLARIS__
    audio_info_t info;

    AUDIO_INITINFO(&info);

    info.play.encoding = format_type;
    info.play.channels = channels;
    info.play.sample_rate = sample_rate;
    info.play.port = port; /* 1 = speaker, 2 = jack */

    if (ioctl(*(int *) handle, AUDIO_SETINFO, &info) == -1) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Couldn't set sound format: %s\n", s);
        return 0;
    }

#endif

    return 1; /* Set the sound format with no error */
}

/* Gets information concerning the capabilities of the sound card
 * Returns a ptr to an info structure if successful, otherwise NULL */
struct sound_card_info *GetSoundCardInfo(void *handle) {
    /* Unimplemented - Not sure if this is possible with a device
     * driver */
}

/* Gets current status information about the sound queue of the sound card.
 * Returns a ptr to a sound status structure if successful, else NULL */
struct sound_status *GetSoundStatus(void *handle) {
    /* Unimplemented - Not sure if this is possible with a device
     * driver */
}

/* Sends a chunk of buffer into the sound queue to be played. Sound plays
 * On the channel and at the sample rate specified by the SetSoundFormat
 * Function. void *Handle must be coverted to the integer file handle.
 * Returns number of bytes successfully written to device or err (neg.)  */
ssize_t PlaySoundChunk(void *handle, char *buffer, unsigned long size) {
    return (write(*(int *) handle, buffer, size));
}

/* Closes a sound file handle */
void CloseSoundDevice(void *handle) {
    close(*(int *) handle);
}

#endif
