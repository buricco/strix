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
 * This program is currently unusable.
 * It is merely a framework upon which to build the cp(1) utility.
 * Most of the needed code is probably in mv.
 */

#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ftw.h>

#define ITERATE_MAX_HANDLES 5

#ifdef __SVR4__
#define CP_OLD_UTIME
#endif

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

#define MODE_H 0x01 /* Resolve symlinks explicitly specified */
#define MODE_L 0x02 /* Resolve all symlinks                  */
#define MODE_R 0x04 /* Recurse into folders                  */
#define MODE_F 0x08 /* Force overwrite                       */
#define MODE_I 0x10 /* Prompt on overwrite                   */
#define MODE_N 0x20 /* Silently refuse to overwrite          */
#define MODE_P 0x40 /* Preserve more attributes when copying */
#define MODE_V 0x80 /* Verbose                               */
int mode;

struct stat statbuf;

void xperror (const char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

/*
 * Routine to copy a file.
 * 
 * If copying all attributes is desired (cp -p), set attr to nonzero.
 * Otherwise, only basic permissions will be copied (as with regular cp).
 * Returns 0=success, 1=warnings, 2=failure.
 */
int copyfile (char *from, char *to, int attr)
{
 int e;
 int r;
 FILE *in, *out;
 struct stat statbuf;
 char cpbuf[BUFSIZ];
 uid_t u;
 gid_t g;
 off_t l;
 mode_t m;
 
#ifdef CP_OLD_UTIME
 struct utimbuf T;
#else
 struct timespec T[2];
#endif
 
 r=0;
 
 if (stat(from, &statbuf))
 {
  xperror(from);
  return 2;
 }
 
 l=statbuf.st_size;
 u=statbuf.st_uid;
 g=statbuf.st_gid;
 m=statbuf.st_mode;
#ifdef CP_OLD_UTIME
 T.actime=statbuf.st_atime;
 T.modtime=statbuf.st_mtime;
#else
 T[0]=statbuf.st_atim;
 T[1]=statbuf.st_mtim;
#endif
 
 in=fopen(from, "rb");
 if (!in)
 {
  xperror(from);
  return 2;
 }
 
 out=fopen(to, "wb");
 if (!out)
 {
  xperror(to);
  fclose(in);
  return 2;
 }
 
 while (l>BUFSIZ)
 {
  e=fread(cpbuf, 1, BUFSIZ, in);
  if (e<BUFSIZ)
  {
   fclose(in);
   fclose(out);
   xperror(from);
   return 2;
  }
  
  e=fwrite(cpbuf, 1, BUFSIZ, out);
  if (e<BUFSIZ)
  {
   fclose(in);
   fclose(out);
   xperror(to);
   return 2;
  }
  
  l-=BUFSIZ;
 }
 
 if (l)
 {
  e=fread(cpbuf, 1, l, in);
  if (e<l)
  {
   fclose(in);
   fclose(out);
   xperror(from);
   return 2;
  }
  
  e=fwrite(cpbuf, 1, l, out);
  if (e<l)
  {
   fclose(in);
   fclose(out);
   xperror(to);
   return 2;
  }
 }
 
 r=0;
 
 if (attr)
 {
  /* Change the owner; if we can't, disable SUID-SGID */
  e=fchown(fileno(out), u, g);
  if (e)
  {
   fprintf (stderr, "%s: %s: cannot chown: %s\n", 
            progname, to, strerror(errno));
   m&=~(S_ISUID|S_ISGID);
   r=1;
  }
 }
 else
 {
  /* Strip the permissions we copy down to within the basic 777. */
  m&=0777;
 }
 
 e=fchmod(fileno(out), m);
 if (e)
 {
  fprintf (stderr, "%s: %s: cannot chmod: %s\n",
           progname, to, strerror(errno));
  r=1;
 }
 
 fclose(in);
 fclose(out);
 
 if (!attr) return r;
 
 /* Copy timestamps */
#ifdef CP_OLD_UTIME
 e=utime(to, T);
#else
 e=utimensat(AT_FDCWD, to, T, 0);
#endif
 return r;
}

/* Check whether we need to prompt. */
int ckzot (char *fn)
{
 /* If we can't lstat() the file, there's probably nothing to clobber. */
 if (!lstat(fn, &statbuf))
  return 0;
 
 if (mode&MODE_N) /* No clobber (GNU, FreeBSD) */
  return 1;
 
 if (mode&MODE_I) /* Prompt before zotting. */
 {
  while (1)
  {
   char p[20];
     
   fflush(stdin);
   printf ("%s: clobber %s (y/n)? ", progname, fn);
   fgets(p, 19, stdin);
   if (feof(stdin)) exit(1);
   if (*p=='y') break;
   if (*p=='n') return 1;
  }
    
  /* Eff outta here. */
  if (unlink(fn))
  {
   xperror(fn);
   return 1;
  }
 }
 
 if (mode&MODE_F) /* Always zot. */
  unlink(fn);
 
 return 0;
}

int do_cp (char *from, char *to)
{
 /* Can't stat; no sense progressing further */
 if (lstat(from, &statbuf))
 {
  xperror(from);
  return 1;
 }
 
 /* -H - resolve symlinks on the command line only */
 if ((statbuf.st_mode & S_IFMT) == S_IFLNK)
 {
  char *mkln;
  
  mkln=malloc(PATH_MAX);
  if (!mkln) scram();
  memset(mkln, 0, PATH_MAX);
  if (readlink(from, mkln, PATH_MAX)==-1)
  {
   xperror(from);
   return 1;
  }
  e=do_cp(mkln, to);
  free(mkln);
  return e;
 }
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-HLPRafipv] source ... target\n", 
          progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 char *target;
 int e, r, t;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 while (-1!=(e=getopt(argc, argv, "HLPRfinprv")))
 {
  switch (e)
  {
   case 'H':
    mode &= ~MODE_L;
    mode |= MODE_H;
    break;
   case 'L':
    mode &= ~MODE_H;
    mode |= MODE_L;
    break;
   case 'P':
    mode &= ~(MODE_H|MODE_L);
    break;
   case 'R': /* FALL THROUGH */
   case 'r':
    mode |= MODE_R;
    break;
   case 'f':
    mode &= ~(MODE_I|MODE_N);
    mode |= MODE_F;
    break;
   case 'i':
    mode &= ~(MODE_F|MODE_N);
    mode |= MODE_I;
    break;
   case 'n':
    mode &= ~(MODE_F|MODE_I);
    mode |= MODE_N;
    break;
   case 'p':
    mode |= MODE_P;
    break;
   case 'v':
    mode |= MODE_V;
    break;
   case 'a': /* -a = -RpP */
    mode |= (MODE_R|MODE_P);
    mode &= ~(MODE_L|MODE_H);
    break;
   default:
    usage();
  }
 }
 
 /* Definitely missing a target path */
 if (argc-optind<2) usage();
 
 /* Take the last argument, then strip it off. */
 target=argv[--argc];
 while (target[strlen(target)-1]=='/')
  target[strlen(target)-1]=0;
 
 /* More than 1 source parameter: target must exist and be a directory. */
 if (argc-optind>1)
 {
  if (stat(target, &statbuf))
  {
   xperror(target);
   return 1;
  }
  if ((statbuf.st_mode & S_IFMT) != S_IFDIR)
  {
   fprintf (stderr, "%s: %s: %s", progname, target, strerror(ENOTDIR));
   return 1;
  }
 }
 
 r=0;
 for (t=optind; t<argc; t++)
 {
  while ((argv[t])[strlen(argv[t])-1]=='/')
   (argv[t])[strlen(argv[t])-1]=0;
  e=do_cp(argv[t], target);
  if (r<e) r=e;
 }
 
 return r;
}
