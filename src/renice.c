/*
 * (C) Copyright 2020, 2022, 2023 S. V. Nickolas.
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

#include <sys/resource.h>
#include <sys/time.h>

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RN_ISPID  0x00
#define RN_ISPGID 0x01
#define RN_ISUID  0x02

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

static char *progname;

/*
 * If the name exists in the user database, fill in target and return 0.
 * If not, or if user or target is NULL, return -1.
 */
int find_uid (char *user, uid_t *target)
{
 struct passwd *p;

 if (!user) return -1;
 if (!target) return -1;

 p=getpwnam(user);
 if (!p) return -1;

 *target=p->pw_uid;
 return 0;
}

int do_renice (int n, uid_t v, int p)
{
 int c, r;

 r=setpriority(p, v, getpriority(p, v)+n);
 if (!r) return 0;
 fprintf (stderr, "%s: %d: %s\n", progname, c, strerror(errno));
 return 1;
}

int renice_pid (int n, int optind, int argc, char **argv)
{
 int e,t;

 e=0;
 for (t=optind; t<argc; t++)
  e+=do_renice(n, atoi(argv[t]), PRIO_PROCESS);

 return e;
}

int renice_pgid (int n, int optind, int argc, char **argv)
{
 int e,t;

 e=0;
 for (t=optind; t<argc; t++)
  e+=do_renice(n, atoi(argv[t]), PRIO_PGRP);

 return e;
}

int renice_uid (int n, int optind, int argc, char **argv)
{
 int e,t;
 uid_t v;

 e=0;
 for (t=optind; t<argc; t++)
 {
  if (!(find_uid(argv[t], &v)))
  {
   fprintf (stderr, "%s: no such user '%s'\n", progname, argv[t]);
   return -1;
  }

  e+=do_renice(n, v, PRIO_USER);
 }

 return e;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-g | -p | -u] -n increment id ...\n",
          progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e, n;
 int mode;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 e=0;
 n=0;
 mode=RN_ISPID;
 while (-1!=(e=getopt(argc, argv, "gpun:")))
 {
  switch (e)
  {
   case 'g':
    mode=RN_ISPGID;
    break;
   case 'p':
    mode=RN_ISPID;
    break;
   case 'u':
    mode=RN_ISUID;
    break;
   case 'n':
    n=atoi(optarg);
    break;
   default:
    usage();
  }
 }

 if (optind==argc) usage();
 switch (mode)
 {
  case (RN_ISPID):
   return renice_pid(n, optind, argc, argv);
  case (RN_ISPGID):
   return renice_pgid(n, optind, argc, argv);
  case (RN_ISUID):
   return renice_uid(n, optind, argc, argv);
 }

 return 0;
}
