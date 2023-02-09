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

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "signames.h"

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

int globargc;
static char *progname;

extern int matchsig (char *);
extern char *unmatchsig (int);

void usage (void)
{
 fprintf (stderr, "usage: %s [-SIGNAL] process ...\n"
                  "       %s [-s SIGNAME] process ...\n"
                  "       %s [-n SIGNUM] process ...\n"
                  "       %s -l [SIGNUM ...]\n", 
          progname, progname, progname, progname);
 exit(1);
}

void nope (void)
{
 fprintf (stderr, "%s: not a valid signal\n", progname);
 exit(2);
}

void doit (int sig, char **argv, int start)
{
 int t;
 
 for (t=start; t<globargc; t++)
 {
  int n;
  
  n=atoi(argv[t]);

  switch (kill(n,sig))
  {
   case EINVAL:
    nope();
   case EPERM:
    fprintf (stderr, "%s: access denied to pid %u\n", progname, n);
    exit(3);
   case ESRCH:
    fprintf (stderr, "%s: no such process %u\n", progname, n);
    exit(4);
  }
 }
 exit(0);
}

int main (int argc, char **argv)
{
 int n;
 
 globargc=argc;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 
 if (argc==1)
 {
  usage();
 }
 
 if (!strcmp(argv[1],"-l"))
 {
  if (argc==2)
  {
   const char *x;
   
   for (n=1; 0!=(x=unmatchsig(n)); n++)
   {
    printf ("%u: %s\n",n,x);
   }
   
   return 0;
  }
  else
  {
   int t;
   
   for (t=2; t<argc; t++)
   {
    if (isdigit(argv[t][0]))
    {
     const char *x;
     n=atoi(argv[t]);
     x=unmatchsig(n);
     if (!x) nope();
     printf ("%s\n", x);
    }
    else
    {
     int x;
     x=matchsig(argv[t]);
     if (!x) nope();
     printf ("%u\n",x);
    }
   }
   
   return 0;
  }
 }
 
 if (argv[1][0]=='-')
 {
  if (argv[1][1]=='s')
  {
   if (argv[1][2])
   {
    if (argc<3) usage();
    n=matchsig(&(argv[1][2]));
    if (!n) nope();
    doit(n,argv,2);
   }
   else
   {
    if (argc<4) usage();
    n=matchsig(argv[2]);
    if (!n) nope();
    doit(n,argv,3);
   }
  }
  if (argv[1][1]=='n')
  {
   if (argv[1][2])
   {
    if (argc<3) usage();
    n=atoi(&(argv[1][2]));
    doit(n,argv,2);
   }
   else
   {
    if (argc<4) usage();
    n=atoi(argv[2]);
    doit(n,argv,3);
   }
  }
  else
  {
   int n;
   
   if (argc==2) usage();
   if (isdigit(argv[1][1]))
   {
    n=atoi(&(argv[1][1]));
    doit(n,argv,2);
   }
   else
   {
    n=matchsig(&(argv[1][1]));
    if (!n) nope();
    doit(n,argv,2);
   }
  }
 }
 
 doit(SIGTERM,argv,1);
}
