/*
 * (C) Copyright 2020, 2023 S. V. Nickolas.
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

#ifdef __SVR4__
#define _XOPEN_SOURCE 500
#endif

#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2020, 2023 S. V. Nickolas\n";

int tracelink (char *filename, int recurse, int nonl)
{
 struct stat st;
 off_t sz;
 char p[PATH_MAX+1], *q;
 char *s;

 s=nonl?"%s":"%s\n";

 memset(p,0,PATH_MAX+1);

 if (lstat(filename,&st))
 {
  perror(filename);
  return -1;
 }

 if (st.st_mode&S_IFLNK) return 1;

 if (recurse)
 {
  q=realpath(filename, p);
  if (!q) return -1;

  printf (s, p);
  return 0;
 }
 else
 {
  sz=st.st_size;
  readlink(filename,p,PATH_MAX);
  printf (s, p);
  return 0;
 }
}

void usage(char *argv0)
{
 fprintf (stderr, "%s: usage: %s [-fn] filename \n", argv0, argv0);
 exit(1);
}

int main (int argc, char **argv)
{
 int e, f, n;
#ifdef __SVR4__ /* braindead libc */
 extern int optind;
#endif

 f=n=0;

 while (-1!=(e=getopt(argc, argv, "fn")))
 {
  switch (e)
  {
   case 'f':
    f++;
    break;
   case 'n':
    n++;
    break;
   default:
    usage(argv[0]);
  }
 }

 if ((argc-optind)!=1) usage(argv[0]);

 return tracelink (argv[optind], f, n);
}
