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
 * Use -lbsd on Linux for getmode() and setmode();
 */

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__linux__)||defined(__SVR4__)
#include <setmode.h>
#endif

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

mode_t getmode(const void *bbox, mode_t omode);
void *setmode(const char *p);

static char *progname;

void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-m mode] [-p] dirname ...\n",
          progname, progname);
 exit(1);
}

int mkdir_with_parents (char *path, mode_t mode)
{
 if (strrchr(path,'/'))
 {
  int e;
  char *p;

  p=malloc(strlen(path)+1);
  if (!p) return -1;
  strcpy(p,path);
  *(strrchr(p,'/'))=0;

  /* Ignore errors */
  mkdir_with_parents(p, mode);
  free(p);
 }

 return mkdir(path, mode);
}

int main (int argc, char **argv)
{
 void *m;
 int r,s;
 int e, mode, pflag;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 mode=0755;

 while (-1!=(e=getopt(argc, argv, "m:p")))
 {
  switch (e)
  {
   case 'm':
    m=setmode(optarg);
    if (!m)
    {
     fprintf (stderr, "%s: invalid mode '%s'\n", argv[0], optarg);
     return 1;
    }
    mode=getmode(m,0755);
    free(m);
   case 'p':
    pflag=1;
    break;
   default:
    usage();
  }
 }

 if (optind==argc) usage();

 r=0;

 for (e=optind; e<argc; e++)
 {
  if (pflag)
  {
   if (0!=(s=mkdir_with_parents(argv[optind], mode))) xperror(argv[optind]);
  }
  else
  {
   if (0!=(s=mkdir(argv[optind], mode))) xperror(argv[optind]);
  }
  r+=s;
 }

 return r;
}
