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
 * dircmp used to be part of POSIX; it was designated deprecated then, and
 * eventually stripped out of the standard.  We follow this version of dircmp;
 * we don't even try to make our output look anything like AT&T's version.
 *
 * For compatibility with System V Release 2 and later, accept -w (with a
 * sane default, too!), but ignore it; we don't format in a way where the line
 * length actually makes any difference to us.
 *
 * This program is almost as braindead as the original.  Please use "diff -r"
 * instead - it does about everything this can without profound brain damage.
 */

#if (defined(__linux__)||defined(__SVR4__))
#define _XOPEN_SOURCE 500
#endif

#ifdef __linux__
#define _GNU_SOURCE 1
#define _DEFAULT_SOURCE 1
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <ftw.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __SVR4__
typedef signed long ssize_t;
extern char *strdup (const char *);
#endif

/*
 * Maximum number of handles to let nftw(3) use, if it matters.
 * (NetBSD doesn't use it.)
 */
#define ITERATE_MAX_HANDLES 5

/* The location of diff(1). */
#ifndef PATH_DIFF
#define PATH_DIFF "/usr/bin/diff"
#endif

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

char **curfiltab;
char **filtab1, **filtab2;
long long count0, count1, count2;

/* Common point to die screaming if we run out of memory. */
void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

/* "Safe" chdir, in case a folder is zotted while we're iterating */
void chdir_or_scram (char *path)
{
 if (chdir(path))
 {
  fprintf (stderr, "%s: the rug was pulled out from under us\n", progname);
  exit(1);
 }
}

/* Modified perror that shows our name at the head. */
void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

/*
 * Execute diff(1) if requested by the user.
 * We are supposed to run in the heinously anus default mode rather than the
 * much saner unidiff mode.
 */
void diff (char *path1, char *path2, char *filename)
{
 pid_t p;
 char *mkarg1, *mkarg2;

 /* Make room to construct the arguments to diff. */
 mkarg1=malloc(strlen(path1)+strlen(filename)+2);
 if (!mkarg1) scram();

 mkarg2=malloc(strlen(path2)+strlen(filename)+2);
 if (!mkarg2) scram();

 sprintf (mkarg1, "%s/%s", path1, filename);
 sprintf (mkarg2, "%s/%s", path2, filename);

 /*
  * In the main process:
  *   Just twiddle our thumbs.
  *
  * In the subprocess:
  *   Run the constructed command line.
  */
 p=fork();
 if (p<0)
 {
  fprintf (stderr, "%s: warning: could not fork\n", progname);
  return;
 }
 if (!p)
 {
  execl (PATH_DIFF, "diff", "--", mkarg1, mkarg2, NULL);
  fprintf (stderr, "%s: warning: could not exec " PATH_DIFF "\n", progname);
 }
 else
 {
  wait(NULL);
 }
}

/*
 * Sort a file table.
 *
 * This is the daft "Exchange Sort" algo, but at least it's not the worst of
 * all possible sort algos.
 */
void sortfiltab (char **filtab, long long count)
{
 long long t1, t2;
 int clean;

 clean=0;
 while (!clean)
 {
  clean=1;

  for (t1=0; t1<count; t1++)
  {
   for (t2=t1+1; t2<count; t2++)
   {
    if (strcmp(filtab[t1], filtab[t2])>0)
    {
     char *tmp;

     tmp=filtab[t1];
     filtab[t1]=filtab[t2];
     filtab[t2]=tmp;
     clean=0;
    }
   }
  }
 }
}

/*
 * Callback from nftw() when we get a filename while adding to one of our
 * running lists.  Just strdup() it into our list.
 */
int iterate_add (const char *filename, const struct stat *statptr,
                 int fileflags, struct FTW *pftw)
{
 if (!strcmp(filename, ".")) return 0;

 if (NULL==(curfiltab[count0++]=strdup(filename)))
 {
  scram();
 }

 return 0;
}

/*
 * Callback from nftw() when we get a filename while tallying entries.  Just
 * increment the current counter.
 */
int iterate_count (const char *filename, const struct stat *statptr,
                   int fileflags, struct FTW *pftw)
{
 if (strcmp(filename, ".")) count0++;
 return 0;
}

/* Die screaming with a usage diagnostic. */
void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-ds] path1 path2\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 char mypwd[PATH_MAX];
 int e;
 int diffme, s, w;
 char *path1, *path2;
 long long t1, t2;
 int mode;
#ifdef __SVR4__ /* braindead libc */
 extern int optind;
 extern char *optarg;
