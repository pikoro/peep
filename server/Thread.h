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

#ifndef __PEEP_THREAD_H__
#define __PEEP_THREAD_H__

#include <pthread.h>

typedef void *(*ThreadFunc) (void *data);

/* Begin a new Thread */
int StartThread (ThreadFunc func, void *data, pthread_t * handle);

/* Functions to lock and unlock threads */
void ThreadLock (pthread_mutex_t * l);
void ThreadUnlock (pthread_mutex_t * l);

/* Make a thread sleep */
void ThreadSleep (unsigned long n);

/* Kill a thread */
void ThreadKill (pthread_t thread);

#endif __PEEP_THREAD_H__
