/*
 * (C) Copyright 2007, 2013, 2020, 2022, 2023 S. V. Nickolas.
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
 * This version combines:
 *
 * 1. Lunaris uname, plus the SysV -S switch.  (The SysV -p switch is not
 *    supported.)
 *
 * 2. Lunaris arch.  This is implemented via uname.  SysV didn't have an arch
 *    command (afaict) but some later distributions do and extend it in ways
 *    that we do not support.
 *
 * 3. Lunaris hostname.  This is, as I understand, the same thing as SysV's
 *    uname -S.
 *
 * 4. Lunaris domainname.  (Not supported on SVR4.)
 */

/*
 * uname
 *
 * switches:
 *   -s system name (default if nothing supplied)
 *   -n node name
 *   -r release name
 *   -v version
 *   -m machine type (default if invoked as "arch")
 *   -a all of the above
 *
 * POSIX says always use this order, no matter in which order the switches
 * were actually entered.
 *
 * When invoked as "arch", with no parameters, acts like the GNU version of
 * arch, which is equivalent to uname -m.
 */

#include <sys/param.h>
#include <sys/utsname.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright=
   "@(#) (C) Copyright 2007, 2013, 2020, 2022, 2023 S. V. Nickolas\n";

#ifdef __SVR4__
#ifndef _POSIX_HOST_NAME_MAX
#define _POSIX_HOST_NAME_MAX 255 /* Linux; should be good enough for here */
#endif

/*
 * Stub functions.
 *
 * You can get gethostname() and sethostname() from -lucb but it doesn't have
 * getdomainname() and setdomainname().  Replace them with dummy versions so
 * the program will link without us having to remove them from the code.
 */
int stub (void)
{
 errno=ENOSYS;
 return -1;
}

#define getdomainname(x,y) stub()
#define setdomainname(x,y) stub()
#endif

static void usage_domainname (char *pname)
{
 fprintf (stderr, "%s: usage: %s [name]\n", pname, pname);
 exit (1);
}

static void usage_hostname (char *pname)
{
 fprintf (stderr, "%s: usage: %s [-s | name]\n", pname, pname);
 exit (1);
}

static void usage_arch (char *pname)
{
 fprintf (stderr, "%s: usage: %s\n", pname, pname);
 exit (1);
}

static void usage_uname (char *pname)
{
 fprintf (stderr, "%s: usage: %s [-amnrsv]\n"
                  "%s: usage: %s -S name\n", pname, pname, pname, pname);
 exit(1);
}

int main (int argc, char **argv)
{
 int c;
 int synerr;
 char *progname;
 struct utsname utsname;

 int s,n,r,v,m;
 int d,H,e,S;

 char h[_POSIX_HOST_NAME_MAX+1];
 char *p;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 d=(!strcmp(progname,"domainname"));
 H=(!strcmp(progname,"hostname"));

 if (d)
 {
  if (argc>2) usage_domainname(progname);

  if (argc==1)
  {
   if (getdomainname(h,_POSIX_HOST_NAME_MAX))
   {
    perror ("getdomainname()");
    return 1;
   }
   printf ("%s\n", h);
   return 0;
  }

  if (setdomainname(argv[1],strlen(argv[1])))
  {
   perror(argv[1]);
   return 1;
  }
 }
 else if (H)
 {
  while (-1!=(e=getopt(argc, argv, "s")))
  {
   switch (e)
   {
    case 's':
     s=1;
     break;
    default:
     usage_hostname(progname);
   }
  }

  if (s && (optind<argc))
  {
   fprintf (stderr, "%s: -s and name are mutually exclusive\n", progname);
   return 1;
  }

  if (argc-optind>1) usage_hostname(progname);

  if (argc==optind)
  {
   if (gethostname(h,_POSIX_HOST_NAME_MAX))
   {
    perror ("gethostname()");
    return 1;
   }

   if (s)
   {
    char *p;

    p=strchr(h,'.');
    if (p) *p=0;
   }

   printf ("%s\n", h);
   return 0;
  }
  else
  {
   if (sethostname(argv[optind],strlen(argv[optind])))
   {
    perror(argv[optind]);
    return 1;
   }
  }
 }

 s=n=r=v=m=S=0;

 synerr=0;

 if (!strcmp(progname, "arch"))
 {
  if (argc!=1)
  {
   usage_arch(progname);
  }
  m=1;
 }
 else
 {
  p=0;

  while (-1!=(c=getopt(argc, argv, "snrvmaS:")))
  {
   switch (c)
   {
    case 's':
     s=1;
     break;
    case 'n':
     n=1;
     break;
    case 'r':
     r=1;
     break;
    case 'v':
     v=1;
     break;
    case 'm':
     m=1;
     break;
    case 'a':
     s=n=r=v=m=1;
     break;
    case 'S':
     S=1;
     p=optarg;
     break;
    case '?':
     synerr++;
   }
  }
 }

 if (S)
 {
  if (s+n+r+v+m) usage_uname(progname);

  if (sethostname(p,strlen(p)))
  {
   perror(p);
   return 1;
  }
 }

 if (synerr)
 {
  usage_uname(progname);
 }

 if (!(s||n||r||v||m)) s=1;

 /* SVR4 may return a positive value */
 if (uname(&utsname)<0)
 {
  perror(progname);
  return 2;
 }

 if (s)
 {
  printf ("%s",utsname.sysname);
  if (n||r||v||m) printf (" ");
 }

 if (n)
 {
  printf ("%s",utsname.nodename);
  if (r||v||m) printf (" ");
 }

 if (r)
 {
  printf ("%s",utsname.release);
  if (v||m) printf (" ");
 }

 if (v)
 {
  printf ("%s",utsname.version);
  if (m) printf (" ");
 }

 if (m)
 {
  printf ("%s",utsname.machine);
 }

 printf ("\n");
 return 0;
}
