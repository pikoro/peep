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
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "Main.h"
#include "Parser.h"
#include "EngineQueue.h"
#include "Playback.h"
#include "Server.h"
#include "VoiceMixer.h"
#include "Debug.h"
#include "Copyright.h"

/* Initialize defaults */
static char *Config_File = DEFAULT_CONFIG_PATH;
static int Card = 0; /* Default sound card params - ALSA driver specific */
static char *Device = DEFAULT_DEVICE_PATH; /* Path to the device to open */

static unsigned int Port = DEFAULT_PORT_NO; /* Default port to use */
unsigned int Sound_Port = 2; /* Sound port option for the suns, default the jack */

static FILE *Pid_File = NULL;
static char *Pid_File_Path = NULL;

/* The engine and mixer thread */
static pthread_t Ethread, Mthread;

extern int errno;

int main(int argc, char *argv[]) {
    char *string_ptr = NULL; /* String Pointer to work on misc strings */
    char *cmd_device = NULL; /* To get the path to the device off the cmd line */
    char *log_file = NULL; /* The local copy of logfile, read from arguments */
    char *pid_file_path = NULL; /* THe local copy of the pidfile, read from args */
    int Daemon_Mode = 1; /* Start in daemon mode */
    /* For playback */
    char *playback_file_path = NULL, *start_t = NULL, *end_t = NULL;

    pid_t pid; /* The PID our server */
    int i;

    /* Handle signal functions */
    signal(SIGINT, HandleInterrupt);

    /* Process Arguments. For a list of which arguments do what, see char *Usage,
     * found later in the file */
    while (argc > 1) {
        if (argv[1][0] != '-') {
            Usage();
            exit(1);
        }

        /* Deal with arguments of type -- else deal with switchs of type - */
        if (argv[1][1] == '-') {

            /* Position to beginning of string */
            string_ptr = &(argv[1][2]);

            /* Check if they want help */
            if (strcmp(string_ptr, "help") == 0) {
                Usage();
                exit(1);
            }

            /* Check if we're not supposed to run in daemon mode */
            if (strcmp(string_ptr, "nodaemon") == 0)
                Daemon_Mode = 0;

            /* Set ourselves in playback mode */
            if (strcmp(string_ptr, "playback-mode") == 0) {
                int x = 1;
                PlaybackMode m = PLAYBACK_MODE;

                PlaybackModeOn(&x);
                PlaybackSetMode(&m);
            }

            /* Set ourselves in recording mode */
            if (strcmp(string_ptr, "record-mode") == 0) {
                int x = 1;
                PlaybackMode m = RECORD_MODE;

                PlaybackModeOn(&x);
                PlaybackSetMode(&m);
            }

        } else {
            switch (argv[1][1]) {

                case 'c':
                    GET_INT_ARG(Card)
                    break;

                case 'd':
                    GET_STRING_ARG(cmd_device)
                    Device = cmd_device;
                    break;

                case 'i':
                    GET_STRING_ARG(pid_file_path)
                    break;

                case 'j':
                    GET_INT_ARG(Sound_Port)
                    break;

                case 'p':
                    GET_INT_ARG(Port)
                    break;

                case 'S':
                    GET_STRING_ARG(Config_File)
                    break;

                case 's':
                    GET_STRING_ARG(Config_File)
                    break;

                case 'V':
                {
                    int voices;
                    GET_INT_ARG(voices)

                    MixerVoices(&voices);
                }
                    break;

                case 'l':
                    GET_STRING_ARG(log_file)
                    break;

                case 'f':
                    GET_STRING_ARG(playback_file_path)
                    break;

                case 't':
                    GET_STRING_ARG(start_t)
                    break;

                case 'e':
                    GET_STRING_ARG(end_t)
                    break;

                default:
                    fprintf(stderr, "Bad Option: %s\n", argv[1]);
                    Usage();
                    exit(1);
            }
        }

        /* Proceed to next argument */
        GET_NEXT_ARG
    }

    /* Init Routines */

    /* Check if we're to run in daemon mode */
    if (Daemon_Mode) {
        if (fork()) {
            /* We're the parent, so let's detach ourselves */
            fclose(stdin);
            fclose(stdout);
            fclose(stderr);
            exit(0);
        }
        /* Else we're the child, so let's continue. Let's write our
         * pid out into the designated file */

        pid = getpid();

        /* Copy our local path into the global, so we can unlink it later */
        if (pid_file_path) {
            Pid_File_Path =
                    (char *) calloc(sizeof (char), strlen(pid_file_path) + 1);
            strcpy(Pid_File_Path, pid_file_path);
        } else {
            char *str = DEFAULT_PID_FILE;

            Pid_File_Path = (char *) calloc(sizeof (char), strlen(str) + 1);
            strcpy(Pid_File_Path, str);
        }

        if ((Pid_File = fopen(Pid_File_Path, "w")) == NULL) {
            /* Format our error string nicely */
            char s[256];

            strcpy(s, "Couldn't write pid out to file: ");
            sprintf(s, "%s %s", s, Pid_File_Path);
            perror(s);

            /* Continue anyway... */
            fprintf(stderr, "Continuing with pid: %d\n", pid);
        } else {
            /* Now write the pid out to the file */
            fprintf(Pid_File, "%d\n", pid);
            fflush(Pid_File);
        }
    }

    /* Initialize logging routines */
    if (log_file) {
        if (!LogInit(log_file)) {
            fprintf(stderr, "Couldn't open logfile Exiting...\n");
            exit(1);
        }
    } else {
        /* If there's no logfile, we init with NULL to use stderr
         * and clear the console */
        LogInit(NULL);
    }

    Log(DBG_DEF, "\n===========================================\n");
    Log(DBG_DEF, "Welcome to PEEP (the network auralizer).\n");
    Log(DBG_DEF, "Version %s\n", VERSION);
    Log(DBG_DEF, "Copyright (C) 2000 Michael Gilfix\n");
    Log(DBG_DEF, "===========================================\n\n");

    /* Check if we're higher than lowest level, which is really debugging
     * turned on */
    if (DEBUG_LEVEL & ~DBG_LOWEST) {
        char str[255];
        strcpy(str, "DEBUG MODE ON! DEBUG SEVERITY ");

        switch (DEBUG_LEVEL) {

            case DBG_LOWER:
                strcat(str, "'LOWER'");
                break;

            case DBG_MEDIUM:
                strcat(str, "'MEDIUM'");
                break;

            case DBG_HIGHER:
                strcat(str, "'HIGHER'");
                break;

            case DBG_HIGHEST:
                strcat(str, "'HIGHEST'");
                break;

            case DBG_ALL_W_ASSERT:
                strcat(str, "'HIGHEST WITH ASSERTIONS!'");
                break;

            default:
                strcat(str, "'DEBUGGING LEVEL UNSUPPORTED'");
        }

        Log(DBG_DEF, "%s\n\n", str);
    }

    Log(DBG_DEF, "Initializing Engine...\n");
    Log(DBG_DEF, "Used %u voices.\n", MixerVoices(NULL));

#ifdef DYNAMIC_VOLUME
    Log(DBG_DEF, "Using Dynamic Volume Mixing...\n");
#else
    Log(DBG_DEF, "Using Static Volume Mixing...\n");
#endif

    EngineInit(Card, Device);

    Log(DBG_DEF, "Parsing peep.conf and Sound Samples into memory...\n");
    Log(DBG_DEF,
            "Depending on your machine and sound files, this may take a while.\n");
    if (!LoadConfig(Config_File)) {
        Log
                (DBG_GEN,
                "Uh Oh! peep.conf not properly configured or error loading sound files");
        Usage();
        exit(1);
    }

    Log(DBG_DEF, "Finished loading configuration information.\n");
    Log(DBG_DEF, "Starting Mixer thread...\n");

    StartThread(DoMixing, 0, &Mthread);

    Log(DBG_DEF, "Starting Engine thread...\n");

    StartThread(EnginePoll, 0, &Ethread);

    /* Check if playback mode is on and initialize if so */
    if (PlaybackModeOn(NULL)) {
        /* Check if we should use the default */
        if (playback_file_path == NULL)
            playback_file_path = DEFAULT_PLAYBACK_FILE;

        Log(DBG_DEF, "Initializing playback file...\n");

        if (!PlaybackFileInit(playback_file_path)) {
            Log(DBG_GEN,
                    "Uh Oh! Couldn't successfully enter playback mode. Giving up.\n");
            Shutdown();
        }
    }

    /* Now check if we're in playback mode since if we're in playback
     * mode, we don't want to start up the server */
    if (PlaybackModeOn(NULL) && PlaybackSetMode(NULL) == PLAYBACK_MODE) {
        Log(DBG_DEF, "Entering playback mode...\n");
        GoPlaybackMode(start_t, end_t);
    } else {
        Log(DBG_DEF, "Initializing server thread...\n");

        if (PlaybackModeOn(NULL) && PlaybackSetMode(NULL) == RECORD_MODE)
            Log(DBG_DEF, "Recording mode on - Recording incoming events to: %s\n",
                playback_file_path);

        if (!ServerInit(Port)) {
            Log(DBG_GEN, "Server setup failed!");
            Shutdown();
        }

        Log(DBG_DEF, "Starting Server thread...\n");

        ServerStart();
    }
}

