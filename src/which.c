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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

void err_path (void)
{
 fprintf (stderr, "%s: malformed PATH variable\n", progname);
 exit(1);
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-a] command\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e;
 
 char *path, *pathptr;
 char *fnptr;
 int keepgoing;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 
 keepgoing=0;
 while (-1!=(e=getopt(argc, argv, "a")))
 {
  switch (e)
  {
   case 'a':
    keepgoing=1;
    break;
   default:
    usage();
  }
 }
 
 if ((argc-optind)!=1) usage();

 path=getenv("PATH");
 if (!path) path="/bin:/usr/bin";
 
 pathptr=path;
 while (pathptr)
 {
  char *next;
  
  if (!*pathptr) break;
  
  if (*pathptr=='"')
  {
   next=strchr(pathptr+1, '"');
   if (!next)
    err_path();
   *next=0;
   next++;
   if (*next!=':')
    err_path();
   if (next) next++;
   pathptr++;
  }
  else
  {
   next=strchr(pathptr, ':');
   
   if (next)
    *(next++)=0;
  }
  
  e=1;
  fnptr=malloc(strlen(pathptr)+strlen(argv[optind])+2);
  if (!fnptr) scram();
  sprintf (fnptr, "%s/%s", pathptr, argv[optind]);
  if (!access(fnptr, X_OK))
  {
   e=0;
   printf ("%s\n", fnptr);
   if (!keepgoing) return 0;
  }
  free(fnptr);
  
  pathptr=next;
 }
 return e;
}
