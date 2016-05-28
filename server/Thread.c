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
#include "Thread.h"

int StartThread (ThreadFunc func, void *data, pthread_t * handle)
{
	int rc;

	pthread_attr_t attr;

	rc = pthread_attr_init (&attr);
	rc = pthread_create (handle, &attr, func, data);
	pthread_attr_destroy (&attr);

	return rc;
}

void ThreadLock (pthread_mutex_t * l)
{
	pthread_mutex_lock (l);
}

void ThreadUnlock (pthread_mutex_t * l)
{
	pthread_mutex_unlock (l);
}

void ThreadSleep (unsigned long n)
{
	usleep (n);
}

void ThreadKill (pthread_t thread)
{
	pthread_cancel (thread);
}
