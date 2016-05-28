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
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <values.h>

/* To enable the wrapper library */
#ifdef HAVE_LIBWRAP
#include <tcpd.h>
#include <syslog.h>

int allow_severity = LOG_INFO;
int deny_severity = LOG_WARNING;
#endif

#include "Engine.h"
#include "Server.h"

#include <errno.h>
#include "Debug.h"

static int Server_Fd; /* Server socket descriptor */
static struct sockaddr_in Saddr; /* Socket Addresses for server */
static struct sockaddr *Saddr_Ptr; /* Ptr to server address */
static int Slen; /* Length of address structures */
static struct hostent *Host; /* Host information */
static struct in_addr *Host_Node;
static struct protoent *Prot; /* Entry from getprotobyname */
static int Prot_No; /* Protocol number */
static fd_set Read_Set; /* The handles to read with select */
static Packet Net_Packet; /* Event message the server read */
static char Localhost[PROT_MAXHOSTNAME]; /* The localhost string */
static unsigned int Port;

/* Declare the list of hosts */
static struct hostlist *Hlhead = NULL;

/* Data structure for having a class identifier string and holding
 * the broadcast zones */
static char *Identifier = NULL;
static unsigned int No_Zones = 0;
static char Broadcast_Zones[PROT_MAXZONES][PROT_ZONE_LENGTH];
static unsigned int Broadcast_Ports[PROT_MAXZONES];

int ServerInit(unsigned int p) {
    int i;

    /* Let's keep our own statically global copy */
    Port = p;

    /* Get the hostname */
    if (gethostname(Localhost, PROT_MAXHOSTNAME) < 0) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Uh Oh! Couldn't get my hostname: %s\n", s);
        return 0;
    }

    if ((Host = gethostbyname(Localhost)) == NULL) {
        char *s = strerror(errno);
        Log(DBG_GEN, "Uh Oh! Peep couldn't get local host information: %s\n", s);
        return 0;
    }

    Host_Node = (struct in_addr *) Host->h_addr;

    if ((Prot = getprotobyname("udp")) == NULL) {
        char *s = strerror(errno);
        Log(DBG_GEN,
                "Uh Oh! Error resolving protocl information for 'udp': %s\n", s);
        return 0;
    }
    Prot_No = Prot->p_proto;

    Server_Fd = socket(AF_INET, SOCK_DGRAM, Prot_No);

    /* Let's set the SO_BROADCAST so we can send n' receive broadcasts */
    {
        int x = 1;

        if (setsockopt(Server_Fd, SOL_SOCKET, SO_BROADCAST, &x,
                sizeof (x)) == -1) {
            char *s = strerror(errno);
            Log(DBG_GEN, "Uh Oh! Error setting socket options: %s\n", s);
            return 0;
        }
    }
    {
        int x = 1;

        if (setsockopt(Server_Fd, SOL_SOCKET, SO_REUSEADDR, &x,
                sizeof (x)) == -1) {
            char *s = strerror(errno);
            Log(DBG_GEN, "Uh Oh! Error setting socket options: %s\n", s);
            return 0;
        }
    }

    Slen = sizeof (Saddr);
    bzero((char *) &Saddr, Slen);
    Saddr.sin_family = AF_INET;
    Saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    Saddr.sin_port = htons(Port);

    Saddr_Ptr = (struct sockaddr *) &Saddr;
    bind(Server_Fd, Saddr_Ptr, Slen);

    FD_ZERO(&Read_Set);
    FD_SET(Server_Fd, &Read_Set);

    /* Let's check to see if Identifier is NULL to avoid segfaulting */
    if (Identifier == NULL) {
        Identifier = "";
        Log(DBG_DEF,
                "\nWARNING: Could not assemble a class identifier string. \n");
        Log(DBG_DEF,
                "Check your classes in peep.conf. Using blank identifier instead!\n\n");
    }


    Log(DBG_DEF, "Using class identifier string: %s\n", Identifier);

    /* Now that we're all setup, let's create our broadcast string
     * and let all the clients know we're here */
    for (i = 0; i < No_Zones; i++) {
        struct sockaddr_in bc;
        Packet outgoing;

        bzero((char *) &bc, sizeof (struct sockaddr_in));

        bc.sin_family = AF_INET;
        bc.sin_addr.s_addr = inet_addr(Broadcast_Zones[i]);
        bc.sin_port = htons(Broadcast_Ports[i]);

        /* Build the broadcast packet */
        outgoing.majorver = PROT_MAJORVER;
        outgoing.minorver = PROT_MINORVER;
        outgoing.type = PROT_BCSERVER;

        strncpy(outgoing.msg.bc_server.infostring, Identifier, PROT_STRLEN);

        outgoing.msg.bc_server.lease.min = PROT_SERVERLEASEMIN;
        outgoing.msg.bc_server.lease.sec = PROT_SERVERLEASESEC;

        if (sendto(Server_Fd, &outgoing, sizeof (outgoing), 0,
                (struct sockaddr *) &bc, sizeof (struct sockaddr_in)) < 0) {
            char *s = strerror(errno);
            Log(DBG_GEN, "Received a broadcast error message: %s\n", s);
            /* Don't return.. let's try to continue */
        }

        Log(DBG_GEN, "Sent alive broadcast to %s:%d...\n", Broadcast_Zones[i],
                Broadcast_Ports[i]);
    }

    /* Set the alarm signal handler and schedule an alarm */
    signal(SIGALRM, ServerHandleAlarm);
    alarm(PROT_SERVERWAKEUPMIN * 60 + PROT_SERVERWAKEUPSEC);

    /* We've made it this far, so let's succeed */
    return 1;
}

