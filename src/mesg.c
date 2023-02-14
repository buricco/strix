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

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2022, 2023 S. V. Nickolas\n";

static char *progname;

/*
 * Die screaming with a message showing correct usage.
 * This is an error condition (see note below).
 */
void usage (void)
{
 fprintf (stderr, "%s: usage: %s {y | n}\n", progname, progname);
 exit(2);
}

int main (int argc, char **argv)
{
 struct stat stat;
 int e, r;
 int ttyhand;
 int m;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /*
  * Validate arguments.
  * If invalid, die screaming.
  * In any case, if there is an error, do NOT return 0 or 1.
  *   Supports System V "-n" and "-y", but we don't document this.
  */
 if (argc>2) usage();
 if (argc==2)
 {
  if (argv[1][0]=='-')
  {
   m=argv[1][1];
   if (argv[1][2]) usage();
  }
  else
  {
   m=*argv[1];
   if (argv[1][1]) usage();
  }

  if ((m!='y')&&(m!='n')) usage();
 }

 /*
  * Find our controlling terminal.  As per POSIX.1-2018.
  * If we fail, die screaming.
  */
 for (ttyhand=0; ttyhand<3; ttyhand++)
  if (isatty(ttyhand)) break;

 if (ttyhand>2)
 {
  fprintf (stderr, "%s: could not find controlling tty\n", progname);
  return 2;
 }

 /*
  * Get mode of the terminal.
  * If we fail, die screaming.
  */
 e=fstat(ttyhand, &stat);
 if (e)
 {
  fprintf (stderr, "%s: could not get mode of controlling tty\n", progname);
  return 2;
 }

 /*
  * No parameters: show whether write(1) would be permitted.
  * Return 0=allowed, 1=not allowed.
  */
 if (argc==1)
 {
  e=(stat.st_mode&S_IWGRP)?0:1;
  printf ("is %c\n", e?'n':'y');
  return e;
 }

 /*
  * Set the appropriate flag on the controlling terminal.
  * Return 0=allowed, 1=not allowed.
  * If we fail, die screaming.
  */
 e=stat.st_mode;
 if (m=='y')
 {
  e |= S_IWGRP;
  r=0;
 }
 else
 {
  e &= (~S_IWGRP);
  r=1;
 }
 if (fchmod(ttyhand, e))
 {
  fprintf (stderr, "%s: could not change mode of controlling tty\n", progname);
  return 2;
 }
 return r;
}
