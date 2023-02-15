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
 * BSD had already added support for "Julian mode" in 4.3BSD-Reno, but that
 * just feels like too much work given that SVR4 didn't support it and Posix
 * doesn't require it.
 * 
 * Posix explicitly does not specify the output format.  The traditional
 * format for annual calendars is to stack them 3x4.  We take the lazy
 * approach and just run them one by one.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

/* Whoever designed this distribution of days was braindamaged. */
unsigned dpm[12]=
{
 31, 28, 31, 30,
 31, 30, 31, 31,
 30, 31, 30, 31
};

char *full_month[12]=
{
 "January", "February", "March", "April",
 "May", "June", "July", "August",
 "September", "October", "November", "December"
};

char *dow="Su Mo Tu We Th Fr Sa";

/* Tomohiko Sakamoto algo for Gregorian. 0=Sunday. */
unsigned dayofweek (unsigned y, unsigned m, unsigned d)
{
 static int t[12]=
 {
  0, 3, 2, 5,
  0, 3, 5, 1,
  4, 6, 2, 4
 };
 
 if (m<3) y--;
 return (y+(y/4)-(y/100)+(y/400)+t[m-1]+d)%7;
}

/* The above, adapted to Julian. */
unsigned dayofweekj (unsigned y, unsigned m, unsigned d)
{
 static int t[12]=
 {
  0, 3, 2, 5,
  0, 3, 5, 1,
  4, 6, 2, 4
 };
 
 if (m<3) y--;
 return (y+(y/4)+t[m-1]+d+5)%7;
}

int isleap (unsigned year)
{
 if (year>1752)
 {
  if (!(year%4))
  {
   if (!(year%400)) return 1;
   if (!(year%100)) return 0;
   return 1;
  }
  return 0;
 }
 return !(year%4);
}

int monthly (unsigned month, unsigned year)
{
 unsigned d, l, t;
 char spool[22];
 char dstr[3];
 
 /* Bissextile in February on leap years */
 dpm[1]=28+isleap(year);
 
 if ((!month)||(month>12)) return 1;
 if ((year>1752)||((year==1752)&&(month>9)))
  d=dayofweek(year, month, 1);
 else
  d=dayofweekj(year, month, 1);
 
 for (t=8-(year>99)-(strlen(full_month[month-1])>>1); t; --t) putchar(' ');
 printf ("%s %u\n", full_month[month-1], year);
 printf ("%s\n", dow);

 /* Cheat */
 if ((year==1752)&&(month==9))
 {
  printf ("       1  2 14 15 16\n"
          "17 18 19 20 21 22 23\n"
          "24 25 26 27 28 29 30\n"
          "\n"
          "\n"
          "\n");
  return 0;
 }
 
 memset(spool, ' ', 21);
 spool[21]=0;
 
 l=0;
 
 for (t=1; t<=dpm[month-1]; t++)
 {
  sprintf (dstr, "%2d", t);
  spool[d*3]=dstr[0];
  spool[(d*3)+1]=dstr[1];
  d++;
  if (d==7)
  {
   l++;
   d=0;
   printf ("%s\n", spool);
   memset(spool, ' ', 21);
   spool[21]=0;
  }
 }
 printf ("%s\n", spool);
 for (; l<5; l++) putchar('\n');
 
 return 1;
}

int main (int argc, char **argv)
{
 struct tm *tm;
 int e, r;
 unsigned m, y;
 time_t t;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 
 /*
  * Also sprach Posix: "[The environment variable] TZ [is used to] determine
  * the time zone used to calculate the value of the current month."
  * 
  * localtime(3) does quite that.
  */
 t=time(0);
 tm=localtime(&t);

 if (argc==1)
 {
  if (!tm)
  {
   fprintf (stderr, "%s: could not get date from system\n", progname);
   return 1;
  }
  
  return (monthly(tm->tm_mon+1, tm->tm_year+1900));
 }

 /*
  * Ideally, we would want to stack the monthly calendars in a 3x4 grid like
  * System V and BSD implementations do.  But this is easier and actually
  * still Posix-compliant.
  * 
  * Also sprach Posix: "The standard output shall be used to display the
  * calendar, in an unspecified format."  Thus both 3x4 and 1x12 are perfectly
  * cromulent implementations.
  */
 if (argc==2)
 {
  errno=0;
  y=strtoul(argv[1], 0, 0);
  if (errno||(!y)||(y>9999))
  {
   fprintf (stderr, "%s: bogus year '%s'\n", progname, argv[2]);
   return 1;
  }
  r=0;
  for (m=0; m<12; m++)
  {
   e=monthly(m+1, y);
   if (r<e) r=e;
  }
  return r;
 }
 
 if (argc==3)
 {
  errno=0;
  m=strtoul(argv[1], 0, 0);
  if (errno||(!m)||(m>12))
  {
   fprintf (stderr, "%s: bogus month '%s'\n", progname, argv[1]);
   return 1;
  }
  errno=0;
  y=strtoul(argv[2], 0, 0);
  if (errno||(!y)||(y>9999))
  {
   fprintf (stderr, "%s: bogus year '%s'\n", progname, argv[2]);
   return 1;
  }
  
  return monthly(m, y);
 }
 
 return 1;
}