int ServerAddBroadcastZone(char *zone, int port) {

    if (No_Zones < PROT_MAXZONES) {

        strcpy(Broadcast_Zones[No_Zones], zone);
        Broadcast_Ports[No_Zones] = port;
        No_Zones++;

        return SERVER_SUCCESS;

    } else
        return SERVER_ALLOC_FAILED;

}

int ServerAddIdentifier(char *class) {
    if (!Identifier) {
        if ((Identifier = (char *) calloc(sizeof (char), PROT_CLASSNAMELEN)) ==
                NULL) return SERVER_ALLOC_FAILED;
    }

    /* Copy in the class and delimiter */
    strcpy(Identifier, class);
    strcat(Identifier, PROT_CLASSDELIM);

    return SERVER_SUCCESS;
}

void ServerSetPort(int p) {
    Port = p;
}

int ServerGetPort(void) {
    return Port;
}

void ServerStart(void) {
    unsigned int size_recv = 0;
    struct sockaddr_in from;
    unsigned long int fromlen;

    Log(DBG_DEF, "%s | Using INET Address: %s:%d\n", Localhost,
            inet_ntoa(*Host_Node), Port);

    /* Loop to for connections */
    while (1) {
        select(Server_Fd + 1, &Read_Set, NULL, NULL, NULL);

        /* Set the length of the struct for our recvfrom call */
        fromlen = sizeof (struct sockaddr_in);

#ifdef HAVE_LIBWRAP
        {
            struct request_info req;

            request_init(&req, RQ_DAEMON, argv[0], RQ_FILE, Server_Fd, NULL);
            fromhost(&req);

            if (!hosts_access(&req)) {

                /* Receive and discard the message */
                size_recv = recvfrom(Server_Fd, &Net_Packet, sizeof (Packet), 0,
                        (struct sockaddr *) &from,
                        (socklen_t *) & fromlen);

                Log(DBG_SRVR, "Refused connection from %s.\n", req.client[0].name);
                continue;
            }
        }
#else
        /* Receive the event. */
        size_recv = recvfrom(Server_Fd, &Net_Packet, sizeof (Packet), 0,
                (struct sockaddr *) &from, (socklen_t *) & fromlen);
#endif

        if (size_recv == -1 && errno == ECONNREFUSED) {

#if DEBUG_LEVEL & DBG_SRVR
            Log
                    (DBG_SRVR,
                    "Server got connection refused. UDP packet did not reach its destination...\n\n");
#endif
            continue;
        }

#if DEBUG_LEVEL & DBG_SRVR
        Log(DBG_SRVR, "\nServer received a packet from %s with size: %d\n",
                inet_ntoa(from.sin_addr), size_recv);
#endif

        /* Process the packet based on type. Since we currently only have one 
         * version, we can ignore id for now.. */
#if DEBUG_LEVEL & DBG_SRVR
        Log
                (DBG_SRVR,
                "Packet header:\nMajor ver: %d\nMinor ver: %d\nType: %d\nReserved: %d\n",
                Net_Packet.majorver, Net_Packet.minorver, Net_Packet.type,
                Net_Packet.reserved);
#endif

        switch (Net_Packet.type) {

            case PROT_BCCLIENT:

#if DEBUG_LEVEL & DBG_SRVR
                Log(DBG_SRVR, "Got client broadcast packet with identifier: %s\n",
                        Net_Packet.msg.infostring);
#endif

                ServerProcessClientBC((char *) &Net_Packet.msg.infostring, &from);
                break;

            case PROT_CLIENTSTILLALIVE:

#if DEBUG_LEVEL & DBG_SRVR
                Log(DBG_SRVR, "Got client still alive packet\n");
#endif
                ServerUpdateClient(&from);
                break;

            case PROT_CLIENTEVENT:

#if DEBUG_LEVEL & DBG_SRVR
                Log(DBG_SRVR, "Got client event\n");
#endif

                /* Enqueue the event so the engine can get it later */
                EngineEnqueue(Net_Packet.msg.event);
                break;

            default:

#if DEBUG_LEVEL & DBG_SRVR
                Log(DBG_SRVR, "Received unsupported packet type. Discarding...\n");
#endif

        }
    }
}

