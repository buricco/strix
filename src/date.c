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
 * Posix:
 *
 *   date [-u] [+format]
 *   date [-u] mmddhhmm[[cc]yy]
 *
 * -u = Zulu time (won't work when setting time on SVR4)
 * +format - option to strftime
 * cc is assumed to be 19 if yy<69
 * default is "%a %b %e %H:%M:%S %Z %Y".
 *
 * SVR4 adds -a to inch the time.
 *
 *   DO NOT REPLACE A BSD date(1) WITH THIS VERSION.
 */

#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

static char *def_format = "%a %b %e %H:%M:%S %Z %Y";

int w00t;

char buf[BUFSIZ];

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

/*
 * Inching the clock - sys/time.h
 *   int adjtime(const struct timeval *delta, const struct timeval *olddelta);
 *
 * A NULL to olddelta is valid.  delta contains the number of microseconds and
 * seconds by which to shift the time.
 *
 * Setting the clock at once - sys/time.h
 *   int settimeofday(const struct timeval *when, const struct timezone *tz);
 *
 * The timevalue to set to settimeofday is relative to the EPOCH.
 * Pass NULL as tz.
 *
 * Making a time delta means we have to gettimeofday() into a timeval, then
 * compute the difference.  We're not bothered about microseconds, so just use
 * time(), throw microseconds into the woodchipper and operate on the seconds.
 */

int settime (time_t when, int gradual)
{
 struct timeval t;

 t.tv_usec=0;

 if (gradual)
 {
  t.tv_sec=when-time(NULL);
  return adjtime(&t, NULL);
 }
 else
 {
  t.tv_sec=when;
  return settimeofday(&t, NULL);
 }
}

int pair (char *x)
{
 char a, b;

 if (!isdigit(x[0])) return -1;
 if (!isdigit(x[1])) return -1;
 a=x[0]-'0';
 b=x[1]-'0';
 return (a*10)+b;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-u] [+format]\n", progname, progname);
 if (w00t)
 {
  fprintf (stderr, "%s: usage: %s [-au] MMDDhhmm[[yy]yy]\n",
           progname, progname);
 }
 exit(1);
}

void bogus (char *str)
{
 fprintf (stderr, "%s: bogus date string '%s'\n", progname, str);
 exit(1);
}

int main (int argc, char **argv)
{
 int e;
 int edge, zulu;
 char *p;
 time_t t;
 struct tm *tm, ntm;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 edge=zulu=0;
 w00t=!geteuid();
 while (-1!=(e=getopt(argc, argv, "au")))
 {
  switch (e)
  {
   case 'a':
    if (!w00t)
     usage();
    edge=1;
    break;
   case 'u':
    zulu=1;
    break;
   default:
    usage();
  }
 }

 if ((argc-optind)>1) usage();

 /* No args - just show time and be done with it */
 if ((argc-optind)==0)
 {
  t=time(NULL);

  tm=(zulu?gmtime:localtime)(&t);
  if (!tm)
  {
   xperror("could not process time");
   return 1;
  }

  strftime(buf, BUFSIZ, def_format, tm);
  printf ("%s\n", buf);
  return 0;
 }

 /* Pretend we don't know what the user's talking about */
 if (((*argv[optind])!='+')&&(!w00t)) usage();

 if ((*argv[optind])=='+')
 {
  t=time(NULL);

  tm=(zulu?gmtime:localtime)(&t);
  if (!tm)
  {
   xperror("could not process time");
   return 1;
  }

  strftime(buf, BUFSIZ, argv[optind]+1, tm);
  printf ("%s\n", buf);
  return 0;
 }

 memset(&ntm, 0, sizeof(struct tm));
 p=argv[optind];

 /* Validate length; get the year if any */
 switch (strlen(p))
 {
  case 12: /* 4-digit year */
   e=pair(p+8);
   if (e<0) bogus(p);
   ntm.tm_year=(e*100)-1900;
   e=pair(p+10);
   if (e<0) bogus(p);
   ntm.tm_year+=e;
   break;
  case 10: /* 2-digit year */
   e=pair(p+8);
   if (e<0) bogus(p);
   if (e<69) e+=100; /* Also sprach Posix */
   ntm.tm_year=e;
   break;
  case 8:
   t=time(NULL);
   tm=(zulu?gmtime:localtime)(&t);
   ntm.tm_year=tm->tm_year;
   break;
  default:
   bogus(p);
 }

 e=pair(p);
 if (e<0) bogus(p);
 ntm.tm_mon=e;
 e=pair(p+2);
 if (e<0) bogus(p);
 ntm.tm_mday=e;
 e=pair(p+4);
 if (e<0) bogus(p);
 ntm.tm_hour=e;
 e=pair(p+6);
 if (e<0) bogus(p);
 ntm.tm_min=e;

 ntm.tm_isdst=-1;

#ifdef __SVR4__
 t=mktime(&ntm);
#else
 t=(zulu?timegm:mktime)(&ntm);
#endif
 if (t==-1) bogus(p);

 e=settime(t, edge);

 return 0;
}
