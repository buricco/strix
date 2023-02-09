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
#include <unistd.h>

#ifdef __SVR4__
#include "getline.h"
#endif

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

int do_head (char *filename, long count)
{
 char *buf;
 int isstdin;
 FILE *file;
 int e, r;
 long t;

 if (!strcmp(filename, "-"))
 {
  isstdin=1;
  file=stdin;
  rewind(stdin);
 }
 else
 {
  file=fopen(filename, "rt");
  if (!file)
  {
   xperror(filename);
   return 1;
  }
 }

 buf=0;
 r=0;
 for (t=0; t<count; t++)
 {
  size_t x;

  if (buf)
  {
   free(buf);
   buf=0;
  }
  e=getline(&buf, &x, file);
  if (e==-1)
  {
   if (feof(file)) break;
   xperror(filename);
   r=1;
   break;
  }
  printf("%s", buf);
 }
 if (isstdin) rewind(stdin); else fclose(file);
 return r;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-n lines] [file ...]\n", progname, progname);
 exit (1);
}

int main (int argc, char **argv)
{
 int e, gotchas, r, t;
 long count;

 count=10;
 gotchas=0;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /* Old BSD switch format */
 if (argc>1)
 {
  if (argv[1][0]=='-')
  {
   if ((argv[1][1]>='0')&&(argv[1][1]<='9'))
   {
    errno=0;

    count=strtol(&(argv[1][1]), 0, 0);
    if (errno)
    {
     fprintf (stderr, "%s: bogus line count: '%s'\n",
              progname, &(argv[1][1]));
     return 1;
    }
    gotchas++;
    argv++;
    argc--;
   }
  }
 }

 while (-1!=(e=getopt(argc, argv, "n:")))
 {
  switch (e)
  {
   case 'n':
    errno=0;

    count=strtol(optarg, 0, 0);
    if (errno)
    {
     fprintf (stderr, "%s: bogus line count: '%s'\n", progname, optarg);
     return 1;
    }
    break;
   default:
    usage();
  }
 }

 if (argc==optind)
  return do_head("-", count);

 r=0;
 for (t=optind; t<argc; t++)
 {
  e=do_head(argv[optind], count);
  if (r<e) r=e;
 }
 return r;
}