/* Break up the info string and extract info */
void ServerProcessClientBC(char *infostring, struct sockaddr_in *from) {
    char delim;
    char *p, *q;

    sprintf(&delim, "%s", PROT_CLASSDELIM);

    /* Note: Perl pads strings with blank spaces. So let's check if we've
     * actually hit the end or just a blank space. We can also use strstr()
     * to find the class within our identifier since each classname will
     * be bounded by a "!"
     */
    for (p = infostring, q = infostring; *q != '\0' && !isspace(*q); q++) {
        if (*q == delim) {
            char tmp[PROT_STRLEN];

            *q = '\0';
            strcpy(tmp, p);
            strcat(tmp, PROT_CLASSDELIM);

            /* Check if the string is part of our identifier. 
             * Note that the delimiter is at the end of the string to prove 
             * that it's the entire string */
            if (strstr(Identifier, tmp) != NULL) {
                /* Then we are part of this class and should add the client */
                ServerAddClient(from->sin_addr, from->sin_port);
                break;
            }

            p = q + 1;
        }
    }
}

void ServerUpdateClient(struct sockaddr_in *from) {
    struct hostlist *p = Hlhead;
    struct timeval expired;

    while (p) {
        if (from->sin_addr.s_addr == p->host.s_addr) {
            gettimeofday(&expired, NULL);
            expired.tv_sec += (PROT_SERVERWAKEUPMIN * 60 + PROT_SERVERWAKEUPSEC) *
                    PROT_WAKEUPSBEFOREEXPIRED;
            expired.tv_usec = 0; /* Zero out our expire time */
            p->expired = expired;
        }

        p = p->nextent;
    }
}

int ServerAddClient(struct in_addr newhost, unsigned int hostport) {
    struct hostlist *p;
    struct timeval expired;
    unsigned int flag = 0;
    /* Variables for returning a response to the client directly */
    struct sockaddr_in *bc;
    Packet outgoing;

#if DEBUG_LEVEL & DBG_SRVR
    Log(DBG_SRVR, "Adding client: %s:%d\n", inet_ntoa(newhost),
            ntohs(hostport));
#endif

    /* First search to see if we have an entry before we make a new one */
    for (p = Hlhead; p; p = p->nextent) {
        if (p->host.s_addr == newhost.s_addr && p->port == hostport) {
            flag = 1;

#if DEBUG_LEVEL & DBG_SRVR
            Log(DBG_SRVR, "Client already in hostlist. Updating expiry time...\n");
#endif

            break;
        }
    }

    /* If we don't have a set flag, let's make a new entry */
    if (!flag) {
        if (Hlhead == NULL) {
            Hlhead = (struct hostlist *) calloc(sizeof (struct hostlist), 1);
            p = Hlhead;
        } else {
            p = Hlhead;

            /* Move to the end of the list */
            while (p->nextent)
                p = p->nextent;

            p->nextent = (struct hostlist *) calloc(sizeof (struct hostlist), 1);

            if ((p = p->nextent) == NULL)
                return 0;
        }


        p->host = newhost;
        p->port = hostport;
    }

    /* Calculate the entry's expiry time */
    gettimeofday(&expired, NULL);
    expired.tv_sec += (PROT_SERVERWAKEUPMIN * 60 + PROT_SERVERWAKEUPSEC) *
            PROT_WAKEUPSBEFOREEXPIRED;
    expired.tv_usec = 0; /* Zero out our expire time */
    p->expired = expired;


    /* Now send the client directly a BC packet so that it will
     * Add us to its hostlist. Note that we send the packet even
     * though the host may already be in the list. This is to
     * tell clients that may have done a quick restart to readd
     * the server... */
    bc = (struct sockaddr_in *) calloc(sizeof (struct sockaddr_in), 1);
    bc->sin_family = AF_INET;
    bc->sin_addr.s_addr = p->host.s_addr;
    bc->sin_port = p->port;

    /* Build the broadcast packet */
    outgoing.majorver = PROT_MAJORVER;
    outgoing.minorver = PROT_MINORVER;
    outgoing.type = PROT_BCSERVER;

    strcpy(outgoing.msg.bc_server.infostring, Identifier);

    outgoing.msg.bc_server.lease.min = PROT_SERVERLEASEMIN;
    outgoing.msg.bc_server.lease.sec = PROT_SERVERLEASESEC;

    if (sendto
            (Server_Fd, &outgoing, sizeof (outgoing), 0, (struct sockaddr *) bc,
            sizeof (struct sockaddr_in)) < 0) {
        char *s = strerror(errno);
        Log(DBG_SRVR, "Received an error responding to a client broadcast: %s\n",
                s);
        /* Don't return.. let's try to continue */
    }

#if DEBUG_LEVEL & DBG_SRVR
    Log(DBG_SRVR, "Sent a response to the client\n");
#endif

    cfree(bc);

#if DEBUG_LEVEL & DBG_SRVR
    {
        int i = 0;
        struct hostlist *r;
        for (r = Hlhead; r; r = r->nextent)
            i++;
        Log(DBG_SRVR, "We now have %d entries in our hostlist...\n", i);
    }
#endif

    return 1;
}

