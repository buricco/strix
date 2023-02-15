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

/*
 * This implementation is braindamaged.
 */

#include <sys/stat.h>
#include <errno.h>
#if defined(__linux__)||defined(__OpenBSD__)
#include <utmp.h>
#else
#include <utmpx.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char *progname;

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

#ifdef __SVR4__
#include <ctype.h>

#define UTX_LINESIZE 31 /* extrapolated from utmpx.h */

int strcasecmp (const char *a, const char *b)
{
 char *aa, *bb;

 aa=(char *)a; bb=(char *)b;
 while (toupper(*aa++)==toupper(*bb++))
  if (!*aa) return 0; /* Match. */
 bb--;
 return *aa-*bb;
}
#endif

int do_who (char *filename, int meonly, int showidle, int header, 
            int writable, int type)
{
 FILE *file;
#if defined(__linux__)||defined(__OpenBSD__)
 struct utmp utmp;
#else
 struct utmpx utmp;
#endif
 struct tm *tm;
 time_t ttime;
 char datebuf[20];
 char *mytty, *p;
 struct stat s;

 if (header)
 {
  printf ("NAME     ");
  if (writable) printf ("  ");
  printf ("LINE         TIME             ");
  if (showidle) printf ("IDLE                  ");
  printf ("COMMENT\n");
  return 0;
 }
 
 mytty=ttyname(fileno(stdin));
 if (meonly&&(!mytty))
 {
  fprintf (stderr, "%s: could not obtain identity of controlling tty\n", 
           progname);
  return 1;
 }
 if (!strncmp(mytty, "/dev/", 5))
  p=&(mytty[5]);
 else if (meonly)
 {
  fprintf (stderr, "%s: ttyname is bogus\n", progname);
  return 1;
 }
 
 file=fopen(filename, "rb");
 if (!file)
 {
  fprintf(stderr, "%s: %s: %s", progname, filename, strerror(errno));
  return 1;
 }
 while (1)
 {
  if (!(fread(&utmp, sizeof(utmp), 1, file))) break;
  
  ttime=utmp.ut_tv.tv_sec;
  
  if (utmp.ut_type!=type) continue;
  if (meonly)
#if defined(__linux__)||defined(__OpenBSD__)
   if (strncmp(p,utmp.ut_line,UT_LINESIZE)) continue;
#else
   if (strncmp(p,utmp.ut_line,UTX_LINESIZE)) continue;
#endif

  tm=localtime(&ttime);
  if (!tm)
  {
   fprintf (stderr, "%s: %s: bogus time\n", progname, filename);
   continue;
  }
  strftime(datebuf, 19, "%F %R", tm);
  printf ("%-8s ", utmp.ut_name);
  if (writable)
  {
   char *b;
   
#if defined(__linux__)||defined(__OpenBSD__)
   b=malloc(UT_LINESIZE+6);
#else
   b=malloc(UTX_LINESIZE+6);
#endif
   if (!b) /* SCRAM! */
   {
    fprintf (stderr, "%s: out of memory\n", progname);
    fclose(file);
    return 1;
   }
   sprintf(b,"/dev/%s",utmp.ut_line);
   if (stat(b, &s))
    fputc ('?', stdout);
   else
    fputc ((s.st_mode&0020)?'+':'-', stdout);
   fputc(' ',stdout);
   free(b);
  }
  printf ("%-12s %s ", (type==RUN_LVL)?utmp.ut_id:utmp.ut_line, datebuf);
  if (showidle)
  {
   char *b;
   
#if defined(__linux__)||defined(__OpenBSD__)
   b=malloc(UT_LINESIZE+6);
#else
   b=malloc(UTX_LINESIZE+6);
#endif
   if (!b) /* SCRAM! */
   {
    fprintf (stderr, "%s: out of memory\n", progname);
    fclose(file);
    return 1;
   }
   sprintf(b,"/dev/%s",utmp.ut_line);
   if (stat(b, &s))
   {
    fprintf (stderr, "%s: stat failed on %s: %s\n", 
             progname, b, strerror(errno));
   }
   else
   {
    unsigned long idle;
    unsigned m,h,d;
    
    idle=time(0)-s.st_atim.tv_sec;
    d=idle/86400;
    h=(idle%86400)/3600;
    m=(idle%3600)/60;
    printf ("%5u days, %2u:%02u:%02u  ", d,h,m,(unsigned char)(idle%60));
   }
   free(b);
  }
  printf  ("(%s)\n", utmp.ut_host);
 }
 fclose(file);
 return 0;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-abdHlmprstTu] [file]\n"
                  "%s: usage: %s -q [file]\n"
                  "%s: usage: who am i\n", 
          progname, progname, progname, progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e;
 int b,d,H,l,m,p,r,s,t,T,u;
 int q;
 
 char *utmpfile="/var/run/utmp";

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 
 b=d=H=l=m=p=r=t=T=u=0;
 s=2;
 q=0;

 /*
  * Traditionally, this would take any combination of two parameters,
  * but that would require endrunning around getopt().
  * Instead, take the strict approach and ONLY support the specific pairings
  * "am i" and "am I", which POSIX requires.
  * 
  * Thus, treat these pairings as equivalent to "who -m".
  */ 
 
 if (argc==3)
 {
  if (!strcmp(argv[1],"am"))
   if (!strcasecmp(argv[2], "i"))
    return do_who(utmpfile,m,0,0,0,USER_PROCESS);
 }
 
 while (-1!=(e=getopt(argc, argv, "abdHlmpqrstTu")))
 {
  switch (e)
  {
   case 'a':
    b=d=l=p=r=s=t=T=u=1;
    break;
   case 'b':
    b=1;
    break;
   case 'd':
    d=1;
    break;
   case 'H':
    H=1;
    break;
   case 'l':
    l=1;
    break;
   case 'm':
    m=1;
    break;
   case 'p':
    p=1;
    break;
   case 'q':
    q=1;
    break;
   case 'r':
    r=1;
    break;
   case 's':
    s=1;
    break;
   case 't':
    t=1;
    break;
   case 'T':
    T=1;
    break;
   case 'u':
    u=1;
    break;
   default:
    usage();
  }
 }

 if (argc-optind>1) usage();
 if (argc-optind) utmpfile=argv[optind];
 
 if (s==2)
 {
  if (b||d||l||p||r||t) s=0;
 }
 
 e=0;

 if (H) do_who(0,0,u,1,T,0);
 if (b) e+=do_who(utmpfile,m,u,0,T,BOOT_TIME); 
 if (d) e+=do_who(utmpfile,m,u,0,T,DEAD_PROCESS);
 if (l) e+=do_who(utmpfile,m,u,0,T,LOGIN_PROCESS);
 if (p) e+=do_who(utmpfile,m,u,0,T,INIT_PROCESS);
 if (r) e+=do_who(utmpfile,m,u,0,T,RUN_LVL);
 if (t) e+=do_who(utmpfile,m,u,0,T,NEW_TIME);
 if (s) e+=do_who(utmpfile,m,u,0,T,USER_PROCESS);
 
 return e;
}
