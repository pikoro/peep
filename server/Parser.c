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
#include <unistd.h>
#include <sys/stat.h>
#include <values.h>
#include "Parser.h"
#include "Server.h"
#include "VoiceMixer.h"

#include <errno.h>
#include <string.h>
#include "Debug.h"

extern int errno;

int LoadConfig(char *config_path) {
    FILE *config_file, *snd_file;
    char ret[256], localhost[PROT_MAXHOSTNAME];
    struct tok tok;

    /* Grab the hostname so we'll recognize it in the config file */
    if (gethostname(localhost, PROT_MAXHOSTNAME) < 0) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Uh Oh! Couldn't get my hostname: %s\n", s);
        return 0;
    }

    if ((config_file = fopen(config_path, "r")) == NULL) {
        Log(DBG_GEN, "Couldn't find peep.conf at: %s\n", config_path);
        return 0;
    }

    while (fgets(ret, sizeof (ret), config_file)) {

        /* Skip over # comments and blank lines */
        if (ret[0] == '#' || ret[0] == '\n')
            continue;

        /* Now Tokenize the line */
        tok.remain_buf = ret;
        Tokenize(&tok);

        /* Are we at the end? */
        if (tok.token == NULL) {
            return 1;
        }

        if (strcmp(tok.token, "class") == 0) {
            char *classname;

            Tokenize(&tok);
            classname = tok.token;

            if (!ParseClass(classname, localhost, config_file)) {
                Log(DBG_GEN, "Error parsing class %s...\n", classname);
                return 0;
            }
        }

        if (strcmp(tok.token, "client") == 0) {
            /* this is relevant to us, so let's skip to the closing brace */
            while (fgets(ret, sizeof (ret), config_file) != NULL) {
                tok.remain_buf = ret;
                Tokenize(&tok);

                /* Make sure that we've hit an "end client" */
                if (strcmp(tok.token, "end") == 0) {
                    Tokenize(&tok);
                    if (strcmp(tok.token, "client") == 0) {
                        break;
                    }
                }
            }
        }

        if (strcmp(tok.token, "events") == 0) {
            if (!ParseEvents(config_file)) {
                Log(DBG_GEN, "Error parsing events...\n");
                return 0;
            }
        }

        if (strcmp(tok.token, "states") == 0) {
            if (!ParseStates(config_file)) {
                Log(DBG_GEN, "Error parsing states...\n");
            }
        }

    }

    /* If we've made it this far, succeed */
    return 1;
}

/* Parses a class and loads the class info into the necessary
 * data structures */
int ParseClass(char *class, char *localhost, FILE * config) {
    struct tok tok;
    char ret[256];
    int idflag = 0;

    while (fgets(ret, sizeof (ret), config)) {
        tok.remain_buf = ret;
        Tokenize(&tok);

        /* Skip hashes */
        if (tok.token[0] == '#')
            continue;

        if (strcmp(tok.token, "broadcast") == 0) {

            while (1) {
                Tokenize(&tok);

                if (tok.remain_buf == NULL)
                    break;

                /* Now split the port based on the ":" */
                {
                    char *p = tok.token, *q = tok.token;
                    while (*q != '\0') {
                        if (*q == ':') {
                            *q = '\0';

                            if (ServerAddBroadcastZone(p, atoi(++q)) < 0) {

                                Log(DBG_GEN, "Too many broadcast zones. Ignoring broadcast to: %s:%d\n", p, atoi(q));

                            }

                            break;
                        }

                        q++;
                    }
                }

            }
        }

        if (strcmp(tok.token, "server") == 0) {

            while (1) {
                Tokenize(&tok);

                if (tok.remain_buf == NULL)
                    break;

                /* Split the port, then check the server name to see if we're it */
                {
                    char *p = tok.token, *q = tok.token;

                    while (*q != '\0') {

                        if (*q == ':') {
                            *q = '\0';

                            if (strcmp(p, localhost) == 0) {

                                ServerAddIdentifier(class);
                                ServerSetPort(atoi(++q));
                                break;
                            }
                        }

                        q++;
                    }
                }

            }
        }

        if (strcmp(tok.token, "end") == 0)
            return 1;

    }
}

/* Parses the list of events from the configuration file, and loads the necessary
 * information into the data structures.
 * In this case, this means loading files */
