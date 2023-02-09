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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2022, 2023 S. V. Nickolas\n";

static char *progname;

/* prerror(3) with the name of the utility AND the name of the input file. */
void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

int main (int argc, char **argv)
{
 int a,c,e,r,s,t;
 FILE **files;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /*
  * Set defaults, then process switches.
  *
  * -a = open for append, rather than write.
  * -i = ignore SIGINT.
  *
  * s = stdout is usable.
  */
 s=1;
 a=0;
 while (-1!=(e=getopt(argc, argv, "ai")))
 {
  switch (e)
  {
   case 'a':
    a=1;
    break;
   case 'i':
    signal(SIGINT,SIG_IGN);
    break;
   default:
    fprintf (stderr, "%s: usage: %s [-ai] [file ...]\n", progname, progname);
    return 1;
  }
 }

 /* Allocate enough file handles to blast to all specified files. */
 c=argc-optind;
 files=calloc(c, sizeof (FILE*));
 if (!files)
 {
  fprintf (stderr, "%s: out of memory\n", progname);
  return 0;
 }

 /*
  * Open as many files as we can.
  *
  * A failed open is an error condition, but not a fatal one.  Just mark it
  * and keep going.
  */
 r=0;
 for (t=optind, c=0; t<argc; t++, c++)
 {
  files[c]=fopen(argv[t], a?"a":"w");
  if (!files[c])
  {
   xperror(argv[t]);
   r=1;
  }
 }

 while (1)
 {
  c=getchar();

  /*
   * End of file or error condition?
   * The former means we're done.
   * The latter is a fatal error, so die screaming.
   */
  if (c<0)
  {
   if (!feof(stdin))
   {
    r=1;
    xperror("(stdin)");
   }

   /* Clean up and quit. */
   for (t=0; t<(argc-optind); t++) fclose(files[t]);
   return r;
  }

  if (s)
  {
   e=putchar(c);

   /*
    * Error condition on stdout!
    * Shut it down, mark error condition and continue.
    */
   if (e<0)
   {
    xperror("(stdout)");
    r=1;
    s=0;
   }
  }

  /*
   * Output to each file.
   * If any have an error condition, close them and mark them.
   */
  for (t=0; t<(argc-optind); t++)
  {
   if (files[t])
   {
    e=fputc(c, files[t]);
    if (e<0)
    {
     xperror(argv[t+optind]);
     r=1;
     fclose(files[t]);
     files[t]=0;
    }
   }
  }
 }

 /* NOTREACHED */
}
