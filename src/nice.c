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

void bogus (char *what)
{
 fprintf (stderr, "%s: niceness '%s' is bogus\n", progname, what);
 exit(1);
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-n increment] command\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int index;
 int niceness;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (argc<2) usage();

 niceness=10;

 /*
  * Do not use getopt(); instead, handle the parsing ourselves.
  * Using getopt() might mess with handling of parameters to commands,
  * especially if it permutes (like GNU's).
  */
 index=1;
 if (*(argv[1])=='-')
 {
  if (!strcmp(argv[1], "-n"))
  {
   index=3;
   if (argc<4) usage();
   niceness=atoi(argv[2]);
   if ((!niceness)&&(argv[2][0]!='0')) bogus(argv[2]);
  }
  else if (!strncmp(argv[1], "-n", 2))
  {
   index=2;
   if (argc<3) usage();
   niceness=atoi(&(argv[1][2]));
   if ((!niceness)&&(argv[1][2]!='0')) bogus(&(argv[1][2]));
  }
  else if ((argv[1][1]>='0')&&(argv[1][1]<='9'))
  {
   /* Do not still use this format */
   index=2;
   if (argc<3) usage();
   niceness=atoi(&(argv[1][1]));
   if ((!niceness)&&(argv[1][1]!='0')) bogus(&(argv[1][1]));
  }
  else
  {
   fprintf (stderr, "%s: invalid switch '%s'\n", progname, argv[1]);
   usage();
  }
 }


 errno=0;
 if (nice(niceness)==-1)
 {
  if (errno) /* It's OK, POSIX says to do this */
  {
   fprintf (stderr, "%s: failed to set niceness to %d: %s\n",
            progname, niceness, strerror(errno));
  }
  return 1;
 }

 execvp(argv[index], argv+index);
 perror(argv[index]);
 return 127-(errno!=ENOENT);
}
