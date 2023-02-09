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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

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
 fprintf (stderr, "%s: usage: %s [-p] dirname ...\n", progname, progname);
 exit(1);
}

int rmdir_with_parents (char *path)
{
 int e;

 e=rmdir(path);
 if (e) return e;

 if (strrchr(path,'/'))
 {
  char *p;

  p=malloc(strlen(path)+1);
  if (!p) return -1;
  strcpy(p,path);
  *(strrchr(p,'/'))=0;

  e=rmdir_with_parents(p);
  free(p);
  return e;
 }
 return 0;
}

int main (int argc, char **argv)
{
 int e, pflag, r, s;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 while (-1!=(e=getopt(argc, argv, "ps")))
 {
  switch (e)
  {
   case 'p':
    pflag=1;
    break;
   case 's': /* compatibility NOP */
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
   if (0!=(s=rmdir_with_parents(argv[optind]))) xperror(argv[optind]);
  }
  else
  {
   if (0!=(s=rmdir(argv[optind]))) xperror(argv[optind]);
  }

  r+=s;
 }

 return r;
}
