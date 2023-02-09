/*
 * (C) Copyright 2022, 2023 S. V. Nickolas.
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
 * POSIX specifies:
 *
 * -a: change only the ACCESS time
 * -m: change only the MODIFICATION time
 *     Default to both.  By setting the struct's tv_nsec to UTIME_OMIT no
 *     change is made.
 *
 * The default timestamp is now (UTIME_NOW), but:
 * -r: take the timestamp from this file
 * -t: parse "[[cc]yy]mmddhhmm[.ss]" as a timestamp
 * -d: parse "ccyy-mm-ddThh:mm:SS[{. | ,}cc][Z]" as a timestamp, where:
 *     T is the literal letter T, or a space.
 *     Z means use Zulu time instead of local time.
 *
 * If file not found and -c is not set:
 *   * creat(2) it with mode
 *     (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH), i.e., 0666
 *   * set the struct for futimens(2) with the appropriate access and
 *     modification times.
 *
 * If file not found and -c is set, silently die (don't scream).
 *
 * Otherwise:
 *   * utimensat(2) it, with the first arg set to AT_FDCWD and flags set to 0,
 *     and set the array as above.
 *
 * The SVR4 version did not need -t.  This version does.
 *
 * BSD's -f switch is silently ignored.
 */

/*
 * NOTE: The BSD extension timegm() is used in this code, which is a version
 *       of mktime() that uses Zulu time instead of local time.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MODE_A 0x01
#define MODE_M 0x02
#define MODE_C 0x04
char mode;

#define DEFMOD (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH)

struct timespec my_times[2];

static char *copyright="@(#) (C) Copyright 2022, 2023 S. V. Nickolas\n";

static char *progname;

/*
 * Modified version of perror(3) that shows the program name
 * (as uso believes a *x app should do with all diagnostics).
 */
void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

/*
 * SysV style timestamp argument into my_times[].
 */
int timeize1 (char *s)
{
 unsigned long l;
 char *p;
 char sec, month, day, hour, minute;
 char element[5];
 int year;
 struct tm *tm1, tm2;
 time_t t;

 t=time(0);
 tm1=localtime(&t);

 /* Sanity check */
 if (!tm1) return -1;

 /* Find seconds if specified; then strip them out of the string. */
 sec=0;
 if (0!=(p=strchr(s, '.')))
 {
  *p=0;
  p++;

  errno=0; /* documented */
  l=strtol(p,0,10);
  if (errno) return -1;
  if (strlen(p)==1) l *= 10;
  if (l>60) return -1;
  sec=l;
 }

 /*
  * Length is 8: mmddhhmm
  * Length is 10: yymmddhhmm (assume 19xx if yy>=69)
  * Length is 12: ccyymmddhhmm
  *
  * Otherwise, return error.
  */
 if (strlen(s)==12)
 {
  strncpy(element, s, 4);
  element[4]=0;
  p=s+4;
  errno=0; /* documented */
  year=strtol(element,0,10);
  if (errno) return -1;
 }
 else if (strlen(s)==10)
 {
  strncpy(element, s, 2);
  element[2]=0;
  p=s+2;
  errno=0; /* documented */
  year=strtol(element,0,10);
  if (errno) return -1;
  if (year<70) /* Also sprach POSIX */
   year+=2000;
  else
   year+=1900;
 }
 else if (strlen(s)==8)
 {
  p=s;
  year=tm1->tm_year+1900;
 }
 else
  return -1;

 /* Month */
 strncpy(element, p, 2);
 element[2]=0;
 p+=2;
 errno=0; /* documented */
 month=strtol(element,0,10);
 if (errno) return -1;

 /* Day */
 strncpy(element, p, 2);
 element[2]=0;
 p+=2;
 day=strtol(element,0,10);
 if (errno) return -1;

 /* Hour */
 strncpy(element, p, 2);
 element[2]=0;
 p+=2;
 hour=strtol(element,0,10);
 if (errno) return -1;

 /* Minute */
 strncpy(element, p, 2);
 element[2]=0;
 p+=2;
 minute=strtol(element,0,10);
 if (errno) return -1;

 tm2.tm_sec=sec;
 tm2.tm_min=minute;
 tm2.tm_hour=hour;
 tm2.tm_mday=day;
 tm2.tm_mon=month;
 tm2.tm_year=year-1900;
 tm2.tm_wday=-1;
 tm2.tm_yday=-1;
 tm2.tm_isdst=-1;
 t=mktime(&tm2);
 if (t==-1) return -1;

 my_times[0].tv_sec=my_times[1].tv_sec=t;
 my_times[0].tv_nsec=my_times[1].tv_nsec=0;
 return 0;
}

/*
 * POSIX-style timestamp argument into my_times[].
 */
