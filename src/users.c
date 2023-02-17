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

#if defined(__linux__)||defined(__OpenBSD__)
#include <utmp.h>
#else
#include <utmpx.h>
#endif

static char *progname;

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [file]\n");
 exit(1);
}

int main (int argc, char **argv)
{
 int space;
 FILE *file;
#if defined(__linux__)||defined(__OpenBSD__)
 struct utmp utmp;
#else
 struct utmpx utmp;
#endif
 char *utmpfile;

 if (argc==1)
  utmpfile="/var/run/utmp";
 else if (argc==2)
  utmpfile=argv[1];
 else
  usage();

 file=fopen(utmpfile, "rb");
 if (!file)
 {
  fprintf(stderr, "%s: %s: %s", progname, utmpfile, strerror(errno));
  return 1;
 }
 
 space=0;
 while (1)
 {
  if (!(fread(&utmp, sizeof(utmp), 1, file))) break;
  
  if (utmp.ut_type!=USER_PROCESS) continue;
  
  if (space)
   putchar(' ');
  else
   space=1;
  
  printf ("%s", utmp.ut_name);
 }
 fclose(file);
 putchar('\n');
 return 0;
}
