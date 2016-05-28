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

#ifndef __PEEP_PARSER_H__
#define __PEEP_PARSER_H__

/* Loads the map between events and sounds into the SoundMap structure from
 * The peep.conf config file. Defaults to /etc/peep.conf unless specified
 * Otherwise on the command line. The sound files specified by the map file
 * Are loaded into memory in the appropriate arrays. The func will  load
 * Many mappings of each type (Event or State) as the number of channels
 * Based on voices will allow.
 * Returns True if successful or  logs an error message and returns False
 */
int LoadConfig(char *config_path);

int ParseClass(char *class, char *localhost, FILE * config);
int ParseEvents(FILE * config);
int ParseStates(FILE * config);

/* A thread safe tokenizing function, whose functionality is a little
 * better than a strtok on isspace() */
struct tok {
    char *token;
    char *remain_buf;
};

void Tokenize(struct tok *buf);

/* Loading functions, which are necessary to the process of parsing */
unsigned int GetFileSize(char *path);
short *LoadSoundFile(unsigned int *array_size, char *path);

#endif __PEEP_PARSER_H__
