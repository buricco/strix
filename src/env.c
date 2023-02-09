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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

extern char **environ;

/*
 * Linux provides a clearenv(3), but if we use it, env(1) will segvee.
 * Instead, provide a more "proper" clearenv(3) that will not segvee env(1).
 */
char	*cleanenv[1];

/* Actual return is unused. */
int clearenv (void)
{
 environ=cleanenv;
 cleanenv[0]=NULL;
 return 0;
}

static char *progname;

void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

/*
 * Use setenv() with its funkier semantics if we have it, but fall back to the
 * simpler putenv() on SVR4.  See below.
 */
#ifndef __SVR4__
int cpysetenv (char *t)
{
 char *p,*q;
 int e;

 if (!strchr(t,'=')) return -1;
 p=malloc(strlen(t)+1);
 if (!p)
 {
  fprintf (stderr, "%s: out of memory\n", progname);
  return -1;
 }
 strcpy(p,t);
 q=strchr(p,'=');
 *(q++)=0;
 e=setenv(p,q,1);
 free(p);
 if (e) xperror("setenv()");
 return e;
}
#endif

static void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-i] [tag=content ...] [command args ...]\n",
          progname, progname);
 exit (1);
}

int do_printenv (int argc, char **argv)
{
 char *foo;
 int i, e;

 if (argc==1)
 {
  for (i=0; environ[i]; i++) printf ("%s\n", environ[i]);
  return 0;
 }

 if (argv[1][0]=='-')
 {
  fprintf (stderr, "%s: usage: %s [variable ...]\n", progname, progname);
  return 1;
 }

 if (argc==2)
 {
  foo=getenv(argv[1]);
  if (!foo) return 1;
  printf ("%s\n", foo);
  return 0;
 }

 e=0;

 for (i=1; i<argc; i++)
 {
  foo=getenv(argv[i]);
  if (!foo)
   e=1;
  else
   printf ("%s=%s\n", argv[i], foo);
 }
 return e;
}

int do_env (int argc, char **argv)
{
 int e,i;

 i=0;
 e=1;
 if (argc>1)
 {
  if ((!strcmp(argv[1], "-i"))||(!strcmp(argv[1],"-")))
  {
   i=1;
   e++;
  }
  else if (*(argv[1])=='-')
  {
   fprintf (stderr, "%s: invalid switch: '%s'\n", progname, argv[1]);
   usage();
  }
 }

 if (i)
 {
  if (clearenv())
  {
   xperror("clearenv()");
   return 1;
  }
 }

 for (; e<argc; e++)
 {
  if (!strchr(argv[e],'=')) break;
#ifdef __SVR4__ /* see above */
  if (putenv(argv[e]))
#else
  if (cpysetenv(argv[e]))
#endif
   return 1;
 }

 if (e<argc)
 {
  i=execvp(argv[e], argv+e);
  xperror(argv[e]);
  return (errno==ENOENT)?127:126;
 }

 for (i=0; environ[i]; i++) printf ("%s\n", environ[i]);
 return 0;
}

int main (int argc, char **argv)
{
 progname=argv[0];

 if (strrchr(progname, '/'))
  progname=strrchr(progname, '/')+1;

 if (!strcmp(progname, "printenv"))
  return do_printenv(argc, argv);
 else
  return do_env(argc, argv);
}
