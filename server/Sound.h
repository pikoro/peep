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

#ifndef __PEEP_SOUND_H__
#define __PEEP_SOUND_H__

#include <sys/time.h>

#ifdef __USING_ALSA_DRIVER__
/* Include these headers for ALSA definitions */

#include <sys/asoundlib.h>
#include <linux/asound.h>
#include <linux/asoundid.h>

#define AU_SOUND_FORMAT          SND_PCM_SFMT_MU_LAW
#define SIGNED_16_BIT            SND_PCM_SFMT_S16_LE
#define SOUND_WRONLY             SND_PCM_OPEN_PLAYBACK
#define SOUND_RDONLY             SND_PCM_OPEN_RECORD
#define SOUND_RDWR               SND_PCM_OPEN_DUPLEX
#endif

#ifdef __USING_DEVICE_DRIVER__
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __LINUX__
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#define AU_SOUND_FORMAT          AFMT_MU_LAW
#define SIGNED_16_BIT            AFMT_S16_LE
#define SOUND_WRONLY             O_WRONLY
#define SOUND_RDONLY             O_RDONLY
#define SOUND_RDWR               O_RDWR
#endif
#ifdef __SOLARIS__
#include <sys/audioio.h>
#define AU_SOUND_FORMAT          AUDIO_ENCODING_U_LAW
#define SIGNED_16_BIT            AUDIO_ENCODING_LINEAR
#define SOUND_WRONLY             O_WRONLY
#define SOUND_RDONLY             O_RDONLY
#define SOUND_RDWR               O_RDWR
#endif

#endif

/* Contains information about the capabilities of the sound card */
struct sound_card_info {
    unsigned int min_rate; /* Minimum sampling rate supported */
    unsigned int max_rate; /* Maximum rate supported by the card */
    unsigned int max_channels; /* Maximum number of voices on the soundcard */
    unsigned int buffer_size; /* Size of the playback buffer */
};

/* Describes the current status of the sound card */
struct sound_status {
    unsigned int actual_rate; /* Real playback rate - Hardware limits */
    int free_byte_count; /* Cound of bytes that could be written
																 * Without blocking in the queue */
    int bytes_in_queue; /* The number of bytes waititng to be played
																 * By the sound device */
    struct timeval future_play_time; /* Estimated time when the sample will play */
    struct timeval start_time; /* Time when playing of the sample started */
};

/* Function Declarations */
void *InitSoundCard(void *card, int device, int mode);
int SetSoundFormat(void *handle, unsigned int format_type,
        unsigned int sample_rate, unsigned int channels,
        unsigned int port);
struct sound_card_info *GetSoundCardInfo(void *handle);
struct sound_status *GetSoundStatus(void *handle);
ssize_t PlaySoundChunk(void *handle, char *buffer, unsigned long size);
void CloseSoundDevice(void *handle);

#endif __PEEP_SOUND_H__
