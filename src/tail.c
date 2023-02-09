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

#ifdef __SVR4__
typedef signed long ssize_t;
#endif

#define MODE_BLOCK 0x01
#define MODE_CHARS 0x02
#define MODE_LINES 0x04
int mode;
int retry;

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

ssize_t off;

void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

int do_tail (char *filename, int noforever)
{
 int c, e;
 FILE *file;
 char *slurp;
 long lins, *linptrs;
 size_t l, t, lt, sp;

 if (!strcmp(filename, "-"))
 {
  char foo[20];
  int h;

  /* 20 */
  strcpy(foo, "/var/tmp/tailXXXXXX");

#ifdef __SVR4__ /* DO NOT DO THIS, MAGGOT! */
  file=fopen(mktemp(foo), "w");
#else
  h=mkstemp(foo);
  if (h<0)
  {
   fprintf (stderr, "%s: could not open holding tank for stdin\n", progname);
   exit(1);
  }
  file=fdopen(h, "w");
#endif
  rewind(stdin);
  while ((c=getchar())>=0) fputc(c, file);
  fclose(file);
  e=do_tail(foo, 0);
  unlink(foo);
  if (retry)
  {
   while (1)
   {
    rewind(stdin);
    while (0<=(c=getchar())) putchar(c);
   }
  }
  return e;
 }

 /* Slurp file */
 file=fopen(filename, "r");
 if (!file)
 {
  xperror(filename);
  return 1;
 }
 fseek(file, 0, SEEK_END);
 l=ftell(file);
 fseek(file, 0, SEEK_SET);
 slurp=malloc(l);
 if (!slurp) scram();
 if (fread(slurp, 1, l, file)<l)
 {
  free(slurp);
  perror(filename);
  return 1;
 }

 /* Get line numbers */
 lins=0;
 for (t=0; t<l; t++)
  if (slurp[t]=='\n') lins++;
 /* last char not EOL */
 if (slurp[l-1]!='\n') lins++;
 linptrs=calloc(sizeof(long), lins+1);
 if (!linptrs) scram();
 linptrs[0]=0;
 lt=0;
 for (t=0; t<l; t++)
  if (slurp[t]=='\n') linptrs[++lt]=t+1;

 if (mode==MODE_BLOCK)
 {
  if (off<0)
  {
   sp=(l>>9)<<9;
   sp+=(off<<9);
  }
  else sp=(off<<9);
  if (sp<0) sp=0;
 }
 else if (mode==MODE_CHARS)
 {
  if (off<0)
   sp=(l+off);
  else
   sp=off;
 }
 else if (mode==MODE_LINES)
 {
  if (!off) off++;

  if (off<(-lins)) off=0;

  if (off<0)
   sp=linptrs[lins+off];
  else
   sp=linptrs[off-1];
 }
 if (sp<0) sp=0;
 if (sp>l) sp=l;

 e=0;
 if (fwrite(&(slurp[sp]), 1, l-sp, stdout)<(l-sp))
 {
  e=1;
  xperror(filename);
 }

 free(linptrs);
 free(slurp);
 if ((!noforever)&&retry)
 {
  while (1)
  {
   clearerr(file);
   while (0<=(c=fgetc(file))) putchar(c);
  }
 }
 fclose(file);
 return e;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-f] [-bcn offset] [file ...]\n",
          progname, progname);
 exit(1);
}

long getoffset(char *s)
{
 char *p;
 long t;
 int dir;

 dir=-1;

 p=s;
 if (*p=='+')
 {
  p++;
  dir=1;
 }
 else if (*p=='-')
 {
  p++;
 }

 errno=0;
 t=strtol(p, 0, 0);
 if (errno)
 {
  fprintf (stderr, "%s: bogus line count: '%s'\n", progname, s);
  exit(1);
 }

 if (dir==-1) t=-t;
 return t;
}

int main (int argc, char **argv)
{
 int e, r, t;
 int gotchas;

 gotchas=mode=retry=0;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /* Old BSD switch format */
 if (argc>1)
 {
  if (argv[1][0]=='-')
  {
   if ((argv[1][1]>='0')&&(argv[1][1]<='9'))
   {
    errno=0;

    off=strtol(&(argv[1][1]), 0, 0);
    if (errno)
    {
     fprintf (stderr, "%s: bogus line count: '%s'\n",
              progname, &(argv[1][1]));
     return 1;
    }
    off=-off;
    mode=MODE_LINES;
    gotchas++;
    argv++;
    argc--;
   }
  }
 }

 while (-1!=(e=getopt(argc, argv, "fb:c:n:")))
 {
  switch (e)
  {
   case 'b':
    gotchas++;
    mode=MODE_BLOCK;
    off=getoffset(optarg);
    break;
   case 'c':
    gotchas++;
    mode=MODE_CHARS;
    off=getoffset(optarg);
    break;
   case 'n':
    gotchas++;
    mode=MODE_LINES;
    off=getoffset(optarg);
    break;
   case 'f':
    retry=1;
    break;
   default:
    usage();
  }
 }

 if (!gotchas)
 {
  mode=MODE_LINES;
  off=-10;
 }
 else if (gotchas>1)
 {
  fprintf (stderr, "%s: too many offsets supplied\n", progname);
  return 1;
 }

 if (argc==optind) return do_tail("-", 0);

 r=0;
 for (t=optind; t<argc; t++)
 {
  e=do_tail(argv[t], 0);
  if (r<e) r=e;
 }

 return r;
}