int ParseEvents(FILE * config) {
    struct tok tok;
    char ret[256], event_name[256], event_path[256];
    fpos_t cur_pos;
    int events = 0, err, q, mappings = 0;
    unsigned int no_event_snds = 0, length = 0;
    short *sound = NULL;

    fgetpos(config, &cur_pos);

    /* Count the number of events in the config file and allocate the map array */
    for (mappings = 0; fgets(ret, sizeof (ret), config); mappings++) {

        if (ret[0] == '#')
            mappings--;
        tok.remain_buf = ret;
        Tokenize(&tok);

        if (strcmp(tok.token, "end") == 0)
            break;
    }

    /* Tell the engine about the number of event maps */
    EngineSetEventMapSize(mappings);

    /* Allocate the event arrays and reset the file position */
    EngineCreateIntelliMap(mappings);
    EngineAllocEventSounds(mappings);

    if (fsetpos(config, &cur_pos) != 0) {
        Log(DBG_GEN, "Error accessing peep.conf file. Couldn't fsetpos...\n");
        return 0;
    }

    while (fgets(ret, sizeof (ret), config)) {

        if (ret[0] == '#')
            continue;

        /* Let's look for the end, make a copy to tokenize */
        {
            char tmp[256];

            strcpy(tmp, ret);
            tok.remain_buf = tmp;
            Tokenize(&tok);

            if (strcmp(tok.token, "end") == 0) {
                return 1;
            }
        }

        if (events >= MixerEventChans()) {
            Log(DBG_DEF, "Too many event sounds in peep.conf: skipped: %d\n",
                    (events++) - MixerEventChans());
            continue;
        }

        (void) sscanf(ret, "%s%s%d", &event_name, &event_path, &no_event_snds);

        EngineSetNoEventSnds(events, no_event_snds);

        /* We used to read a parameter for the intelligeable time of a sound but
         * but we don't anymore. This option is currently unused. Set it to it's
         * max possible time so that it doesn't affect us */
        EngineSetIntelliMap(events, MAXDOUBLE);

        EngineCreateEventSoundEntry(events, EngineGetNoEventSnds(events));

        /* Go to the end of our path string so we can set the .* in the peep.conf
         * path to the appropriate number */
        for (err = 0; event_path[err] != '\0'; err++);

        for (q = 0; q < no_event_snds; q++) {
            char event_temp[256];

            strcpy(event_temp, event_path);
            sprintf(event_temp + err - 1, "%02d", q + 1);
            sound = LoadSoundFile(&length, event_temp);
            EngineAssignEventSoundEntry(events, q, sound, length);
        }

        events++;
    }
}

/* Parse a list of state sounds and load them into the necessary data structures
 * This also involves the loading of sounds files */
int ParseStates(FILE * config) {
    char ret[256], state_name[256], state_path[256], state_temp[256];
    int states = 0, err, q;
    unsigned int length = 0, snds_per_state = 0;
    short *sound = NULL;
    double fade;

    while (fgets(ret, sizeof (ret), config)) {

        if (ret[0] == '#')
            continue;

        /* Check if we've reached the end */
        {
            struct tok tok;
            char line[256];

            strcpy(line, ret);
            tok.remain_buf = line;
            Tokenize(&tok);

            if (strcmp(tok.token, "end") == 0) {
                /* Just keep track of the number of states we actually loaded */
                MixerLoadedStates(&states);
                return 1;
            }
        }

        if (states >= MixerStateChans()) {
            Log(DBG_DEF, "Too many state sounds in peep.conf: Skipped: %d\n",
                    (states++) - MixerStateChans());
            continue;
        }

        (void) sscanf(ret, "%s%s%d%lf", &state_name, &state_path, &snds_per_state, &fade);

        MixerSetSoundsPerState(states, snds_per_state);

        MixerStateFade(states, &fade);

        /* Allocate the StateSound arrays based on the number of state sounds
         * we have. This affects the StateSounds and the StateSndLength arrays */
        MixerCreateState(states, snds_per_state);

        /* Go to the end of our path string so we can set the .* in the peep.conf
         * path to the appropriate number */
        for (err = 0; state_path[err] != '\0'; err++);

        for (q = 0; q < snds_per_state; q++) {
            strcpy(state_temp, state_path);
            sprintf(state_temp + err - 1, "%02d", q + 1);
            sound = LoadSoundFile(&length, state_temp);
            MixerAddStateSound(states, q, sound, length);
        }

        states++;
    }
}

/* Takes a token structure and tokens the string in "remain_buf", putting
 * the new token in the "token" member and reassigned "remain_buf" to the
 * remainder.
 * We don't use strtok cause it's not threadsafe */
void Tokenize(struct tok *buf) {
    char *p, *q;

    /* Skip leading white space */
    for (p = buf->remain_buf; isspace(*p); p++);

    for (q = p; q; q++) {

        if (isspace(*q) || *q == '\n') {
            *q = '\0';
            q++;

            /* Eat remainder */
            while (isspace(*q)) {
                q++;
            }
            buf->remain_buf = q;
            buf->token = p;
            return;
        }

        if (*q == '\0') {
            buf->remain_buf = NULL;
            buf->token = p;
            return;
        }

    }
}

/* Returns the size in bytes of the file found at Path */
unsigned int GetFileSize(char *path) {
    struct stat file_stat;
    stat(path, &file_stat);
    return (file_stat.st_size);
}

/* Load a sound file at a given path into a buffer in memory */
short *LoadSoundFile(unsigned int *array_size, char *path) {
    int i;
    short *snd;
    FILE *infile;

    if ((infile = fopen(path, "r")) == NULL) {
        Log(DBG_DEF, "WARNING: Attempted to load non-existant sound file: %s\n",
                path);
        Log(DBG_DEF, "All attempts to play this sound file will be ignored.\n");
        return (NULL);
    }

    /* Length is number of bytes /2 for shorts */
    *array_size = GetFileSize(path) / 2;
    snd = (short *) calloc(*array_size, sizeof (short));

    /* Load Sound into memory */
    for (i = 0; i < *array_size; i++) {
        (void) fread(&(snd[i]), 1, sizeof (short), infile);

#ifdef WORDS_BIGENDIAN
        /* Switch the byte order as we load */
        snd[i] = ((snd[i] >> 8) & 0xff) | (snd[i] << 8);
#endif

#ifdef STATIC_VOLUME
        /* Divide our sound by the number of voices we're using
         * to avoid sound overflow */
        snd[i] /= MixerVoices(NULL);
#endif
    }

    Log(DBG_DEF, "Loaded: %s... Size: %d\n", path, (2 * *array_size));

    return (snd);
}
