/*
 * (C) Copyright 2020, 2023 S. V. Nickolas.
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

static char *copyright="@(#) (C) Copyright 2020, 2023 S. V. Nickolas\n";

static char *progname;

int do_the_needful (char *filename, size_t size, int relative, int nocreate)
{
 int e;
 FILE *file;
 size_t as;
 
 if (relative)
 {  
  file=fopen(filename, "rb");
  if (file)
  {
   fseek(file,0,SEEK_END);
   as=ftell(file);
   fclose(file);
  }
  else
  {
   if (nocreate)
   {
    fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
    return 1;
   }
   
   file=fopen(filename, "w");
   if (!file)
   {
    fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
    return 1;
   }
   fclose(file);
   as=0;
  }
  as+=size;
 }
 else
 {
  as=size;
  file=fopen(filename, "rb");
  if (file)
  {
   fclose(file);
  }
  else
  {
   if (nocreate)
   {
    fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
    return 1;
   }
   file=fopen(filename,"w");
   fclose(file);
  }
 }
 
 e=truncate(filename, as);
 if (e)
  fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
 
 return e;
}

void usage (void)
{
 fprintf (stderr, 
          "%s: usage: %s [-co] [-r filename] [-s size] filename ...\n", 
          progname, progname);
 exit(0);
}

size_t size_by_reference (char *filename)
{
 size_t s;
 FILE *file;
 
 file=fopen(filename,"rb");
 if (!file)
 {
  fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
  exit(1);
 }
 
 fseek(file,0,SEEK_END);
 s=ftell(file);
 fclose(file);
 return s;
}

int main (int argc, char **argv)
{
 int e;
 int c, o, r;
 size_t s;
 int szck, ock, rel;
 char *p;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 
 szck=c=o=ock=r=rel=0;
 
 while (-1!=(e=getopt(argc, argv, "cor:s:")))
 {
  switch (e)
  {
   case 'c':
    c=1;
    break;
   case 'o':
    o=1;
    break;
   case 'r':
    szck++;
    r=1;
    s=size_by_reference(optarg);
    break;
   case 's':
    szck++;
    if (*optarg=='-')
    {
     rel=-1;
     optarg++;
    }
    else if (*optarg=='+')
    {
     rel=1;
     optarg++;
    }
    s=strtoll(optarg, &p, 0);
    if (optarg==p)
    {
     fprintf (stderr, "%s: '%s' does not appear to be a size\n", 
              progname, optarg);
     return 1;
    }
    if (*p)
    {
     ock=1;
     
     /* I am NOT accepting salesman's units. A kilobyte is 1024 bytes. */
     
     if (*p=='K') s<<=10; /* 1024 */
     if (*p=='M') s<<=20; /* 1048576 */
     if (*p=='G') s<<=30; /* 1073741824 */
     if (!p[1]) break;
     if (p[1]=='B')
      if (!p[2]) break;
     if (p[1]=='i')
      if (p[2]=='B')
       if (!p[3]) break;
     
     fprintf (stderr, "%s: '%s' does not appear to be a size multiplicand\n",
              progname, optarg);
     return 1;
    }
    break;
   default:
    usage();
  }
 }
 
 if (!szck)
 {
  fprintf (stderr, "%s: no size specified\n", progname);
  return 1;
 }
 
 if (o)
 {
  if (r)
  {
   fprintf (stderr, "%s: -o and -r are mutually exclusive\n", progname);
   return 1;
  }
  if (ock)
  {
   fprintf (stderr, "%s: -o and metric suffixes are mutually exclusive\n",
            progname);
   return 1;
  }
  s<<=9; /* 512-byte blocks */
 }
 
 if (optind==argc) usage();
 
 r=0;
 
 for (e=optind; e<argc; e++)
  r+=do_the_needful (argv[e], s, rel, c);
 
 return r;
}