int timeize2(char *s)
{
 char *p;
 int hms;
 int zulu;
 int year;
 char sec, month, day, hour, minute;
 long nano;
 time_t t;
 struct tm tm;

 nano=-1;
 zulu=0;
 hms=0;

 /* Not enough of a string to parse */
 if (strlen(s)<10) return -1;

 /* Doesn't start with ????-??-?? */
 if ((s[4]!='-')||(s[7]!='-')) return -1;

 /* If the string ends with Z, enable zulu mode and remove the Z. */
 if (s[strlen(s)-1]=='Z')
 {
  zulu=1;
  s[strlen(s)-1]=0;
 }

 p=s+5;

 errno=0; /* documented */
 s[4]=0;
 year=strtol(s,0,10);
 if (errno) return -1;
 s[7]=0;
 month=strtol(s+5,0,10);
 if (errno) return -1;

 /*
  * T or space only between date and time.
  * End of string: time is midnight
  */
 if ((s[10]=='T')||(s[10]==' '))
  hms=1;
 else if (s[10])
  return -1;

 s[10]=0;
 day=strtol(s+8,0,10);
 if (errno) return -1;
 if (hms)
 {
  char *q;
  s[13]=0;

  p=s+14;
  q=strchr(p,':');
  if (!q) return -1;
  *q=0;
  errno=0; /* documented */
  hour=strtol(p,0,10);
  if (errno) return -1;
  p=q+1;
  q=strchr(p,':');
  if (!q) /* no seconds */
   sec=nano=0;
  else
   *(q++)=0;

  minute=strtol(p,0,10);
  if (errno) return -1;
  if (q) /* seconds found */
  {
   char c_nano[10];
   p=q;

   q=strchr(p,'.');
   if (!q) q=strchr(p,',');

   if (!q) /* no nanoseconds */
   {
    nano=0;
   }
   else
   {
    strcpy(c_nano, q+1);
    while (strlen(c_nano)<9)
     strcat(c_nano, "0");
   }
   errno=0; /* documented */
   nano=strtol(c_nano,0,10);
  }
 }
 else
 {
  hour=minute=sec=nano=0;
 }

 tm.tm_sec=sec;
 tm.tm_min=minute;
 tm.tm_hour=hour;
 tm.tm_mday=day;
 tm.tm_mon=month;
 tm.tm_year=year-1900;
 tm.tm_wday=-1;
 tm.tm_yday=-1;
 tm.tm_isdst=-1;

 if (zulu)
  t=timegm(&tm);
 else
  t=mktime(&tm);

 if (t==-1) return -1;

 my_times[0].tv_sec=my_times[1].tv_sec=t;
 my_times[0].tv_nsec=my_times[1].tv_nsec=0;
 return 0;
}

/* Get timestamps from file. */
int timeize3(char *s)
{
 int e;
 struct stat statbuf;

 e=stat(s, &statbuf);

 /* An error occurred. */
 if (e) return -1;

 my_times[0].tv_sec=statbuf.st_atim.tv_sec;
 my_times[0].tv_nsec=statbuf.st_atim.tv_nsec;
 my_times[1].tv_sec=statbuf.st_mtim.tv_sec;
 my_times[1].tv_nsec=statbuf.st_mtim.tv_nsec;

 return 0;
}

int do_touch (char *filename)
{
 int e;
 struct stat statbuf;

 if (!(mode&MODE_A)) my_times[0].tv_nsec=UTIME_OMIT;
 if (!(mode&MODE_M)) my_times[1].tv_nsec=UTIME_OMIT;

 e=stat(filename, &statbuf);
 if (e)
 {
  int h;

  /*
   * -c specified, silently fail.
   *
   * POSIX says to return 0 even if we fail; original implementation would
   * silently return an error in this case.
   */
  if (mode&MODE_C) return 0;

  h=creat(filename, DEFMOD);
  if (h<0) return -1;
  e=futimens(h, my_times);
  close(h);
  return e;
 }

 return utimensat(AT_FDCWD, filename, my_times, 0);
}

void usage (void)
{
 fprintf (stderr,
          "%s: usage: %s [-acm] [-d yyyy-mm-dd[Thh:mm:ss[.nnnnnnnnn][Z]]]\n"
          "                    "
          "[-t [[yy]yy]mmddhhmm[.ss]] [-r file] filename...\n",
          progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e, r, t;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /* Default timestamp. */
 my_times[0].tv_nsec=my_times[1].tv_nsec=UTIME_NOW;

 mode=0;

 while (-1!=(e=getopt(argc, argv, "acd:fmr:t:")))
 {
  switch (e)
  {
   case 'a':
    mode |= MODE_A;
    break;
   case 'c':
    mode |= MODE_C;
    break;
   case 'd':
    if (timeize2(optarg)) usage();
    break;
   case 'f': /* NOP */
    break;
   case 'm':
    mode |= MODE_M;
    break;
   case 'r':
    if (timeize3(optarg))
    {
     xperror(optarg);
     return 1;
    }
   case 't':
    if (timeize1(optarg)) usage();
    break;
   default:
    usage();
  }
 }

 /* No files were specified. */
 if (optind==argc) usage();

 /* If neither -a or -m, use both -a AND -m. */
 if (!(mode&(MODE_A|MODE_M))) mode|=(MODE_A|MODE_M);

 /* Iterate over the specified files. */
 r=0;
 for (t=optind; t<argc; t++)
 {
  if (0!=(e=do_touch(argv[t])))
  {
   /*
    * Return 0 = success
    * Return -1 = scream
    * Return 1 = silently fail
    */
   if (e<0) xperror(argv[t]);

   r=1;
  }
 }

 return r;
}
