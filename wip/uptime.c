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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>

#ifndef __linux__
#include <utmpx.h>
#endif

/* Valid on Linux and NetBSD */
#define _PATH_UTMP "/var/run/utmp"

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

#define MODE_P 0x01
#define MODE_S 0x02
int mode;

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

unsigned long long uptime (void)
{
 FILE *file;
#ifdef __linux__
 struct utmp utmp;
#else
 struct utmpx utmp;
#endif
 uint32_t t;
 
 file=fopen(_PATH_UTMP, "rt");
 if (!file)
 {
  xperror("opening " _PATH_UTMP);
  exit(1);
 }
 while (1)
 {
  if (fread (&utmp, sizeof(struct utmp), 1, file)<1)
  {
   if (feof(file)) break;
   fclose(file);
   xperror("reading " _PATH_UTMP);
   exit(1);
  }
  
  if (utmp.ut_type!=BOOT_TIME) continue;
  t=utmp.ut_tv.tv_sec;
 }
 fclose(file);
 return time(0)-t;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-ps]\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e;
 unsigned long long u;
 double avgs[3];
 uint32_t d;
 uint8_t h,m,s;
 struct tm *tm;
 time_t t;
 char tbuf[21];
 int commas;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 
 mode=0;
 
 while (-1!=(e=getopt(argc, argv, "ps")))
 {
  switch (e)
  {
   case 'p':
    mode|=MODE_P;
    break;
   case 's':
    mode|=MODE_S;
    break;
   default:
    usage();
  }
 }
 
 if (argc>optind) usage();
 if (mode==(MODE_P|MODE_S))
 {
  fprintf (stderr, "%s: -p and -s are mutually exclusive\n", progname);
  return 1;
 }
 
 u=uptime();
 e=getloadavg(avgs, 3);
 d=u/86400;
 h=(u/3600)%24;
 m=(u/60)%60;
 s=u%60;
 t=time(0);
 tm=localtime(&t);
 
 switch (mode)
 {
  case 0:
   strftime(tbuf, 20, "%H:%M:%S", tm);
   printf (" %s up ", tbuf);
   if (d)
    printf ("%u day%s ", d, d!=1?"s":"");
   printf ("%2d:%02d, load average: %.2f, %.2f, %.2f\n",
           h, m, s, avgs[0], avgs[1], avgs[2]);
   return 0;
  case MODE_P:
   commas=0;
   printf ("up ");
   if (d)
   {
    commas=1;
    printf ("%u day%s", d, d!=1?"s":"");
   }
   if (h)
   {
    if (commas) printf (", ");
    commas=1;
    printf ("%u hour%s", h, h!=1?"s":"");
   }
   if (m)
   {
    if (commas) printf (", ");
    commas=1;
    printf ("%u minute%s", m, m!=1?"s":"");
   }
   printf ("\n");
   return 0;
  case MODE_S:
   t-=u;
   tm=localtime(&t);
   strftime(tbuf, 20, "%Y-%m-%d %H:%M:%S", tm);
   printf ("%s\n", tbuf);
   return 0;
 }
}
