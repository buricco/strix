/*
 * (C) Copyright 2020, 2023 S. V. Nickolas.
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

/*
 * This does do some Linux-specific stuff, but I don't see how it would be
 * particularly hard to get it up on something else.
 */

#include <sys/stat.h>

#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "signames.h"

#ifndef ARG_MAX
#define ARG_MAX 8192
#endif

/*
 * Implemented:
 *
 * -d set delimiter (default "\n")
 * -f match the whole process name
 * -i case insensitive match
 * -l also show the argv[0] of the process (ignore if kill)
 * -v not
 * -x wrap the regex in ^$
 *
 * Unimplemented, might implement later:
 *
 * -g limit to these process groups
 * -G limit to these groups
 * -n only find the most recently initiated process
 * -o only find the least recently initiated process
 * -P limit to immediate children of this process
 * -s limit to processes running in this session (0: mine)
 * -t limit to processes connected to this tty
 * -u limit to processes with this effective user id
 * -U limit to processes with this actual user id
 */

static char *copyright="@(#) (C) Copyright 2020, 2023 S. V. Nickolas\n";

static char *progname;

extern int matchsig (char *foo);

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

FILE *fopen_or_else (char *filename, char *mode)
{
 char *p;
 FILE *file;

 file=fopen(filename,mode);
 if (file) return file;
 xperror(filename);
 exit(3);
}

void *malloc_or_else (size_t size)
{
 void *foo;

 foo=malloc(size);
 if (foo) return foo;
 perror(progname);
 exit(3);
}

void usage (int ispkill)
{
 if (ispkill)
  fprintf (stderr, "%s: usage: %s [-S signal] [-dfivx] expression ...\n",
           progname, progname);
 else
  fprintf (stderr, "%s: usage: %s [-dfilvx] expression ...\n", progname, progname);
 exit(2);
}

unsigned long int_from_file (char *filename)
{
 FILE *file;
 char *p;
 long l;
 size_t s;

 file=fopen_or_else(filename,"r");
 s=0;
 while (1)
 {
  if (fgetc(file)<0) break;
  s++;
 }
 rewind(file);
 /* Result of next 3 lines: file input will be null-terminated */
 p=malloc_or_else(s+1);
 memset(p,0,s);
 l=fread(p,1,s,file);
 fclose(file);
 if (l<s)
  fprintf (stderr, "%s: warning: short read from %s\n", progname, filename);

 l=strtol(p,0,0);
 free(p);

 return l;
}

int main (int argc, char **argv)
{
 FILE *file;
 pid_t maxpid, travel, sid;
 size_t s;
 char *p;
 char *delim;
 int e;
 int one;
 int fflag,iflag,lflag,nflag,oflag,Pflag,sflag,vflag,xflag;
 int ispkill;
 int reflags;
 int mode;

 ispkill=0;
 mode=SIGTERM;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (strstr(progname,"kill")) ispkill=1;

 fflag=iflag=lflag=nflag=oflag=Pflag=sflag=vflag=xflag=0;

 maxpid=int_from_file("/proc/sys/kernel/pid_max");
 if (maxpid<=0)
 {
  fprintf (stderr, "%s: received PID_MAX is bogus\n", progname);
  return 3;
 }

 delim="\n";

 e=0;
 while (-1!=(e=getopt(argc, argv, ispkill?"d:fnoPS:s:vx":"d:flnoPs:vx")))
 {
  switch (e)
  {
   case 'd':
    delim=optarg;
    break;
   case 'f':
    fflag=1;
    break;
   case 'i':
    iflag=1;
    break;
   case 'l':
    lflag=1;
    break;
   case 'n':
    nflag=1;
    break;
   case 'o':
    oflag=1;
    break;
   case 'P':
    Pflag=1;
    break;
   case 'S':
    mode=matchsig(optarg);
    if (!mode)
    {
     fprintf (stderr, "%s: signal '%s' is unknown\n", progname, optarg);
     return 3;
    }
    break;
   case 's':
    sflag=1;
    sid=atoi(optarg);
    if (!sid) sid=getsid(0);
    if (sid==-1)
    {
     perror(progname);
     return 3;
    }
    break;
   case 'v':
    vflag=1;
    break;
   case 'x':
    xflag=1;
    break;
   default:
    usage(ispkill);
  }
 }

 if (optind==argc) usage(ispkill);

 if (fflag&&xflag)
  fprintf (stderr, "%s: -f flag ignored because -x set\n", progname);

 if (nflag&&oflag)
 {
  fprintf (stderr, "%s: -n and -o flags are mutually exclusive\n", progname);
  return 2;
 }

 reflags=REG_NOSUB;
 if (iflag) reflags|=REG_ICASE;

 for (e=optind; e<argc; e++)
 {
  int E;
  regex_t regex;

  if (xflag)
  {
   char *b;

   b=malloc_or_else(strlen(argv[e]+3));
   sprintf (b,"^%s$",argv[e]);
   E=regcomp(&regex, b, reflags);
   free(b);
  }
  else
  {
   E=regcomp(&regex, argv[e], reflags);
  }
  if (E)
  {
   fprintf (stderr, "%s: could not parse regular expression '%s'\n",
            progname, argv[e]);
  }

  one=0;
  for (travel=0; travel<(maxpid+1); travel++)
  {
   struct stat S;
   char foobuf[ARG_MAX];
   char *barbuf, *grillbuf;
   int base;
   int r;
   int msl;

   sprintf(foobuf, "/proc/%u", travel);
   base=strlen(foobuf);
   E=stat(foobuf,&S);
   if (E) continue;
   strcat(foobuf,"/cmdline");
   msl=0;
   file=fopen(foobuf,"rb");
   if (!file)
    continue;
   while (1)
   {
    r=fgetc(file);
    if (r<=0) break;
    msl++;
   }
   if (r<0)
   {
    fclose(file);
    continue;
   }
   rewind(file);
   barbuf=malloc_or_else(msl+1);
   fread(barbuf,1,msl+1,file);
   fclose(file);

   if (fflag)
    grillbuf=barbuf;
   else
   {
    grillbuf=strrchr(barbuf,'/');
    if (grillbuf)
     grillbuf++;
    else
     grillbuf=barbuf;
   }

   E=regexec(&regex,grillbuf,0,0,0);

   if (ispkill)
   {
    if (vflag?E:!E)
    {
     kill(travel,mode);
    }
   }
   else
   {
    if (vflag?E:!E)
    {
     if (!one)
      one++;
     else
      printf ("%s", delim);

     if (lflag)
      printf ("%u %s",travel,grillbuf);
     else
      printf ("%u",travel);
    }
   }

   free(barbuf);
  }
  printf ("\n");

  regfree(&regex);
 }
}
