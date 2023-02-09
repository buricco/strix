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
 * This is a bit rough around the edges.  In particular, -H is unimplemented
 * and -R is untested.  You'll need -lbsd if rolling on Linux.
 */

#ifdef __linux__
#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE 1
#define _GNU_SOURCE 1
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__linux__)||defined(__SVR4__)
#include <setmode.h>
#endif

#define ITERATE_MAX_HANDLES 5

/*
 * NOTE:
 *   Although we support -HLP from 4.4BSD for consistency with other
 *   utilities, we don't document this fact.  No one needs to know right
 *   now...
 *
 *   Also, we don't talk about -f, or Bruno for that matter.
 */

                    /*****************************************************/
#define MODE_R 0x01 /* Recurse                                           */
#define MODE_H 0x02 /* Physical, but resolve links given at command line */
#define MODE_L 0x04 /* Logical                                           */
#define MODE_P 0x08 /* Physical                                          */
#define MODE_F 0x10 /* Force; silent                                     */
#define MODE_V 0x20 /* Verbose                                           */
int mode;           /*****************************************************/

mode_t fmode_num;
void *fmode;
int absolute;
int global_e;

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

int apply (char *filename, int follow)
{
 int e;
 mode_t m;

 /*
  * If we have an absolute mode...that's easy, just use it.
  * If not...get the existing mode and modify it.
  */
 if (absolute)
 {
  m=fmode_num;
 }
 else
 {
  struct stat statbuf;

  e=(follow?stat:lstat)(filename, &statbuf);
  if (e)
  {
   xperror(filename);
   return 1;
  }
  m=getmode(fmode, statbuf.st_mode);
 }

 /* Do the needful. */
 if (mode&MODE_V) printf ("%s\n", filename);
 /* These systems lack lchmod(3) */
#if (defined(__linux__)||defined(__OpenBSD__)||defined(__SVR4__))
 e=chmod(filename, m);
#else
 e=(follow?chmod:lchmod)(filename, m);
#endif
 if (e)
 {
  if (!(mode&MODE_F)) xperror(filename);
 }
 return e;
}

int iterate_hit (const char *filename, const struct stat *statptr,
                 int fileflags, struct FTW *pftw)
{
 int e;

 e=apply((char *)filename, 0);
 if (e>global_e) global_e=e;

 return 0;
}

int do_chmod (char *filename)
{
 if (mode&MODE_R)
 {
  int m;

  if (mode&MODE_L) m=0;
  if (mode&MODE_P) m=FTW_PHYS;

  global_e=0;
  nftw(filename, iterate_hit, ITERATE_MAX_HANDLES, m);
  return global_e;
 }
 else
 {
  return apply(filename, 0);
 }
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-Rv] mode file...\n", progname, progname);
 exit (1);
}

int main (int argc, char **argv)
{
 int e, r, t;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 mode=MODE_P;
 while (-1!=(e=getopt(argc, argv, "HLPRfrv")))
 {
  switch (e)
  {
   case 'H':
    mode&=~(MODE_H|MODE_L|MODE_P);
    mode|=MODE_H;
    break;
   case 'L':
    mode&=~(MODE_H|MODE_L|MODE_P);
    mode|=MODE_L;
    break;
   case 'P':
    mode&=~(MODE_H|MODE_L|MODE_P);
    mode|=MODE_P;
    break;
   case 'r': /* FALL THROUGH */
   case 'R':
    mode|=MODE_R;
    break;
   case 'f':
    mode|=MODE_F;
    break;
   case 'v':
    mode|=MODE_V;
    break;
   default:
    usage();
  }
 }
 if ((mode&(MODE_F|MODE_V))==(MODE_F|MODE_V))
 {
  fprintf (stderr, "%s: -f and -v are mutually exclusive\n", progname);
  return 2;
 }
 if (argc-optind<2) usage();
 if ((argv[optind][0]>='0')&&(argv[optind][0]<='7'))
 {
  absolute=1;
  errno=0;
  fmode_num=strtol(argv[optind], 0, 8);
  if (errno)
  {
   fprintf (stderr, "%s: invalid mode '%s'\n", progname, argv[optind]);
   return 2;
  }
 }
 else
 {
  absolute=0;
  fmode=setmode(argv[optind]);
  if (!fmode)
  {
   fprintf (stderr, "%s: invalid mode '%s'\n", progname, argv[optind]);
   exit(2);
  }
 }

 r=0;
 for (t=optind+1; t<argc; t++)
 {
  e=do_chmod(argv[t]);
  if (r<e) r=e;
 }

 free(fmode);
 return r;
}
