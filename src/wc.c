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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef __SVR4__
#include <wchar.h>
#include <wctype.h>
#endif

#define NO_ARGS 0x10
#define MODE_K 0x08
#define MODE_L 0x04
#define MODE_W 0x02
#define MODE_C 0x01
int mode;

static char *copyright="@(#) (C) Copyright 2022, 2023 S. V. Nickolas\n";

static char *progname;

/*
 * Running total for ALL files.
 *
 * This is probably overkill.
 * If your compiler is too primitive for "long long", change it to "long", and
 * every "%llu" to "%lu".
 */
#ifndef __SVR4__
long
#endif
long L, W, C;

/* prerror(3) with the name of the utility AND the name of the input file. */
void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

/* Perform actions on one file. */
int do_wc (char *filename)
{
 FILE *file;
 int isstdin;
#ifndef __SVR4__
 long
#endif
 long l, w, c;
 int inword;

 isstdin=0;
 l=w=c=0;

 /*
  * Treat a filename of "-" as referring to the standard input, and reset it.
  * Otherwise, open the file. (Opening as text doesn't change anything except
  * on descendants of CP/M, you know who you are.)
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
   return -1;
  }
 }

 /* Reset "in word" flag, and then start reading. */
 inword=0;
 while (1)
 {
  int cs;
#ifndef __SVR4__ /* we don't have this feature in our libc */
  wint_t cl;

  /*
   * -k (account for wide chars) mode, as in OSF and UnixWare.
   * This will be followed by normal 8-bit mode, which uses identical logic.
   */
  if (mode&MODE_K)
  {
   cl=fgetwc(file);
   if (cl==WEOF) break;
   c++;

   /* End-of-line condition. */
   if (cl==L'\n') l++;

   /*
    * isspace() and iswspace() detect any end-of-word condition.
    * But note that they MAY appear multiple times, and only count the first.
    * If we don't read an end-of-word, then of course we are now in a word.
    */
   if (iswspace(cl))
   {
    if (inword)
    {
     inword=0;
     w++;
    }
   }
   else
   {
    inword=1;
   }
  }
  else
#endif
  {
   cs=fgetc(file);
   if (cs<0) break;
   c++;
   if (cs=='\n') l++;
   if (isspace(cs))
   {
    if (inword)
    {
     inword=0;
     w++;
    }
    else
    {
     inword=1;
    }
   }
  }
 }

 /*
  * Close the file (if not standard input), and display a summary for it.
  * If we are only reading the standard input by default (no files were given
  * on the command line), suppress the display of the filename.
  */
 if (!isstdin) fclose(file);
#ifdef __SVR4__ /* no notion of a long long */
 if (mode&MODE_L) printf ("%lu ", l);
 if (mode&MODE_W) printf ("%lu ", w);
 if (mode&MODE_C) printf ("%lu ", c);
#else
 if (mode&MODE_L) printf ("%llu ", l);
 if (mode&MODE_W) printf ("%llu ", w);
 if (mode&MODE_C) printf ("%llu ", c);
#endif
 if (!(mode&NO_ARGS))
 {
  if (!strcmp(filename, "-"))
   printf ("(stdin)");
  else
   printf ("%s", filename);
 }
 printf ("\n");
 return 0;
}

/* Simple synopsis of command-line syntax; die screaming. */
void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-clmw] [file ...]\n", progname, progname);
 exit(1);
}

/* Entry point. */
int main (int argc, char **argv)
{
 int e, r, t, n;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /* Set defaults. */
 mode=0;

 /*
  * Parse switches.
  *
  * -k and -m are treated the same.  POSIX wants -m, but some versions use -k.
  * If we catch an invalid switch, die screaming.
  * If no switches are used, default to -clw.
  *
  * Note that in this version -c and -m cannot be used together; this may be a
  * misdesign, but -m changes the way the file is read.  However, the -c flag
  * is turned on after parsing, and used to count up characters instead of
  * bytes.
  */
 while (-1!=(e=getopt(argc, argv, "cklmw")))
 {
  switch (e)
  {
   case 'k':
   case 'm':
    mode |= MODE_K;
    break;
   case 'c':
    mode |= MODE_C;
    break;
   case 'l':
    mode |= MODE_L;
    break;
   case 'w':
    mode |= MODE_W;
    break;
   default:
    usage(); /* NO RETURN */
  }
 }
 if (mode&MODE_K)
 {
  if (mode&MODE_C)
  {
   fprintf (stderr, "%s: -c and -m are mutually exclusive\n", progname);
   return 1;
  }

  mode |= MODE_C;
 }
 if (!(mode&(MODE_C|MODE_L|MODE_W))) mode|=(MODE_C|MODE_L|MODE_W);

 /* Reset error flag, file counter and running counters. */
 r=n=0;
 L=W=C=0;

 /* If no files specified, use stdin and suppress the filename. */
 if (argc==optind)
 {
  mode|=NO_ARGS;
  return do_wc("-");
 }

 /*
  * Iterate through all files specified, getting a running total of how many
  * files we have looked through.  If any of them fail, mark it.
  */
 for (t=optind; t<argc; t++)
 {
  n++;
  e=do_wc(argv[t]);
  if (!r) r=e;
 }

 /* If necessary, display a running total. */
 if (n>1)
 {
#ifdef __SVR4__ /* no notion of a long long */
  if (mode&MODE_L) printf ("%lu ", L);
  if (mode&MODE_W) printf ("%lu ", W);
  if (mode&MODE_C) printf ("%lu ", C);
#else
  if (mode&MODE_L) printf ("%llu ", L);
  if (mode&MODE_W) printf ("%llu ", W);
  if (mode&MODE_C) printf ("%llu ", C);
#endif
  printf ("total\n");
 }

 /* Return whether any error conditions occurred. */
 return r;
}
