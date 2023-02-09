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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * -H  Follow only symlinks on the command line
 * -L  Follow all symlinks
 * -P  Follow no symlinks (4.4BSD-Lite, GNU)
 * -a  Report size for files found in recursion
 * -c  Display a total at the end (FreeBSD, OpenBSD, NetBSD, GNU)
 * -h  Humanize output (FreeBSD, OpenBSD, NetBSD, GNU), undocumented
 * -k  Set blocksize to 1024
 * -r  ignore; for SVR4 compatibility
 * -s  Only output the total
 * -x  Do not cross mountpoints
 *
 *     Last -HLP wins
 */

#define MODE_A     0x01
#define MODE_C     0x02
#define MODE_H     0x04
#define MODE_L     0x08
#define MODE_S     0x10
#define MODE_X     0x20
#define MODE_HUMAN 0x40
int mode;

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

int blocksize;

int grand_e;

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

unsigned long divide_up (size_t x, unsigned y)
{
 return (x/y)+(0!=(x%y));
}

void synoptize (char *path, size_t t)
{
 if (mode&MODE_HUMAN)
 {
  if (blocksize==1024) t<<=1;

  if (!t)
   printf ("0\t%s\n", path);
  else if (t<2048) /* KB */
   printf ("%luK\t%s\n", divide_up(t, 2), path);
  else if (t<2097152) /* MB */
   printf ("%luM\t%s\n", divide_up(t, 2048), path);
  else
   printf ("%luG\t%s\n", divide_up(t, 2097152), path);
 }
 else
  printf ("%lu\t%s\n", t, path);
}

int do_du (char *path, size_t *retval)
{
 DIR *dir;
 struct dirent *dirent;
 struct stat statbuf;
 size_t t;
 int e;
 dev_t mydev;

 if (!retval) return -1;

 /*
  * Get the device number of this folder.
  * If we aren't crossing mountpoints, we can use this to determine that we
  * need to skip a folder.
  */
 e=stat(path, &statbuf);
 if (e)
 {
  xperror(path);
  return 1;
 }
 mydev=statbuf.st_dev;

 dir=opendir(path);
 if (!dir) return -1;

 e=0;
 t=0;
 while (1)
 {
  char *n;

  errno=0;
  dirent=readdir(dir);
  if (!dirent)
  {
   if (!errno) break; /* End of folder */
   grand_e=1;
   xperror(path);
   closedir(dir);
   return 1;
  }

  if (!strcmp(dirent->d_name, ".")) continue;
  if (!strcmp(dirent->d_name, "..")) continue;

  n=malloc(strlen(path)+strlen(dirent->d_name)+2);
  if (!n) scram();
  sprintf(n, "%s/%s", path, dirent->d_name);

  /* lstat does not recurse into symlinks into directories. */
  if (0==((mode&MODE_L)?stat:lstat)(n, &statbuf))
  {
   size_t r;

   /* Recurse into folders.  Add to running tally. */
   if (statbuf.st_mode&S_IFDIR)
   {
    /* Mountpoint; skip if the parameter is set */
    if ((statbuf.st_dev!=mydev)&&(mode&MODE_X))
    {
     if (!(mode&MODE_S)) printf ("0\t%s\n", n);
    }
    else
    {
     e=do_du(n, &r);
     if (!e) t+=r;
    }
   }
   else
   {
    if (statbuf.st_mode&S_IFREG)
    {
     size_t u;

     r=statbuf.st_size;

     u=(r/blocksize);
     if (r%blocksize) u++;
     if (mode&MODE_A) synoptize(n, u);
     t+=u;
    }
    else if (mode&MODE_A)
     synoptize(n, 0);
   }
  }
  else
  {
   /* Print error but keep going. */
   e=1;
   grand_e=1;
   xperror(n);
  }
  free(n);
 }

 closedir(dir);
 *retval=t;
 if (!(mode&MODE_S)) synoptize(path, t);
 return 0;
}

int do_du_smart (char *path, size_t *retval)
{
 struct stat statbuf;

 /*
  * This is only called from main().  Otherwise, use do_du().
  *
  * Unless we specified that we specifically want to follow symlinks on the
  * command line, simply fake do_du for them, because doing do_du directly
  * into a link will follow it even if we're in "physical" mode (it won't
  * recurse into links though).
  */
 if (!(mode|(MODE_H|MODE_L)))
 {
  if (!lstat(path, &statbuf))
  {
   if (statbuf.st_mode&S_IFLNK) /* Symlink; fake it. */
   {
    if (!(mode&MODE_S)) printf ("0\t%s\n", path);
    return 0;
   }
  }
 }

 return do_du(path, retval);
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-HLPacksx] path ...\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e, r, t;
 size_t tally, total;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 blocksize=512;

 mode=0;
 while (-1!=(e=getopt(argc, argv, "HLPachkrsx")))
 {
  switch (e)
  {
   case 'H':
    mode|=MODE_H;
    mode&=~MODE_L;
    break;
   case 'L':
    mode|=MODE_L;
    mode&=~MODE_H;
    break;
   case 'P':
    mode&=~(MODE_L|MODE_H);
    break;
   case 'a':
    mode|=MODE_A;
    break;
   case 'c':
    mode|=MODE_C;
    break;
   case 'h':
    mode|=MODE_HUMAN;
    break;
   case 'k':
    blocksize=1024;
    break;
   case 'r': /* NOP */
    break;
   case 's':
    mode|=MODE_S;
    break;
   case 'x':
    mode|=MODE_X;
    break;
   default:
    usage();
  }
 }

 /* -h override -k */
 if (mode&MODE_HUMAN) blocksize=512;

 if ((mode&(MODE_C|MODE_S))==(MODE_C|MODE_S))
 {
  fprintf (stderr, "%s: -c and -s are mutually exclusive\n", progname);
  return 1;
 }

 if ((mode&(MODE_A|MODE_S))==(MODE_A|MODE_S))
 {
  fprintf (stderr, "%s: -a and -s are mutually exclusive\n", progname);
  return 1;
 }

 grand_e=0;

 if (optind==argc)
 {
  do_du(".", &total);
 }
 else
 {
  total=0;

  for (t=optind; t<argc; t++)
  {
   e=do_du_smart(argv[t], &tally);
   if (e)
    total+=tally;
   else
    r=1;
  }
 }

 if (mode&(MODE_S|MODE_C))
  printf ("%lu\ttotal\n", total);

 return grand_e;
}
