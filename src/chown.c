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
 * The various modes of recursion:
 *   -H - Physical; but if any command line parameters are symlinks, resolve
 *        them before iterating, using readlink(2).
 *   -L - Logical.
 *   -P - Physical.
 */

#define _XOPEN_SOURCE 700 /* nftw(3) */
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <ftw.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __SVR4__
extern char *strdup (const char *);
#endif

#define ITERATE_MAX_HANDLES 5

static char *progname;

#define MODE_H       0x01  /* Physical, but traverse into manually specified links */
#define MODE_L       0x02  /* Logical (lchown) */
#define MODE_P       0x04  /* Physical (chown) */
#define MODE_R       0x08  /* Recurse */
#define MODE_LCHOWN  0x10  /* chown a link (not on Linux) */
int mode;

uid_t uid;
gid_t gid;

int global_e;
int is_chgrp;

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

int iterate_chown (const char *filename, const struct stat *statptr,
                   int fileflags, struct FTW *pftw)
{
 int e;

 e=chown(filename, uid, gid);
 if (e) xperror((char *)filename);
 if (global_e<e) global_e=e;
 return 0;
}

/*
 * The "override" parameter is used with parsing -H.
 *
 * In this case, a filename supplied on the command line, if it is a symlink,
 * needs to be resolved; but otherwise, treat it as if -P was supplied.
 */
int do_chown (char *filename, int override)
{
 int e;

 /* No recursion: do it, maggot */
 if (!(mode&MODE_R))
 {
  e=((mode&MODE_LCHOWN)?lchown:chown)(filename, uid, gid);
  if (e) xperror(filename);
  return e;
 }

 if ((mode&MODE_H)&&(!override))
 {
  struct stat statbuf;
  char buf[PATH_MAX+1];

  e=lstat(filename, &statbuf);
  if (e)
  {
   xperror(filename);
   return 1;
  }
  if (statbuf.st_mode&S_IFLNK)
  {
   memset(buf, 0, PATH_MAX+1);
   e=readlink(filename, buf, PATH_MAX+1);
   if (e)
   {
    xperror(filename);
    return 1;
   }
   return do_chown(buf, 1);
  }
 }

 global_e=0;
 nftw(filename, iterate_chown, ITERATE_MAX_HANDLES,
      (mode&(MODE_H|MODE_P))?FTW_PHYS:0);
 return global_e;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-R [-H | -L | -P]] %s file...\n",
          progname, progname, is_chgrp?"group":"owner[:group]");
 exit(1);
}

int main (int argc, char **argv)
{
 int e, r, t;
 char *p_owner, *p_group;
 int got_owner, got_group;
 struct passwd *p;
 struct group *g;
#ifdef __SVR4__ /* braindead libc */
 extern int optind;
#endif

 is_chgrp=0;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 if (!strcmp(progname, "chgrp")) is_chgrp=1;

 /*
  * Posix doesn't actually say whether to default to -H, -L or -P; it only
  * says use the last one supplied.  -P seems like as good a default as any.
  */
 mode=MODE_P;
 while (-1!=(e=getopt(argc, argv, "hrRHLP")))
 {
  switch (e)
  {
   case 'h':
    mode |= MODE_LCHOWN;
    break;
   case 'H':
    mode &= ~(MODE_H|MODE_L|MODE_P);
    mode |= MODE_H;
    break;
   case 'L':
    mode &= ~(MODE_H|MODE_L|MODE_P);
    mode |= MODE_L;
    break;
   case 'P':
    mode &= ~(MODE_H|MODE_L|MODE_P);
    mode |= MODE_P;
    break;
   case 'r': /* FALL THROUGH */
   case 'R':
    mode |= MODE_R;
    break;
   default:
    usage();
  }
 }

 /* Make sure we have a user or group and at least one filename. */
 if ((argc-optind)<2) usage();

 if (is_chgrp)
 {
  got_owner=0;
  got_group=1;
  p_owner=strdup(argv[optind]);
  p_group=p_owner;
 }
 else
 {
  /* Split the user and group. */
  p_owner=strdup(argv[optind]);
  if (!p_owner) scram();
  p_group=strchr(p_owner,':');
  if (p_owner==p_group)
  {
   *p_owner=0;
   p_group++;
   got_owner=0;
   got_group=1;
  }
  else if (p_group)
  {
   *(p_group++)=0;
   got_owner=1;
   got_group=1;
  }
  else
  {
   got_owner=1;
   got_group=0;
  }
 }

 /* Translate user and group names into numbers. */
 if (got_owner)
 {
  if (isdigit(*p_owner))
  {
   errno=0;
   uid=strtol(p_owner, 0, 0);
   if (errno)
   {
    fprintf (stderr, "%s: invalid user '%s'\n", progname, p_owner);
    free(p_owner);
    return 1;
   }
  }
  else
  {
   p=getpwnam(p_owner);
   if (!p)
   {
    fprintf (stderr, "%s: invalid user '%s'\n", progname, p_owner);
    free(p_owner);
    return 1;
   }
   uid=p->pw_uid;
  }
 }
 else uid=-1;

 if (got_group)
 {
  if (isdigit(*p_group))
  {
   errno=0;
   gid=strtol(p_group, 0, 0);
   if (errno)
   {
    fprintf (stderr, "%s: invalid group '%s'\n", progname, p_group);
    free(p_group);
    return 1;
   }
  }
  else
  {
   g=getgrnam(p_group);
   if (!g)
   {
    fprintf (stderr, "%s: invalid group '%s'\n", progname, p_group);
    free(p_owner);
    return 1;
   }
   gid=g->gr_gid;
  }
 }
 else gid=-1;

 /* Iterate. */
 r=0;
 for (t=optind+1; t<argc; t++)
 {
  e=do_chown(argv[t], 0);
  if (r<e) r=e;
 }

 return r;
}
