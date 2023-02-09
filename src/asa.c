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

/*
 * First character is:
 *   space: write line as normal
 *   '0':   write a newline, then write line as normal
 *   '1':   write a form feed, then write line as normal
 *   '+':   replace the previous newline with a carriage return.
 *   anything else: undefined, but recommend write line as normal.
 *
 * Return nonzero on error.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define BUFSZ 1024

static char *copyright="@(#) (C) Copyright 2022, 2023 S. V. Nickolas\n";

static char *progname;

void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

int do_asa (char *filename)
{
 void *e;
 int squelch;
 int isstdin;
 FILE *file;
 char buf[BUFSZ];

 /* Reset stdin flag. */
 isstdin=0;

 /* Prevent a newline from going at the beginning. */
 squelch=1;

 /*
  * If passed filename is "-", reset standard input and read from that.
  * Otherwise, attempt to open the file, and return 1 on failure.
  */
 if (!strcmp(filename, "-"))
 {
  isstdin=1;
  rewind(stdin);
  file=stdin;
 }
 else
 {
  file=fopen(filename, "rt");
  if (!file)
  {
   xperror(filename);
   return 1;
  }
 }

 /*
  * Main loop.
  * Based on the first character, write the end of the previous line.
  */
 while (1)
 {
  e=fgets(buf, BUFSZ-1, file);
  if (!e) break;

  buf[strlen(buf)-1]=0; /* remove EOL */

  switch (*buf)
  {
   case '+':
    squelch=1;
    putchar('\r');
    break;
   case '1':
    squelch=1;
    putchar('\n');
    putchar('\f'); /* form feed */
    break;
   case '0':
    putchar('\n');
    break;
  }

  /*
   * If we are squelching the newline, reset the flag.
   * Otherwise, add the newline.
   */
  if (squelch)
   squelch=0;
  else
   putchar('\n');

  /*
   * Write line (except EOL).
   * Detect error condition, die screaming if found.
   */
  if (fputs(&(buf[1]), stdout)<0)
  {
   if (!isstdin) fclose(file);
   xperror(filename);
   return -1;
  }
 }

 /* Final newline; then close file (if not stdin) and return success. */
 putchar('\n');
 if (!isstdin) fclose(file);
 return 0;
}

int main (int argc, char **argv)
{
 int e, r, t;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /* Reset error flag. */
 r=0;

 /* No arguments.  Operate on stdin. */
 if (argc==1)
  return do_asa("-");

 /*
  * Iterate over arguments.  If an error happens, set the flag.
  */
 for (t=1; t<argc; t++)
 {
  e=do_asa(argv[t]);
  if (!r) r=e;
 }

 /* Return whether error flag is set. */
 return r;
}
