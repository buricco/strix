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
 * -l  Number of lines per file (default 1000)
 * -a  Length of file suffix (default 2).
 *     If all potential letters (a-z) are exhausted for all bytes (right to
 *     left), die screaming.
 * -b  Number of bytes (*1024 for k, *1048576 for m) per file.
 * -f  Fill the last file (extension); this is useful for creating split disk
 *     images.  Only with -b.  Undocumented.
 * 
 * Default output base is "x".
 * 
 * The 1024-base is something I agree with Posix on.
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MODE_B 0x01
#define MODE_L 0x02 /* for error checking */
#define MODE_F 0x04
int mode;
unsigned long units;
int suflen;
char *base="x";
char *fnbuf;

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

void mktail (int val)
{
 char *p;
 int at;
 int q, r;
 
 memset(fnbuf, 0, strlen(base)+suflen+1);
 strcpy(fnbuf, base);
 p=fnbuf+strlen(base);
 at=suflen;
 q=val;
 
 for (r=0; r<suflen; r++) p[r]='a';
 
 while (q)
 {
  r=q%26;
  p[at-1]='a'+r;
  q/=26;
  at--;
  if (at<0)
  {
   fprintf (stderr, "%s: out of letters (%d)\n", progname, val);
   exit(1);
  }
 }
 
}

int do_split (char *filename)
{
 FILE *in, *out;
 int isstdin;
 int n;
 unsigned long l;
 char *b;
 
 if ((strlen(base)+suflen)>NAME_MAX)
 {
  fprintf (stderr, "%s: %s: generated filenames would be too long\n",
           progname, base);
  exit(1);
 }
 
 fnbuf=malloc(strlen(base)+suflen+1);
 if (!fnbuf) scram();
 
 if (mode&MODE_B)
 {
  b=malloc(units);
  if (!b) scram();
 }
 else
  b=0;
 
 if (strcmp(filename, "-"))
 {
  isstdin=0;
  in=fopen(filename, (mode&MODE_B)?"rb":"rt");
  if (!in)
  {
   xperror(filename);
   exit(1);
  }
  
  /*
   * Since we can know the filesize:
   *   A size of 0 means do nothing and return OK (also sprach Posix).
   */
  fseek(in, 0, SEEK_END);
  if (!ftell(in))
  {
   fclose(in);
   return 0;
  }
  fseek(in, 0, SEEK_SET);
 }
 else
 {
  in=stdin;
  rewind(in);
  isstdin=1;
 }
 
 out=0;
 n=0;

 while (1)
 {
  if (!out)
  {
   mktail(n);
   out=fopen(fnbuf, (mode&MODE_B)?"wb":"wt");
   if (!out)
   {
    xperror(fnbuf);
    exit(1);
   }
   n++;
  }
  
  if (mode&MODE_B)
  {
   size_t e;
   
   memset(b, 0, units);
   
   e=fread(b, 1, units, in);
   if (e!=units)
   {
    if (feof(in))
    {
     if (isstdin) rewind(stdin); else fclose(in);
     if (e>0) /* short read, still OK */
     {
      size_t f;
      
      f=e;
      if (mode&MODE_F) f=units;
      e=fwrite(b, 1, f, out);
      fclose(out);
      if (e!=f)
      {
       xperror(fnbuf);
       exit(1);
      }
     }
     break;
    }
    xperror(filename);
    exit(1);
   }
   
   e=fwrite(b, 1, units, out);
   if (e!=units)
   {
    if (isstdin) rewind(stdin); else fclose(in);
    fclose(out);
    xperror(fnbuf);
    exit(1);
   }
   fclose(out);
   out=0;
  }
  else
  {
   int e;
   size_t x;
   
   if (b) free(b);
   
   for (l=0; l<units; l++)
   {
    e=getline(&b, &x, in);
    if (e==-1)
    {
     if (isstdin) rewind(stdin); else fclose(in);
     fclose(out);
     if (feof(in))
      return 0;
     xperror(filename);
     exit(1);
    }
    fprintf (out, "%s", b);
   }
   fclose(out);
   out=0;
  }
 }
 
 if (mode&MODE_B) free(b);
 return 0;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-l lines] [-a suffixlen] [file [prefix]]\n"
          "%s: usage: %s -b [size[k | m] [-a suffixlen] [file [prefix]]\n",
          progname, progname, progname, progname);
 exit(1);
}

unsigned long checknum (char *arg)
{
 long r;
 
 errno=0;
 if (!arg) return 0;
 r=strtoul(arg, 0, 0);
 if (errno)
 {
  fprintf (stderr, "%s: bogus numeric value: '%s'\n", progname, arg);
  exit(1);
 }
 if (r<=0)
 {
  fprintf (stderr, "%s: value must be positive: '%s'\n", progname, arg);
  exit(1);
 }
 return r;
}

int main (int argc, char **argv)
{
 int e;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 mode=0;
 units=1000;
 suflen=2;
 while (-1!=(e=getopt(argc, argv, "b:l:a:f")))
 {
  switch (e)
  {
   case 'b':
    mode |= MODE_B;
    if (optarg[strlen(optarg)-1]=='k')
    {
     optarg[strlen(optarg)-1]=0;
     units=checknum(optarg)*1024UL;
    }
    else if (optarg[strlen(optarg)-1]=='m')
    {
     optarg[strlen(optarg)-1]=0;
     units=checknum(optarg)*1048576UL;
    }
    else
     units=checknum(optarg);
    break;
   case 'l':
    mode |= MODE_L;
    units=checknum(optarg);
    break;
   case 'a':
    suflen=checknum(optarg);
    break;
   case 'f':
    mode |= MODE_F;
    break;
   default:
    usage();
  }
 }
 
 /* -f without -b: play dumb */
 if ((mode&(MODE_B|MODE_F))==MODE_F) usage();
 
 if ((mode&(MODE_B|MODE_L))==(MODE_B|MODE_L))
 {
  fprintf (stderr, "%s: -b and -l are mutually exclusive\n", progname);
  return 1;
 }
 
 if ((argc-optind)>2) usage();
 
 if ((argc-optind)==2) base=argv[argc-1];
 if (argc==optind)
  return do_split("-");
 return do_split(argv[optind]);
}
