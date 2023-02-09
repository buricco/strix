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

/*
 * comm(1) ASSUMES that the file is already sorted.  Its behavior if not will
 * be erratic.  GNU comm will emit a diagnostic at the end; BSD comm will not,
 * nor will SVR4 comm.  (Posix does not specify a "correct" behavior in this
 * situation.  Chaos is as good as any...)
 *
 * We support the case-insensitive option in the BSDs, but do not document it.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __SVR4__
#include <getline.h>
#endif

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

                     /****************************************************/
#define MODE_1 0x01  /* Suppress printing lines unique to file1          */
#define MODE_2 0x02  /* Suppress printing lines unique to file2          */
#define MODE_3 0x04  /* Suppress printing common lines                   */
#define MODE_I 0x08  /* Smash case (-i in FreeBSD; -f in OpenBSD/NetBSD) */
int mode;            /****************************************************/

#ifdef __SVR4__
#include <ctype.h>

int strcasecmp (const char *a, const char *b)
{
 char *aa, *bb;

 aa=(char *)a; bb=(char *)b;
 while (toupper(*aa++)==toupper(*bb++))
  if (!*aa) return 0; /* Match. */
 bb--;
 return *aa-*bb;
}
#endif

void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

void leftonly (char *x)
{
 if (mode&MODE_1) return;
 printf ("%s", x);
}

void rightonly (char *x)
{
 if (mode&MODE_2) return;
 if (!(mode&MODE_1)) putchar('\t');
 printf ("%s", x);
}

void both (char *x)
{
 if (mode&MODE_3) return;
 if (!(mode&MODE_1)) putchar('\t');
 if (!(mode&MODE_2)) putchar('\t');
 printf ("%s", x);
}

int do_comm (char *filename1, char *filename2)
{
 FILE *src, *tgt;
 char *p1, *p2;
 int e1, e2;
 size_t x; /* Discarded */

 /*
  * See below: Only one handle can use the stdin "-" pseudofile.
  * However, nothing is preventing the same device or pipe twice, and if that
  * happens, you will get a right mess.
  *
  * By this point this has already been validated, so we don't need to make
  * sure of that here.
  *
  * Open our files, or die trying.
  */
 if (strcmp(filename1, "-"))
 {
  src=fopen(filename1, "rb");
  if (!src)
  {
   xperror(filename1);
   return 2;
  }
 }
 else
  src=stdin;

 if (strcmp(filename2, "-"))
 {
  tgt=fopen(filename2, "rb");
  if (!tgt)
  {
   fclose(src);
   xperror(filename2);
   return 2;
  }
 }
 else
  tgt=stdin;

 e1=getline(&p1, &x, src);
 e2=getline(&p2, &x, tgt);
 while (1)
 {
  /* src end; dump tgt */
  if (feof(src))
  {
   while (1)
   {
    if (feof(tgt)) break;
    rightonly(p2);
    e2=getline(&p2, &x, tgt);
   }
   break;
  }

  if (feof(tgt))
  {
   /* tgt end; dump src */
   while (1)
   {
    if (feof(src)) break;
    rightonly(p1);
    e1=getline(&p1, &x, tgt);
   }
   break;
  }

  /* p1<p2: print left, read from 1 */
  if (((mode&MODE_I)?strcasecmp:strcmp)(p1, p2)<0)
  {
   leftonly(p1);
   e1=getline(&p1, &x, src);
   continue;
  }

  /* p1>p2: print right, read from 2 */
  if (((mode&MODE_I)?strcasecmp:strcmp)(p1, p2)>0)
  {
   rightonly(p2);
   e2=getline(&p2, &x, tgt);
   continue;
  }

  /* p1==p2: print both, read both */
  both(p1);
  e1=getline(&p1, &x, src);
  e2=getline(&p2, &x, tgt);
 }

 if (src==stdin)
  rewind(stdin);
 else
  fclose(src);

 if (tgt==stdin)
  rewind(stdin);
 else
  fclose(tgt);

 return 0;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-123] file1 file2\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 mode=0;
 while (-1!=(e=getopt(argc, argv, "123fi")))
 {
  switch (e)
  {
   case '1':
    mode |= MODE_1;
    break;
   case '2':
    mode |= MODE_2;
    break;
   case '3':
    mode |= MODE_3;
    break;
   case 'f': /* FALL THROUGH */
   case 'i':
    mode |= MODE_I;
    break;
   default:
    usage();
  }
 }
 if ((argc-optind)!=2) usage();

 /*
  * Posix doesn't define what happens when both files are the same special
  * file or stdin.  Prepare for chaos if you open a device or pipe twice.
  */
 if ((!strcmp(argv[optind], "-"))&&(!strcmp(argv[optind+1], "-")))
 {
  fprintf (stderr, "%s: only one file can be stdin\n", progname);
  return 2;
 }

 return do_comm(argv[optind], argv[optind+1]);
}
