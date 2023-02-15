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
 * There is an assumption here that the default handles are 0=stdin, 1=stdout,
 * 2=stderr, which is true on any sane *x but might not be true on some
 * particularly braindamaged OSes.  It's also true on MS-DOS, but that's
 * beside the point.
 * 
 * Also sprach Posix: return value is 126 for exec fail, 127 for it didn't get
 * that far in the first place.
 */

#define _XOPEN_SOURCE 700
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __SVR4__
#define FILE_MODE 0600 /* Same thing, just dumber */
#else
#define FILE_MODE (S_IRUSR|S_IWUSR)
#endif

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

int main (int argc, char **argv)
{
 char our_file[PATH_MAX+1];

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 
 if (argc==1)
 {
  fprintf (stderr, "%s: usage: %s command\n", progname, progname);
  return 127;
 }
 
 /*
  * Is stdout redirected? 
  * BSD runs SIGQUIT into the ground.  Also sprach Posix: there's no need to
  * do this, and in fact, if we really want to conform to the letter of the
  * law here, don't bother with that.
  */
 if (isatty(1))
 {
  int handle;
  
  /*
   * Try to open "nohup.out" in the current folder.
   * Also sprach Posix: file should be opened append and with the exact file
   * mode described above.
   * 
   * We need to know the filename we're writing to for later.
   */
  strcpy(our_file, "nohup.out");
  handle=open(our_file, O_RDWR|O_CREAT|O_APPEND, FILE_MODE);
  
  /*
   * Didn't open?  Try $HOME/nohup.out instead.
   * If $HOME is so long that this buffer would overflow, you got much bigger
   * issues.
   */
  if (handle<0)
  {
   char *p;

   memset(our_file, 0, PATH_MAX+1);
   p=getenv("HOME");
   if (!p)
   {
    xperror ("failed to get home directory");
    return 127;
   }
   strncpy(our_file, p, PATH_MAX-10);
   strcat(our_file, "/nohup.out");
   handle=open(our_file, O_RDWR|O_CREAT|O_APPEND, FILE_MODE);
   
   /* Still nerp?  Die screaming. */
   if (handle<0)
   {
    xperror(our_file);
    return 127;
   }
  }
  
  /* Point stdout to our handle. */
  if (dup2(handle, 1)<0)
  {
   xperror ("dup2() fail on stdout");
   return 127;
  }
  fprintf (stderr, "%s: stdout will be appended to %s\n", progname, our_file);
 }
 
 /* Run SIGHUP into the ground. */
 signal(SIGHUP, SIG_IGN);

 /*
  * Is stderr redirected?  If not, point it to stdout.
  * 
  * Idea from BSD: we've shat on stderr, we might at least be able to write to
  * stdin still.
  */
 if (isatty(2))
 {
  if (dup2(1, 2)<0)
  {
   fprintf (stdin, "%s: %s\n", progname, strerror(errno));
   return 127;
  }
 }
 
 /* Chain into our program, or eat screaming death. */
 argv++;
 execvp(*argv, argv);
 fprintf (stderr, "%s: failed to exec %s: %s\n", progname, *argv, strerror(errno));
 return 126;
}