/* Mixing Thread */
void *DoMixing(void *data) {
    while (1) {
        Mixer();

        /* Wait between next mix */
        usleep(MIXER_THREAD_USLEEP);
    }
}

/* 
 * The engine polling thread
 * Checks the event queue to see if any new events have arrived
 * via UDP and then processes them if found
 */
void *EnginePoll(void *data) {
    Event *client_event;

    while (1) {
        if (!EngineQueueEmpty()) {
            client_event = (Event *) calloc(sizeof (Event), 1);
            *client_event = EngineDequeue();

#if DEBUG_LEVEL & DBG_SRVR
            Log(DBG_SRVR,
                    "\nReceived Event:\nEvent.type -> %d\nEvent.sound -> %d\n",
                    client_event->type, client_event->sound);
            Log(DBG_SRVR,
                    "Event->location -> %d\nEvent->volume -> %lf\nEvent->priority -> %d\n",
                    client_event->location, (double) client_event->volume / 255.0,
                    client_event->priority);
            Log(DBG_SRVR, "Event->dither -> %d\n", client_event->dither);
            Log(DBG_SRVR, "Event->mix_in_time -> %d\n\n",
                    client_event->mix_in_time);
#endif

            /* Grab event requests that are out of range */
            if (client_event->type == EVENT) {
                if (client_event->sound >= EngineGetEventMapSize()) {

#if DEBUG_LEVEL & DBG_SRVR
                    Log(DBG_SRVR, "Received event out of range: %d. Ignoring...\n",
                            client_event->sound);
#endif

                    continue;
                }
            } else {

                /* Need to check whether the state sound exists in both cases */
                if (client_event->sound >= MixerStateChans()
                        && !MixerExistsStateSound(client_event->sound)
                        && !MixerLoadedStateSound(client_event->sound, 0)) {

#if DEBUG_LEVEL & DBG_SRVR
                    Log(DBG_SRVR, "Received state out of range: %d. Ignoring... \n",
                            client_event->sound);
#endif

                    continue;
                }
            }

            /* Now handle the input */
            EngineIO(client_event);
        } else
            usleep(ENGINE_POLL_USLEEP);
    }
}

