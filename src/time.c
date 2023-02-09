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

#include <sys/times.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#define TIMEXMAT "real %f\nuser %f\nsys %f\n"

static char *copyright="@(#) (C) Copyright 2022, 2023 S. V. Nickolas\n";

static char *progname;

#ifdef __SVR4__
/* strsignal() is something else, emulate Posix semantics here */
char *strsignal (int sig)
{
 if ((sig<0)||(sig>=_sys_nsig)) return 0;
 return _sys_siglist[sig];
}
#endif

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-p] program\n", progname, progname);
 exit(125);
}

int main (int argc, char **argv)
{
 int e, firstarg;
 long tps;
 pid_t pid;
 clock_t clk1, clk2;
 struct tms tms1, tms2;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 firstarg=1;

 if (argc==1)
  usage();

 if (argv[1][0]=='-')
 {
  if (argv[1][1]!='p') usage();
  if (argv[1][2]) usage();
  firstarg=2;
 }

 tps=sysconf(_SC_CLK_TCK);

 clk1=times(&tms1);
 switch (pid=fork())
 {
  case -1:
   fprintf (stderr, "%s: cannot fork: %s\n", progname, strerror(errno));
   return 125;
  case 0:
   execvp(argv[firstarg], argv+firstarg);
   fprintf (stderr, "%s: failed to exec %s: %s\n", progname,
            argv[firstarg], strerror(errno));
   if (errno==ENOENT) return 126;
   return 127;
  default:
   while (1)
   {
    wait(&e);
    if (WIFEXITED(e))
    {
     clk2=times(&tms2);

     printf (TIMEXMAT, ((double)(clk2-clk1))/tps,
                       ((double)(tms2.tms_cutime-tms1.tms_cutime))/tps,
                       ((double)(tms2.tms_cstime-tms1.tms_cstime))/tps);

     return WEXITSTATUS(e);
    }
    if (WIFSIGNALED(e))
    {
     char *x;

     x=strsignal(WTERMSIG(e));
     if (x)
      fprintf (stderr, "%s: %s: %s", progname, argv[firstarg], x);
     else
      fprintf (stderr, "%s: %s: killed by signal %u",
               progname, argv[firstarg], WTERMSIG(e));
     if (WCOREDUMP(e))
      fprintf (stderr, " (core dumped)");
     fprintf(stderr, "\n");

     clk2=times(&tms2);
     printf (TIMEXMAT, ((double)(clk2-clk1))/tps,
                       ((double)(tms2.tms_cutime))/tps,
                       ((double)(tms2.tms_cstime))/tps);

     return 124;
    }
   }
 }
}
