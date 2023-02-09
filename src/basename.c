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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __SVR4__
#include <base_dirname.h>
#else
#include <libgen.h>
#endif

static char *copyright="@(#) (C) Copyright 2022, 2023 S. V. Nickolas\n";

static char *progname;

void usage_basename (void)
{
 fprintf (stderr, "%s: usage: %s string [suffix]\n", progname, progname);
 exit (1);
}

void usage_dirname (void)
{
 fprintf (stderr, "%s: usage: %s string\n", progname, progname);
 exit (1);
}

int main (int argc, char **argv)
{
 int f;
 char *r;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 f=(!strncmp(progname, "dir", 3));
 if ((argc>3)||((argc==3)&&f)) f?usage_dirname():usage_basename();
 r=(f?dirname:basename)(argv[1]);
 if ((!f)&&(argc==3))
 {
  if (strlen(argv[1])>=strlen(argv[2]))
  {
   if (!strcmp(&(argv[1][strlen(argv[1])-strlen(argv[2])]), argv[2]))
    (argv[1][strlen(argv[1])-strlen(argv[2])])=0;
  }
 }
 printf ("%s\n", (f?dirname:basename)((argc>1)?argv[1]:""));
 return 0;
}