/* General shutdown (aliased to the handler for a SIGINT */
void Shutdown(void) {
    HandleInterrupt();
}

/* Catch a signal that tells us to shutdown */
void HandleInterrupt() {
    /* Kill the threads */
    ThreadKill(Ethread);
    ThreadKill(Mthread);

    /* Cleanup */
    Log(DBG_DEF, "Shutting down Engine...\n");
    EngineShutdown();

    Log(DBG_DEF, "Shutting down Mixer...\n");
    MixerShutdown();

    Log(DBG_DEF, "Shutting down Server...\n");
    ServerShutdown();

    if (PlaybackModeOn(NULL)) {
        Log(DBG_DEF, "Closing playback file...\n");
        PlaybackFileShut();
    }

    Log(DBG_DEF, "Exiting...\n");

    /* Close logging routines */
    LogClose();

    /* Check if we have a pid file to close, and if so, close it
     * and unlink it */
    if (Pid_File) {
        if (fclose(Pid_File) != 0)
            perror("Error closing server log file");

        if (unlink(Pid_File_Path) < 0)
            perror("Couldn't unlink pid file");
    }

    /* Exit normally */
    exit(0);
}

static char *Usage_String =
        "
        Usage of Peep :
        Peep [OPTS]

        Arguments :
        General Options :
        -p [PORT NUM] : The port number for the server process
        - s [PATH] :
        -S [PATH] : The path to peep.conf(defaults to DEFAULT_CONFIG_PATH)
    - V [VOICES] : Number of mixing voices
        - l [PATH] : File for logging(defaults to stderr)
        - i [PATH] : File to store pid(defaults to DEFAULT_PID_FILE)

        Playback Options :
            -f [PATH] : Recording file to use(defaults to eventlog)
        - t [DATE] : Starting date / time(playback only)
        - e [DATE] : Ending date / time(playback only)

        Alsa driver options :

            -c [CARD NO] : The card number of the sound device

            Device driver options :

            -d DEVICE_PATH : The path to device to open
            - j SOUND_PORT : Solaris only : 1 = speaker, 2 = jack, Defaults to 2

            Server Modes :

            --playback - mode Go into playback mode(must be used in conjunction with - f)
        --record - mode Record incoming events(must be used in conjunction wth - f)
        --nodaemon Don't run in daemon mode
            --help Displays this list of proper usage.
            ";

            /* Prints the usage of the program */
            void Usage(void){
            fprintf(stderr, "%s", Usage_String);
        }
