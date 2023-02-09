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
 * Cliff 'em all!
 */

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "signames.h"

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

extern int matchsig (char *);

int main (int argc, char **argv)
{
 char *progname;
 int e, s;
 
 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /* Cowardly die screaming if we are not root. */
 if (geteuid())
 {
  fprintf (stderr, "%s: program requires superuser privileges\n", progname);
  return 1;
 }
 
 if (argc==1)
  s=SIGTERM;
 else
 {
  if (argc!=2)
  {
   fprintf (stderr, "%s: usage: %s [signal]\n", progname, progname);
   return 2;
  }
 
  if (isdigit(*argv[1]))
   s=atoi(argv[1]);
  else
   s=matchsig(argv[1]);
 
  if (s<=0)
   fprintf (stderr, "%s: invalid signal '%s'\n", progname, argv[1]);
 }
 
 e=kill(-1, s);
 if (e==EPERM) fprintf (stderr, "%s: access denied\n", progname);
 return e;
}
