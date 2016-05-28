/*
 * PEEP: The Network Auralizer
 * Copyright (C) 2000 Michael Gilfix
 *
 * This file is part of PEEP.
 *
 * You should have received a file COPYING containing license terms
 * along with this program; if not, write to Michael Gilfix
 * (mgilfix@eecs.tufts.edu) for a copy.
 *
 * This version of PEEP is open source; you can redistribute it and/or
 * modify it under the terms listed in the file COPYING.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef __PEEP_SERVER_H__
#define __PEEP_SERVER_H__

#define PROT_MAJORVER 1
#define PROT_MINORVER 0

#define PROT_BCSERVER (0)
#define PROT_BCCLIENT (1)
#define PROT_SERVERSTILLALIVE (2)
#define PROT_CLIENTSTILLALIVE (3)
#define PROT_CLIENTEVENT (4)

#define PROT_STRLEN 128
#define PROT_CLASSDELIM "!"

#define PROT_SERVERLEASEMIN 5
#define PROT_SERVERLEASESEC 0
#define PROT_SERVERWAKEUPMIN 4
#define PROT_SERVERWAKEUPSEC 30
#define PROT_WAKEUPSBEFOREEXPIRED 2

/* We'll need this header for our time definitions */
#include <time.h>

/* Peep leasing is a very low resolution leasing and is not meant to be
 * used for long periods of time. After lease time has expired, the 
 * information associated with the broadcast should be considered
 * obsolete 
 */
struct lease
{
	unsigned char min;
	unsigned char sec;
};

/* We need the definition of an event to declare this packet
 * structure */
#include "Engine.h"

/* Protocol for internet transmissions */
typedef struct
{
	unsigned char majorver;				/* major protocol version */
	unsigned char minorver;				/* minor protocol version */
	unsigned char type;						/* type of packet */
	unsigned char reserved;				/* reserved for future use */
	union
	{
		struct
		{
			struct lease lease;
			char infostring[PROT_STRLEN];
		}
		bc_server;									/* For server broadcasts */
		struct
		{
			struct lease lease;
		}
		still_alive;								/* For renewing leases */
		Event event;								/* Client event */
		char infostring[PROT_STRLEN];	/* class string from client bc's */
	}
	msg;
}
Packet;

#define PROT_CLASSNAMELEN                        128
#define PROT_MAXSTRLEN                           255
#define PROT_MAXHOSTNAME                         32
#define PROT_MAXZONES                            16
#define PROT_ZONE_LENGTH 16

/* We need this header for struct in_addr */
#include <netinet/in.h>

/* The entry in the linked list database of active clients */
struct hostlist
{
	struct in_addr host;					/* The ip address of the active host */
	unsigned int port;						/* The port to address the host with */
	struct timeval expired;				/* The time remaining at which this has expired */
	struct hostlist *nextent;			/* The next entry in the list */
};

#define SERVER_SUCCESS 1
#define SERVER_ALLOC_FAILED -1

/* Initializes the server data structure and returns true if successful,
 * false otherwise */
int ServerInit (unsigned int port);

/* Adds a broadcast zone with address 'zone' and port 'port' into the
 * broadcast list
 */
int ServerAddBroadcastZone (char *zone, int port);

/* Adds a class identifier into the server's class identifier
 * string */
int ServerAddIdentifier (char *class);

/* Sets and gets the server port */
void ServerSetPort (int p);
int ServerGetPort (void);

/* Starts the actual server process */
void ServerStart (void);

/* Breaks up a client broadcast string and extracts the info
 * into the appropriate datastructures */
void ServerProcessClientBC (char *infostring, struct sockaddr_in *from);

/* Updates the lease time for a client within the server host list */
void ServerUpdateClient (struct sockaddr_in *from);

/* Adds a client and creates a new lease time within the server's
 * autodiscovery and leasing host list */
int ServerAddClient (struct in_addr newhost, unsigned int hostport);

/* Function to remove expired hosts from the list. Should only occur
 * when a warning alarm has gone off */
void ServerPurgeHostList (void);

/* Lets all the clients know we're still alive so our lease doesn't expire */
void ServerSendStillAlive (void);

/* Handles an alarm signal, which allows an extra thread to run
 * concurrently and clean up the server host list at given
 * intervals */
void ServerHandleAlarm ();

/* Clean up the server host list data structure and shutdown the
 * networking sockets */
void ServerShutdown (void);

#endif __PEEP_SERVER_H__
