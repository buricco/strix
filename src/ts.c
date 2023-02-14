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

/*
 * -i ... timestamp is relative to the last line
 * -s ... timestamp is relative to the time the program was started
 * -m ... use CLOCK_MONOTONIC instead of CLOCK_REALTIME
 * 
 * This program requires features System V lacks but Linux and the BSDs have.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

static char *def_format_abs="%b %d %H:%M:%S";
static char *def_format_rel="%H:%M:%S";

#define MODE_I 0x01
#define MODE_S 0x02
int mode;

long getbogosity (void)
{
#ifdef __Linux__
 return timezone;
#else
 struct tm *local;
 time_t t;
 
 t=time(NULL);
 
 local=localtime(&t);
 return -(local->tm_gmtoff); /* do the narsty... */
#endif
}

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-ims] [format]\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 char buf[BUFSIZ], timebuf[BUFSIZ];
 clockid_t whichclock;
 time_t epoch;
 int e;
 char *format;
 struct timespec timespec;
 time_t lasttime, newtime;
 long bogosity;
  
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 
 tzset();
 bogosity=getbogosity();
 
 whichclock=CLOCK_REALTIME;
 epoch=0;
 mode=0;
 
 while (-1!=(e=getopt(argc, argv, "ims")))
 {
  switch (e)
  {
   case 'i':
    mode |= MODE_I;
    break;
   case 's':
    mode |= MODE_S;
    break;
   case 'm':
    whichclock=CLOCK_MONOTONIC;

    /* Find epoch, roughly enough to compensate */
    if (clock_gettime(CLOCK_MONOTONIC, &timespec))
    {
     xperror("clock_gettime");
     return 1;
    }
    epoch=time(NULL)-timespec.tv_sec;
    break;
   default:
    usage();
  }
 }
 
 if (mode==(MODE_I|MODE_S))
 {
  fprintf (stderr, "%s: -i and -s are mutually exclusive\n", progname);
  return 1;
 }
 
 if (argc-optind>1)
  usage();
 
 if (argc==optind)
  format=mode?def_format_rel:def_format_abs;
 else
  format=argv[optind];
  
 clock_gettime(whichclock, &timespec);
 lasttime=timespec.tv_sec;
 
 while (1)
 {
  if (mode==MODE_I) lasttime=timespec.tv_sec;
  fgets(buf, BUFSIZ-1, stdin);
  if (feof(stdin)) return 0;
  if (ferror(stdin))
  {
   xperror("(stdin)");
   return 1;
  }
  clock_gettime(whichclock, &timespec);
  if (mode)
  {
   newtime=(timespec.tv_sec-lasttime)-bogosity;
  }
  else
  {
   newtime=timespec.tv_sec+epoch;
  }
  
  strftime(timebuf, BUFSIZ-1, format, (mode?gmtime:localtime)(&newtime));
  printf ("%s %s", timebuf, buf); /* newline is in buf already */
 }
}
