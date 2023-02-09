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
 * There have been a couple versions of tac(1) before this one.
 * This one is barebones - all it does is flip the order of lines.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

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

int do_tac (char *filename)
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

  /* 19 */
  strcpy(foo, "/var/tmp/tacXXXXXX");

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
  e=do_tac(foo);
  unlink(foo);
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
 {
  if (slurp[t]=='\n')
  {
   slurp[t]=0;
   linptrs[++lt]=t+1;
  }
 }

 while (lt)
 {
  if (puts(&(slurp[linptrs[--lt]]))<0)
  {
   e=1;
   break;
  }
 }

 free(linptrs);
 free(slurp);
 fclose(file);
 return e;
}

int main (int argc, char **argv)
{
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (argc==1)
 {
  return do_tac("-");
 }
 else
 {
  int e, r, t;

  r=0;
  for (t=1; t<argc; t++)
  {
   e=do_tac(argv[t]);
   if (r<e) r=e;
  }
  return e;
 }
}
