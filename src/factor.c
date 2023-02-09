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

static char *progname;

#ifdef __SVR4__
typedef unsigned long xulong;
#define strtoxul strtoul
#define PXUL "%lu"
#else
typedef unsigned long long xulong;
#define strtoxul strtoull
#define PXUL "%llu"
#endif

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

int do_factor (char *x)
{
 xulong n, t, v;

 errno=0;
 n=strtoxul(x, 0, 0);
 if (errno)
 {
  fprintf (stderr, "%s: bogus value '%s'\n", progname, x);
  return 1;
 }
 
 printf (PXUL ":", n);
 v=n;
 for (t=2; t<=v; t++)
 {
  while (!(v%t))
  {
   v=v/t;
   printf (" " PXUL, t);
  }
 }
 printf ("\n");
 return 0;
}

int main (int argc, char **argv)
{
 char buf[128];
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (argc>2)
 {
  fprintf (stderr, "%s: usage: %s [value]\n", progname, progname);
  return 1;
 }
 
 if (argc==2)
  return do_factor(argv[1]);
  
 while (1)
 {
  fgets(buf, 127, stdin);
  if (feof(stdin)) return 0;
  buf[strlen(buf)-1]=0;
  do_factor(buf);
 }
}
