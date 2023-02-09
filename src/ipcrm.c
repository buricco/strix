/*
 * (C) Copyright 2023 S. V. Nickolas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.
 *
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

void err_badid (char *foo)
{
 fprintf (stderr, "%s: bad id: '%s'\n", progname, foo);
 exit(1);
}

void err_badkey (char *foo)
{
 fprintf (stderr, "%s: bad key: '%s'\n", progname, foo);
 exit(1);
}

void usage (void)
{
 fprintf (stderr,
          "%s: usage: %s [-m id] [-s id] [-q id] [-M key] [-S key] [-Q key]\n",
          progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int did_nothing;
 int e, n, r, v, cumul;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 did_nothing=1;
 cumul=0;

 while (-1!=(e=getopt(argc, argv, "m:s:q:M:S:Q:")))
 {
  switch (e)
  {
   case 'm':
    did_nothing=0;
    errno=0;
    v=strtol(optarg, NULL, 0);
    if (errno) err_badid(optarg);
    r=shmctl(v, IPC_RMID, NULL);
    if (r)
    {
     fprintf (stderr, "%s: failed to delete id '%s': %s\n",
              progname, optarg, strerror(errno));
    }
    break;
   case 's':
    did_nothing=0;
    errno=0;
    v=strtol(optarg, NULL, 0);
    if (errno) err_badid(optarg);
    r=semctl(v, 0, IPC_RMID);
    if (r)
    {
     fprintf (stderr, "%s: failed to delete id '%s': %s\n",
              progname, optarg, strerror(errno));
    }
    break;
   case 'q':
    did_nothing=0;
    errno=0;
    v=strtol(optarg, NULL, 0);
    if (errno) err_badid(optarg);
    r=msgctl(v, IPC_RMID, NULL);
    if (r)
    {
     fprintf (stderr, "%s: failed to delete id '%s': %s\n",
              progname, optarg, strerror(errno));
    }
    break;
   case 'M':
    did_nothing=0;
    errno=0;
    v=strtol(optarg, NULL, 0);
    if (errno) err_badkey(optarg);
    n=shmget(v, 0, 0);
    if (n==-1) err_badkey(optarg);
    r=shmctl(v, IPC_RMID, NULL);
    if (r)
    {
     fprintf (stderr, "%s: failed to delete key '%s': %s\n",
              progname, optarg, strerror(errno));
    }
    break;
   case 'S':
    did_nothing=0;
    errno=0;
    v=strtol(optarg, NULL, 0);
    if (errno) err_badkey(optarg);
    n=semget(v, 0, 0);
    if (n==-1) err_badkey(optarg);
    r=semctl(n, 0, IPC_RMID);
    if (r)
    {
     fprintf (stderr, "%s: failed to delete key '%s': %s\n",
              progname, optarg, strerror(errno));
    }
    break;
   case 'Q':
    did_nothing=0;
    errno=0;
    v=strtol(optarg, NULL, 0);
    if (errno) err_badkey(optarg);
    n=msgget(v, 0);
    if (n==-1) err_badkey(optarg);
    r=msgctl(n, IPC_RMID, NULL);
    if (r)
    {
     fprintf (stderr, "%s: failed to delete key '%s': %s\n",
              progname, optarg, strerror(errno));
    }
    break;
   default:
    usage(); /* NO RETURN */
  }
  if (r) cumul=1;
 }

 if (did_nothing)
  usage();

 return cumul;
}