#endif

 /* Sanitize argv[0]. */
 progname=strrchr(*argv, '/');
 if (progname) progname++; else progname=*argv;

 /* Get options. */
 diffme=s=0;
 w=66;
 while (-1!=(e=getopt(argc, argv, "dsw:")))
 {
  switch (e)
  {
   case 'd':
    diffme=1;
    break;
   case 's':
    s=1;
    break;
   case 'w':
    errno=0;
    w=strtol(optarg, 0, 0);
    if (errno)
    {
     fprintf (stderr, "%s: bogus width: '%s'\n", progname, optarg);
     return 1;
    }
    break;
   default:
    usage();
  }
 }

 /* Make sure we have exactly 2 arguments, and save them. */
 if (argc-optind!=2) usage();

 path1=argv[optind];
 path2=argv[optind+1];

 if (!getcwd(mypwd, PATH_MAX))
 {
  fprintf (stderr, "%s: could not obtain current working directory\n",
           progname);
  return 1;
 }

 /*
  * Sanity checks.
  *
  * Once we know our paths work, use a common error handler with each chdir()
  * that dies screaming if it fails (because our work folder got zotted or
  * renamed from underneath us, which would be Very Bad®™©).
  */
 if (chdir(path1))
 {
  perror(path1);
  return 1;
 }
 if (chdir(mypwd))
 {
  fprintf (stderr, "%s: could not return to current working directory\n",
           progname);
  return 1;
 }
 if (chdir(path2))
 {
  perror(path2);
  return 1;
 }
 chdir_or_scram(mypwd);

 /*
  * Start counting files in each folder.
  * Then allocate enough room for a pointer to each one.
  */
 chdir_or_scram(path1);
 count0=0;
 e=nftw(".", iterate_count, ITERATE_MAX_HANDLES, 0);
 count1=count0;
 chdir_or_scram(mypwd);
 chdir_or_scram(path2);
 count0=0;
 e=nftw(".", iterate_count, ITERATE_MAX_HANDLES, 0);
 count2=count0;
 chdir_or_scram(mypwd);
 filtab1=calloc(count1, sizeof(char *));
 if (!filtab1) scram();
 filtab2=calloc(count2, sizeof(char *));
 if (!filtab2) scram();

 /*
  * Now create our file lists.
  * When we've got them, sort them.
  */
 count0=0;
 chdir_or_scram(path1);
 curfiltab=filtab1;
 e=nftw(".", iterate_add, ITERATE_MAX_HANDLES, 0);
 chdir_or_scram(mypwd);
 count0=0;
 chdir_or_scram(path2);
 curfiltab=filtab2;
 e=nftw(".", iterate_add, ITERATE_MAX_HANDLES, 0);
 chdir_or_scram(mypwd);
 sortfiltab(filtab1, count1);
 sortfiltab(filtab2, count2);

 /*
  * Time to iterate over the lists.
  * First, show files only in the source path.
  * Then show files only in the target path.
  * Finally, show files in both paths.  This is where it gets...no, it doesn't
  * get ugly, it gets ügly!
  */
 for (mode=0; mode<3; mode++)
 {
  int q=0;

  t1=t2=0;
  while ((t1<count1)&&(t2<count2))
  {
   if (strcmp(filtab1[t1], filtab2[t2])<0)
   {
    if (mode==0)
    {
     if (!q)
     {
      q=1;
      printf ("only in %s:\n", path1);
     }
     printf ("  %s\n", filtab1[t1]);
    }
    t1++;
   }
   else if (strcmp(filtab1[t1], filtab2[t2])>0)
   {
    if (mode==1)
    {
     if (!q)
     {
      q=1;
      printf ("only in %s:\n", path2);
     }
     printf ("  %s\n", filtab2[t2]);
    }
    t2++;
   }
   else if (!strcmp(filtab1[t1], filtab2[t2]))
   {
    struct stat statbuf;
    ssize_t s1, s2;

    if (mode==2)
    {
     if (!q)
     {
      q=1;
      printf ("in %s and %s:\n", path1, path2);
     }
     chdir_or_scram(path1);

     /* First get file size or directory status for each entry. */
     e=stat(filtab1[t1], &statbuf);
     if (e)
     {
      printf ("  %s: could not stat source\n", filtab1[t1]);
      s1=-1;
     }
     else
      s1=statbuf.st_size;
     if (statbuf.st_mode&S_IFDIR)
      s1=-2;
     chdir_or_scram(mypwd);
     chdir_or_scram(path2);
     e=stat(filtab2[t2], &statbuf);
     if (e)
     {
      printf ("  %s: could not stat target\n", filtab2[t2]);
      s2=-1;
     }
     else
      s2=statbuf.st_size;
     if (statbuf.st_mode&S_IFDIR)
      s2=-2;
     chdir_or_scram(mypwd);

     /*
      * Do a quick compare (just to see IF they're different).
      * If the user wants us to, then run diff(1) on the different ones.
      * If not, just display if they're different (and, if -s not supplied,
      * also display if they're the same).
      */
     if ((s1==s2)&&(s1>=0))
     {
      FILE *f1, *f2;

      chdir_or_scram(path1);
      f1=fopen(filtab1[t1], "rb");
      chdir_or_scram(mypwd);
      if (!f1)
      {
       printf ("  %s: could not open source\n", filtab1[t1]);
      }
      else
      {
       chdir_or_scram(path2);
       f2=fopen(filtab2[t2], "rb");
       chdir_or_scram(mypwd);
       if (!f2)
       {
        fclose(f1);
        printf ("  %s: could not open target\n", filtab2[t2]);
       }
       else
       {
        int d;
        long long c;

        d=0;
        for (c=0; c<s1; c++)
        {
         if (fgetc(f1)!=fgetc(f2))
         {
          d=1;
          break;
         }
        }
        fclose(f1);
        fclose(f2);

        /* -s - suppresses messages about files that are the same. */
        if (s)
        {
         if (d) printf ("  %s (different)\n", filtab1[t1]);
        }
        else
        {
         printf ("  %s (%s)\n", filtab1[t1], d?"different":"same");
        }
        if (diffme&&d) diff(path1, path2, filtab1[t1]);
       }
      }
     }
     else
     {
      printf ("  %s", filtab1[t1]);
      if ((s1!=-2)&(s2!=-2))
      {
       printf (" (different)");
       if (diffme) diff(path1, path2, filtab1[t1]);
      }
      if (s1==-2) printf (" (directory in %s)", path1);
      if (s2==-2) printf (" (directory in %s)", path2);
      printf ("\n");
     }
    }
    t1++, t2++;
   }
  }
 }

 /*
  * Let's just leave the OS to clean up after our nasty butts and beat a quick
  * retreat the hell out of here.
  */
 return 0;
}