void ServerPurgeHostList(void) {
    struct hostlist *p, *q;
    struct timeval cur_time;

#if DEBUG_LEVEL & DBG_SRVR
    int cnt = 0;

    {
        int i;
        for (i = 0, p = Hlhead; p; p = p->nextent, i++);
        Log(DBG_SRVR, "Have %d host entries prior to purge...\n", i);
    }
#endif

    /* Get the current time for comparison */
    gettimeofday(&cur_time, NULL);

    for (p = Hlhead, q = NULL; p; q = p, p = p->nextent) {

        /* At the time of writing this, i might be tired but this has to be neg. */
        if (p->expired.tv_sec <= cur_time.tv_sec) {
            if (p == Hlhead) {
                /* Thanks to Mike Stute mstute@globaldataguard.com for pointing out that
                 * we should reassign Hlhead */
                Hlhead = p->nextent;
                cfree(Hlhead);
                Hlhead = NULL;

#if DEBUG_LEVEL & DBG_SRVR
                cnt++;
#endif

            } else {
                q->nextent = p->nextent;
                cfree(p);

#if DEBUG_LEVEL & DBG_SRVR
                cnt++;
#endif

            }
        }
    }

#if DEBUG_LEVEL & DBG_SRVR
    Log(DBG_SRVR, "Purged %d hosts...\n", cnt);
#endif

}

void ServerSendStillAlive(void) {
    struct hostlist *p;
    Packet Net_Packet;

    Net_Packet.majorver = PROT_MAJORVER;
    Net_Packet.minorver = PROT_MINORVER;
    Net_Packet.type = PROT_SERVERSTILLALIVE;
    Net_Packet.reserved = 0;
    Net_Packet.msg.still_alive.lease.min = PROT_SERVERLEASEMIN;
    Net_Packet.msg.still_alive.lease.sec = PROT_SERVERLEASESEC;

    for (p = Hlhead; p; p = p->nextent) {
        struct sockaddr_in bc;

        memset(&bc, 0, sizeof (bc));
        bc.sin_family = AF_INET;
        bc.sin_addr.s_addr = p->host.s_addr;
        bc.sin_port = p->port;

#if DEBUG_LEVEL & DBG_SRVR
        Log(DBG_SRVR, "Sending Alive to: %s:%d\n", inet_ntoa(p->host),
                ntohs(p->port));
#endif

        sendto(Server_Fd, &Net_Packet, sizeof (Net_Packet), 0,
                (struct sockaddr *) &bc, sizeof (bc));
    }
}

void ServerHandleAlarm() {

#if DEBUG_LEVEL & DBG_SRVR
    Log(DBG_SRVR,
            "Received alarm. Letting valid clients know we're still alive...\n");
#endif

    ServerPurgeHostList();
    ServerSendStillAlive();

    /* reschedule the alarm */
    alarm(PROT_SERVERWAKEUPMIN * 60 + PROT_SERVERWAKEUPSEC);
}

void ServerShutdown(void) {
    struct hostlist *p;

    close(Server_Fd);

    /* Check if we have a list, then get rid of it */
    if (Hlhead) {
        p = Hlhead->nextent;

        while (Hlhead) {
            cfree(Hlhead);
            Hlhead = p;
            if (p && p->nextent)
                p = p->nextent;
        }

    }
}
