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
 * NOTE: -L/-P is ignored on System V.  I'm not sure the semantics for
 *       hardlinking a symlink even exist there.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MODE_F 0x01
#define MODE_S 0x02
#define MODE_L 0x04
int mode;

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

struct stat statbuf;
int isfold;

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

int xmklink (char *source, char *target)
{
 int e, f, l;
 char *basnam;
 char *xtarget;

 /* Get the basename. */
 basnam=strrchr(source, '/');
 if (basnam) basnam++; else basnam=source;

 /*
  * If we're going to place the link into the folder, create a new string with
  * the target path, path separator, and copy of the basename.
  *
  * Note this; we will have to free the string later.
  */
 if (isfold)
 {
  xtarget=malloc(strlen(target)+strlen(basnam)+2);
  if (!xtarget) scram();
  sprintf (xtarget, "%s/%s", target, basnam);
 }
 else
  xtarget=target;

 /*
  * If user wants us to force it, then zot the existing file.
  * (Not if it's a directory!  So use unlink(3), not remove(3).)
  */
 if (mode&MODE_F)
 {
  errno=0;
  e=lstat(xtarget, &statbuf);
  if (errno!=ENOENT)
   unlink(xtarget);
 }

 /*
  * Do it, maggot.
  *
  * POSIX.1:2008 says that link(2) behavior re following symlinks in source is
  * undefined, and provides linkat(2) to define the behavior one way or the
  * other.  That is a lot easier than having to ASS-U-ME it will not follow
  * them and resolving them our damnself, and in fact doesn't require us to
  * ASS-U-ME anything.  It even provides the constant AT_SYMLINK_FOLLOW
  * specifically to implement this behavior.
  *
  * On SVR4, we don't appear to have a way to hardlink a symlink (and frankly,
  * why would you want to do such a braindamaged thing?), so we just ignore
  * -L and -P and just leave it to the runtimes and the kernel to do the
  * needful for us.
  */
 if (mode&MODE_S)
  e=symlink(source, xtarget);
 else
#ifdef __SVR4__
  e=link(source, xtarget);
#else
  e=linkat(AT_FDCWD, source, AT_FDCWD, xtarget,
           (mode&MODE_L)?AT_SYMLINK_FOLLOW:0);
#endif

 if (e) xperror(xtarget);

 /*
  * Common exit point.
  * Free dynamic memory if needed, then return our exit code.
  */
btm:
 if (isfold) free(xtarget);
 return e;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-fs] [-L | -P] source ... target\n",
          progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e, r, t;
 char *target;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 mode=0;
 while (-1!=(e=getopt(argc, argv, "fsLP")))
 {
  switch (e)
  {
   case 'f':
    mode |= MODE_F;
    break;
   case 's':
    mode |= MODE_S;
    break;
   case 'L':
    mode |= MODE_L;
    break;
   case 'P':
    mode &= ~MODE_L;
    break;
   default:
    usage();
  }
 }

 /* At least 2 parameters required. */
 if ((argc-optind)<2)
  usage();

 /*
  * If there is more than one source, the last one MUST be a folder.
  * If there is only one, test this.
  */
 isfold=((argc-optind)>2);
 if (argc-optind==2)
 {
  if (!lstat(argv[argc-1], &statbuf))
   isfold=(statbuf.st_mode&S_IFDIR)?1:0;
 }
 else
 {
  e=0;

  if (stat(argv[argc-1], &statbuf))
   e=1;
  else
  {
   if (!(statbuf.st_mode&S_IFDIR))
    e=1;
  }
  if (e) /* we're supposed to be a directory, but we aren't */
  {
   fprintf (stderr, "%s: %s: not a directory\n", progname, argv[argc-1]);
   return 1;
  }
 }

 /* Do it, maggot. */
 for (e=0, r=0, t=optind; t<(argc-1); t++)
 {
  e=xmklink(argv[t], argv[argc-1]);
  if (r<e) r=e;
 }

 return r;
}
