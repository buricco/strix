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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2020, 2023 S. V. Nickolas\n";

int main (int argc, char **argv)
{
#ifdef __SVR4__ /* braindead libc */
 extern int optind;
#endif
 char buf[PATH_MAX];

 int e, q, r;

 q=0;

 while (-1!=(e=getopt(argc, argv, "q")))
 {
  switch (e)
  {
   case 'q':
    q=1;
    break;
   default:
    fprintf (stderr, "%s: usage: %s [-q] [path ...]\n", argv[0], argv[0]);
    return 1;
  }
 }

 if (optind==argc)
 {
  if (!realpath(".",buf))
  {
   perror(".");
   return 1;
  }
  printf ("%s\n", buf);
  return 0;
 }

 r=0;
 for (e=optind; e<argc; e++)
 {
  if (!realpath(argv[e], buf))
  {
   r++;
   if (!q) perror(argv[e]);
  }
  else
   printf ("%s\n", buf);
 }
 return r;
}
