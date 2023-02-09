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
#include <unistd.h>

#ifdef __MINT__
#define off_t size_t
#endif

static char *progname;

#define MODE_L 0x01
#define MODE_S 0x02
int mode;

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

void usage (void)
{
 fprintf (stderr,
          "%s: usage: %s [-ls] filename1 filename2 [offset1 [offset2]]\n",
          progname, progname);
 exit(2);
}

int main (int argc, char **argv)
{
 int e;
 FILE *src, *tgt;
 size_t skip1, skip2;

 skip1=skip2=0;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 mode=0;
 while (-1!=(e=getopt(argc, argv, "ls")))
 {
  switch (e)
  {
   case 'l':
    mode|=MODE_L;
    break;
   case 's':
    mode|=MODE_S;
    break;
   default:
    usage();
  }
 }

 /*
  * Posix leaves it up to the implementor what to do when both -ls are
  * specified, though it declares "die screaming" to be a bug.  I do not see
  * it as a bug, but as the logical behavior.
  */
 if ((mode&(MODE_L|MODE_S))==(MODE_L|MODE_S))
 {
  fprintf (stderr, "%s: -l and -s are mutually exclusive\n", progname);
  return 1;
 }

 /* Argument count check. */
 e=argc-optind;
 if ((e<2)||(e>4)) usage();

 /* Solaris & 4.4BSD extension. */
 if (e>2)
 {
  errno=0;
#ifdef __SVR4__
  skip1=strtol(argv[optind+2], 0, 0);
#else
  skip1=strtoll(argv[optind+2], 0, 0);
#endif
  if (errno)
  {
   fprintf (stderr, "%s: bogus offset: '%s'\n", progname, argv[optind+2]);
   return 2;
  }
  if (e==4)
  {
   errno=0;
#ifdef __SVR4__
   skip2=strtol(argv[optind+3], 0, 0);
#else
   skip2=strtoll(argv[optind+3], 0, 0);
#endif
   if (errno)
   {
    fprintf (stderr, "%s: bogus offset: '%s'\n", progname, argv[optind+3]);
    return 2;
   }
  }
 }

 if ((!strcmp(argv[optind], "-"))&&(!strcmp(argv[optind+1], "-")))
 {
  fprintf (stderr, "%s: only one file can be stdin\n", progname);
  return 2;
 }

 /* Open our files, or die trying. */
 if (strcmp(argv[optind], "-"))
 {
  src=fopen(argv[optind], "rb");
  if (!src)
  {
   xperror(argv[optind]);
   return 2;
  }
 }
 else
 {
  rewind(stdin);
  src=stdin;
 }

 if (strcmp(argv[optind+1], "-"))
 {
  tgt=fopen(argv[optind+1], "rb");
  if (!tgt)
  {
   if (src!=stdin) fclose(src);
   xperror(argv[optind+1]);
   return 2;
  }
 }
 else
 {
  rewind(stdin);
  tgt=stdin;
 }

 if (fseek(src, skip1, SEEK_SET))
 {
  xperror(argv[optind]);
  return 2;
 }
 if (fseek(tgt, skip2, SEEK_SET))
 {
  xperror(argv[optind+1]);
  return 2;
 }

 if (mode==MODE_L) /* List errors */
 {
  int c1, c2;
  off_t o;
  e=0;
  o=0;
  while (1)
  {
   o++;
   c1=fgetc(src);
   c2=fgetc(tgt);
   if ((c1<0)&&(c2<0)) /* Got to the end of both files */
   {
    if (feof(src)&&feof(tgt))
    {
     if (src!=stdin) fclose(src);
     if (tgt!=stdin) fclose(tgt);
     return e;
    }
    if (!feof(src))
     xperror(argv[optind]);
    if (!feof(tgt))
     xperror(argv[optind+1]);
    return 2;
   }
   if (c1<0)
   {
    if (!feof(src))
    {
     xperror(argv[optind]);
     return 2;
    }
    else
    {
     fprintf (stderr, "%s: EOF on %s\n", progname, argv[optind]);
     return 1;
    }
   }
   if (c2<0)
   {
    if (!feof(tgt))
    {
     xperror(argv[optind+1]);
     return 2;
    }
    else
    {
     fprintf (stderr, "%s: EOF on %s\n", progname, argv[optind+1]);
     return 1;
    }
   }
   if (c1!=c2)
   {
    e=1; /* Mark files as mismatched */
#ifdef __SVR4__
    printf ("%lu %o %o\n", o, c1, c2);
#else
    printf ("%llu %o %o\n", (long long) o, c1, c2);
#endif
   }
  }
 }
 else /* Find one match */
 {
  int c1, c2;
  off_t o;
#ifdef __SVR4__
  long l;
#else
  long long l;
#endif

  o=0;
  l=1;
  while (1)
  {
   o++;
   c1=fgetc(src);
   c2=fgetc(tgt);
   if ((c1<0)&&(c2<0)) /* Got to the end of both files, no differences */
   {
    if (feof(src)&&feof(tgt))
    {
     if (src!=stdin) fclose(src);
     if (tgt!=stdin) fclose(tgt);
     return 0;
    }
    if (!(mode&MODE_S))
    {
     if (!feof(src))
      xperror(argv[optind]);
     if (!feof(tgt))
      xperror(argv[optind+1]);
    }
    return 2;
   }
   if (c1<0)
   {
    if (feof(src))
    {
     if (!(mode&MODE_S)) fprintf (stderr, "%s: EOF on %s\n", argv[0], argv[optind]);
     if (src!=stdin) fclose(src);
     if (tgt!=stdin) fclose(tgt);
     return 1;
    }
    if (!(mode&MODE_S)) xperror(argv[optind]);
    return 2;
   }
   if (c2<0)
   {
    if (feof(tgt))
    {
     if (!(mode&MODE_S)) fprintf (stderr, "%s: EOF on %s\n", argv[0], argv[optind+1]);
     if (src!=stdin) fclose(src);
     if (tgt!=stdin) fclose(tgt);
     return 1;
    }
    if (!(mode&MODE_S)) xperror(argv[optind+1]);
    return 2;
   }
   if (c1=='\n') l++;
   if (c1!=c2)
   {
    if (!(mode&MODE_S))
#ifdef __SVR4__
     printf ("%s %s differ: char %lu, line %lu\n",
#else
     printf ("%s %s differ: char %llu, line %llu\n",
#endif
             argv[optind], argv[optind+1],
#ifndef __SVR4__ /* placate clang */
             (long long)
#endif
             o, l);
    if (src!=stdin) fclose(src);
    if (tgt!=stdin) fclose(tgt);
    return 1;
   }
  }
 }
}
