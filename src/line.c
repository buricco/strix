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

#include <stdio.h>
#include <limits.h>

#ifndef ARG_MAX
#define ARG_MAX 4096
#endif

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

int main (int argc, char **argv)
{
 char *progname;
 char buf[ARG_MAX];

 progname=argv[0];
 while (*(progname++));
 while ((--progname)>argv[0]) if (*progname=='/') {progname++; break;}

 if (argc!=1)
 {
  fprintf (stderr, "%s: usage: %s\n", progname, progname);
  return 1;
 }

 fgets (buf, ARG_MAX-1, stdin);
 if (feof(stdin))
 {
  /* At least print a newline, then return 1 */
  fputc ('\n', stdout);
  return 1;
 }
 fputs (buf, stdout);
 return 0;
}
