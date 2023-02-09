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
 * -b  strict byte count
 *     otherwise parse tabs and backspaces.
 * -s  break on word boundary (space or tab) if possible
 * -w  width (default 80, also sprach Posix)
 * 
 * WARNING: if backspaces or tabs are used, -s might not always Do The Right
 *          Thing...
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

#define MODE_B 0x01
#define MODE_S 0x02
int mode;
int width;

char buf[BUFSIZ];

void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

int do_fold (char *filename)
{
 char *p;
 FILE *file;
 int r, isstdin;
 
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
 
 p=0;
 r=0;
 
 while (1)
 {
  size_t x;
  int b, c, e, l;
  int o, t, u;
  
  if (p) {free(p); p=0;}
  e=getline(&p, &x, file);
  if (e==-1)
  {
   if (feof(file)) break;
   xperror(filename);
   r=1;
   break;
  }
  
  p[strlen(p)-1]=0; /* strip newline */
  c=l=0;
  for (t=0; p[t]; t++)
  {
   if (c>=width)
   {
    if (mode&MODE_S)
    {
     for (b=t; b>l; b--)
     {
      if ((p[b]==' ')||(p[b]=='\t'))
      {
       t=b+1;
       break;
      }
     }
    }
    for (u=l; u<t; u++) putchar(p[u]);
    putchar('\n'); 
    l=t;
    c=0;
   }
   if (!(mode&MODE_B))
   {
    if (p[t]=='\b')
    {
     if (c) c--;
     continue;
    }
    if (p[t]=='\t')
    {
     while (c%8) c++;
     continue;
    }
    if (p[t]=='\r')
    {
     c=0;
     continue;
    }
   }
   c++;
  }
  puts(&(p[l]));
 }
 
 if (isstdin) rewind(file); else fclose(file);
 return r;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-bs] [-w width] [filename ...]\n",
          progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e, r, t;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 mode=0;
 width=80;
 
 while (-1!=(e=getopt(argc, argv, "bsw:")))
 {
  switch (e)
  {
   case 'b':
    mode|=MODE_B;
    break;
   case 's':
    mode|=MODE_S;
    break;
   case 'w':
    errno=0;
    width=strtol(optarg, 0, 0);
    if (errno||(width<1))
    {
     fprintf (stderr, "%s: bogus width '%s'\n", progname, optarg);
     return 1;
    }
    break;
   default:
    usage();
  }
 }
 
 if (argc==optind)
  return do_fold("-");
 else
 {
  r=0;
  
  for (t=optind; t<argc; t++)
  {
   e=do_fold(argv[optind]);
   if (e>r) r=e;
  }
 }
 
 return r;
}
