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
 * This code is not fully tested.
 * 
 * Note: Timestamps are not preserved for directories, symlinks or special
 *       files when moving across devices.
 * 
 * Note: Pipes and sockets are not moved across devices; a directory
 *       containing either will cause mv to blow up the console with
 *       diagnostics.
 * 
 * Note: The only extension we support over Posix is "-v", used by GNU and the
 *       BSDs.  FreeBSD's -h and -n are not supported nor are any of GNU's
 *       unique extensions.
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
#include <utime.h>
#include <ftw.h>

#define ITERATE_MAX_HANDLES 5

#ifdef __SVR4__
#define CP_OLD_UTIME
#endif

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

enum {MODE_D, MODE_F, MODE_I, MODE_N} mode;
int verbose;
struct stat statbuf;
int global_e;

char infn[PATH_MAX], outfn[PATH_MAX], mkln[PATH_MAX];

char *ref_in, *ref_out;

void xperror (const char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

/* Copy a file, preserving as many features as possible. */
int copyfile (char *from, char *to)
{
 int e;
 int r;
 FILE *in, *out;
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
 
 /* Can't get info from file; scream */
 if (lstat(from, &statbuf))
 {
  xperror(from);
  return 2;
 }

 /* Save info */
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
 
 /* Open files */
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
 
 /* Transfer data BUFSIZ bytes at a time */
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
 
 /* Last partial block */
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
 
 /* Change the owner; if we can't, disable SUID-SGID */
 e=fchown(fileno(out), u, g);
 if (e)
 {
  fprintf (stderr, "%s: %s: cannot chown: %s\n", 
           progname, to, strerror(errno));
  m&=~(S_ISUID|S_ISGID);
  r=1;
 }
 
 /* Copy file mode */
 e=fchmod(fileno(out), m);
 if (e)
 {
  fprintf (stderr, "%s: %s: cannot chmod: %s\n",
           progname, to, strerror(errno));
  r=1;
 }
 
 fclose(in);
 fclose(out);
 
 /* Copy timestamps */
#ifdef CP_OLD_UTIME
 e=utime(to, &T);
#else
 e=utimensat(AT_FDCWD, to, T, 0);
#endif
 return r;
}

/* Move one file by copying and deleting. */
int cpmv (char *from, char *to)
{
 /* Scream if we can't figure out what we're moving. */
 if (lstat(from, &statbuf))
 {
  xperror(from);
  return 1;
 }
 
 /*
  * Don't touch directories (we have a different way of handling those), pipes
  * or sockets.  Just scream and scram.
  */
 if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
 {
  fprintf (stderr, "%s: %s: not recursing\n", progname, from);
  return 1;
 }
 if ((statbuf.st_mode & S_IFMT) == S_IFIFO)
 {
  fprintf (stderr, "%s: %s: not cloning pipe\n", progname, from);
  return 1;
 }
 if ((statbuf.st_mode & S_IFMT) == S_IFSOCK)
 {
  fprintf (stderr, "%s: %s: not cloning socket\n", progname, from);
  return 1;
 }
 
 /* Block special, character special, and symlink files: clone them. */
 if (statbuf.st_mode & (S_IFBLK|S_IFCHR))
 {
  if (mknod(to, statbuf.st_mode, statbuf.st_rdev))
  {
   xperror(to);
   return 1;
  }
  if (verbose)
   printf ("cloned %s -> %s\n", from, to);
  if (unlink(from))
  {
   xperror(from);
   global_e=1;
   return 0;
  }
  if (verbose)
   printf ("removed %s\n", from);
  return 0;
 }
 if ((statbuf.st_mode & S_IFMT) == S_IFLNK)
 {
  memset(mkln, 0, PATH_MAX);
  if (readlink(from, mkln, PATH_MAX)==-1)
  {
   xperror(from);
   return 1;
  }
  if (symlink(mkln, to))
  {
   xperror(to);
   return 1;
  }
  if (verbose)
   printf ("cloned %s -> %s\n", from, to);
  if (unlink(from))
  {
   xperror(from);
   global_e=1;
   return 0;
  }
  if (verbose)
   printf ("removed %s\n", from);
  return 0;
 }

 /* Other files: copy them. */
 if (copyfile(from, to)<2)
 {
  if (verbose)
   printf ("copied %s -> %s\n", from, to);
  if (unlink(from))
  {
   xperror(from);
   return 1;
  }
  if (verbose)
   printf ("deleted %s\n", from);
  return 0;
 }
 return 1;
}

/* Check whether we need to prompt. */
int ckzot (char *fn)
{
 /* If we can't lstat() the file, there's probably nothing to clobber. */
 if (!lstat(fn, &statbuf))
  return 0;
 
 if (mode==MODE_N) /* No clobber (GNU, FreeBSD) */
  return 1;
 
 if (mode==MODE_I) /* Prompt before zotting. */
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
 
 if (mode==MODE_F) /* Always zot. */
  unlink(fn);
 
 return 0;
}

/* Clean up by zotting source directory tree */
int iterate_zot (const char *filename, const struct stat *statptr, 
                 int fileflags, struct FTW *pftw)
{
 if (rmdir(filename))
 {
  global_e=1;
  xperror(filename);
 }
 else if (verbose)
  printf ("zotted directory %s\n", filename);
 return 0;
}

/* Iterate over the tree moving files */
int iterate_hit (const char *filename, const struct stat *statptr, 
                 int fileflags, struct FTW *pftw)
{
 char *p;
 char mkfn[PATH_MAX];
 
 /* Create target filename */
 p=(char *)(filename+strlen(ref_in)+1);
 if ((strlen(ref_in)+strlen(p)+2)>PATH_MAX)
 {
  global_e=1;
  fprintf (stderr, "%s: %s: target filename would be too long\n", 
           progname, filename);
  return 0;
 }
 
 while (ref_out[strlen(ref_out)-1]=='/')
  ref_out[strlen(ref_out)-1]=0;
 
 sprintf (mkfn, "%s/%s", ref_out, p);
 
 switch (fileflags)
 {
  case FTW_D: /* Directory: make new, second iteration will clean up */
   if (mkdir(mkfn, statptr->st_mode))
   {
    xperror(mkfn);
    global_e=1;
    return 0;
   }
   if (verbose)
    printf ("created %s\n", mkfn);
   break;
  case FTW_SLN: /* Broken symlink: scream but do not die */
   global_e=1;
   fprintf (stderr, "%s: %s: not copying broken symlink\n", 
            progname, filename);
   return 0;
  case FTW_SL: /* Working symlink: clone it, then zot */
   memset(mkln, 0, PATH_MAX);
   if (readlink(filename, mkln, PATH_MAX)==-1)
   {
    xperror(filename);
    global_e=1;
    return 0;
   }
   if (symlink(mkln, mkfn))
   {
    xperror(mkfn);
    global_e=1;
    return 0;
   }
   if (verbose)
    printf ("cloned %s -> %s\n", filename, mkfn);
   if (unlink(filename))
   {
    xperror(filename);
    global_e=1;
    return 0;
   }
   if (verbose)
    printf ("removed %s\n", filename);
   break;
  default:
   if (!ckzot(mkfn)) cpmv((char *)filename, mkfn);
 }
 return 0;
}

/* Front end that will rename or copy/delete as needed */
int special_mv (char *from, char *to)
{
 int e;
 char *b;
 
 /* Basename */
 b=strrchr(from, '/');
 if (b) b++; else b=from;
 
 /* Does "to" exist? */ 
 if (!lstat(to, &statbuf))
 {
  /* Is it a folder? */
  if (statbuf.st_mode&S_IFDIR)
  {
   if ((strlen(to)+strlen(b)+2)>PATH_MAX)
   {
    fprintf (stderr, "%s: %s: target filename would be too long\n", 
             progname, from);
    return 1;
   }
   sprintf (outfn, "%s/%s", to, b);
   if (ckzot(outfn)) return 1;
  }
  else /* Exists, but is a file. */
  {
   if (ckzot(to)) return 1;
   strcpy(outfn, to);
  }
 }
 else
  strcpy(outfn, to);
 
 /* Try rename(2) */
 errno=0;
 if (0==(e=rename(from, outfn)))
 {
  if (verbose) 
   printf ("renamed %s -> %s\n", from, outfn);
  return 0;
 }
 
 /* Failed, but not because of EXDEV */
 if (errno!=EXDEV)
 {
  xperror(outfn);
  return 1;
 }
 
 /*
  * Rename failed because of EXDEV, so our target is on another drive.
  * If we are a single file, we could try copying and deleting...
  */
 if (lstat(from, &statbuf))
 {
  xperror(from);
  return 1;
 }

 /* Not a directory: copy and delete one file. */
 if ((statbuf.st_mode & S_IFMT) != S_IFDIR)
  return cpmv(from, outfn);

 ref_in=from;
 ref_out=outfn;
 global_e=0;
 
 /* First iteration: move files */
 nftw(from, iterate_hit, ITERATE_MAX_HANDLES, FTW_PHYS);

 /* Second iteration: zot trees */
 nftw(from, iterate_zot, ITERATE_MAX_HANDLES, FTW_PHYS|FTW_DEPTH);
 
 return global_e;
}

void mv_usage (void)
{
 fprintf (stderr, "%s: usage: %s [-fiv] source ... target\n", 
          progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 char *target;
 int e, r, t;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (isatty(0))
  mode=MODE_D;
 else
  mode=MODE_F;
 
 verbose=0;
 while (-1!=(e=getopt(argc, argv, "fivn")))
 {
  switch (e)
  {
   case 'f':
    mode=MODE_F;
    break;
   case 'i':
    mode=MODE_I;
    break;
   case 'v':
    verbose=1;
    break;
   case 'n':
    mode=MODE_N;
    break;
   default:
    mv_usage();
  }
 }
 
 /* Definitely missing a target path */
 if (argc-optind<2) mv_usage();
 
 /* Take the last argument, then strip it off. */
 target=argv[--argc];
 while (target[strlen(target)-1]=='/')
  target[strlen(target)-1]=0;
 
 /* More than 1 source parameter: target must exist and be a directory. */
 if (argc-optind>1)
 {
  if (lstat(target, &statbuf))
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
  e=special_mv(argv[t], target);
  if (r<e) r=e;
 }
 
 return r;
}
