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
 * As originally implemented, this cat(1) copied the System V Release 2
 * version.  With this version, I have replaced the System V "-s" (suppress
 * errors) switch with the BSD "-s" (squeeze newlines).
 *
 * Additionally although I did not implement the line numbering switches -b
 * and -n (we have nl(1) for that) I implemented the switches which were taken
 * from BSD into System V Release 4 (-etv), with their documented behavior
 * from that version.
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef __SVR4__ /* braindead libc */
typedef signed long ssize_t;
#endif

int eflag, sflag, tflag, uflag, vflag;

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

static char *progname;

void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

static void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-estuv] [filename...]\n", progname, progname);
}

int cat (char *filename)
{
 FILE *in;
 int is_stdin;
 int ret;
 int last;

 /*
  * Reset exit code and last character buffer.
  * If we are reading stdin, set the appropriate flag.
  * Otherwise, fail out.
  */
 ret=0;
 is_stdin=0;
 last=-1;
 if (!strcmp(filename,"-"))
 {
  is_stdin=1;
  in=stdin;
  rewind(in);
 }
 else
 {
  in=fopen(filename,"r");
  if (!in)
  {
   xperror(filename);
   return 1;
  }
 }

 /* -u - disable I/O buffering. */
 if (uflag)
 {
  setvbuf(in,0,_IONBF,0);
  setvbuf(stdout,0,_IONBF,0);
 }

 /* File I/O loop. */
 while (1)
 {
  int c,m;

  /* Get a char.  Has to be an int to detect error conditions. */
  c=fgetc(in);

  /* Error condition or EOF? */
  if (c<0)
  {
   if (ferror(in)) ret++;
   break;
  }

  /* -s - squeeze multiple newlines. */
  if ((c=='\n')&&(last=='\n')&&sflag) continue;

  last=c;

  /* -t - display caret syntax for tab and form feed. */
  if (tflag)
  {
   if (c=='\t') {fputs("^I",stdout); continue;}
   if (c=='\f') {fputs("^L",stdout); continue;}
  }

  /*
   * -v - display caret syntax for control characters and DEL and
   *      meta syntax for high bit on.
   */
  if (vflag)
  {
   m=0;
   if (c&0x80)
   {
    putchar('M');
    putchar('-');
    c&=0x7F;
    m=1;
   }
   if (c==0x7F) {fputs("^?",stdout); continue;}
   if ((c<0x20)&&(c!='\n'))
   {
    if (((c!='\t')&&(c!='\f'))||(m)) {putchar('^'); c|='@';}
   }
  }

  /* -e - display $ at end of line. */
  if (eflag&&(c=='\n')) putchar('$');

  /*
   * Write character; detect error condition and mark accordingly.
   */
  if (putchar(c)<0)
  {
   xperror(filename);
   ret=1;
   break;
  }
 }

 /* Close file, if not stdin. */
 if (!is_stdin) fclose(in);

 /* Return error flag. */
 return ret;
}

int main (int argc, char **argv)
{
 int e, t;
#ifdef __SVR4__ /* braindead libc */
 extern int optind;
#endif

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /* Set defaults and evaluate switches. */
 eflag=sflag=tflag=uflag=vflag=0;
 while (-1!=(e=getopt(argc, argv, "estuv")))
 {
  switch (e)
  {
   case 'e':
    eflag=1;
    break;
   case 's':
    sflag=1;
    break;
   case 't':
    tflag=1;
    break;
   case 'u':
    uflag=1;
    break;
   case 'v':
    vflag=1;
    break;
   default:
    usage();
    return 1;
  }
 }

 if (!vflag)
  eflag=tflag=0;

 e=0;

 if (argc==optind)
  e=cat("-");
 else
  for (t=optind; t<argc; t++) e+=cat(argv[t]);

 return e?1:0;
}
