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
 * -a filename        : Exists (GNU, Solaris) - NOT SUPPORTED, use -e instead.
 * -b filename        : Block device
 * -c filename        : Character device
 * -d filename        : Directory
 * -e filename        : Exists
 * -f filename        : (normal) File
 * -g filename        : SGID
 * -h filename        : Symlink
 * -k filename        : Sticky (SVR4)
 * -l filename        : Symlink
 * -n string          : String is not null
 * -p filename        : FIFO
 * -r filename        : Readable
 * -S filename        : Socket
 * -s filename        : File with a size >0 bytes
 * -t handle          : Terminal
 * -u filename        : SUID
 * -w filename        : Writable
 * -x filename        : Executable
 * -z filename        : String is zero (null)
 * string             : String is not null
 * string1 = string2  : Strings match
 * string1 != string2 : Strings do not match
 * value1 -eq value2  : Integers are equal
 * value1 -ne value2  : Integers are non-equal
 * value1 -gt value2  : >
 * value1 -ge value2  : >=
 * value1 -lt value2  : <
 * value1 -le value2  : <=
 * "-a" and "-o" evaluate left to right.  ! and ( ) also work
 *
 * pass = 0
 * fail = 1
 * error = 2
 * 
 * Some things don't work like in other implementations of test(1).  It looks
 * like there's a lot of undefined behavior here, and my approach has been to
 * deal with it logically, meaning that sometimes you will get screaming death
 * when bash and co. will eat the input just fine.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

/* Generic screaming death for calloc() fails. */
void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(2);
}

/* Generic screaming death for syntax errors. */
void derp (void)
{
 fprintf (stderr, "%s: syntax error\n", progname);
 exit(2);
}

/* If argument is not a number, die screaming. */
#ifdef __SVR4__
long num_or_die (char *x)
{
 long t;
#else
long long num_or_die (char *x)
{
 long long t;
#endif

 errno=0;

#ifdef __SVR4__
 t=strtol(x, 0, 0);
#else
 t=strtoll(x, 0, 0);
#endif
 if (errno)
 {
  fprintf (stderr, "%s: integer expression expected: '%s'\n", progname, x);
  exit(2);
 }

 return t;
}

/*
 * The meat and potatoes of this whole ugly operation.
 * It frequently recurses into itself to handle "and" and "or" operations.
 * General behavior should be as previous implementations, though there is
 * heavier error checking, perhaps, than previous implementations.
 *
 * argv[0] will usually be bogus during iterations of do_test, so use 
 * progname, which will never change.
 */
int do_test (int argc, char **argv)
{
 int e, t;
 int nargc;
 char **nargv;
 struct stat statbuf;
 
 /* No args - return 1. Also sprach Posix */
 if (argc==1) return 1;

 /* One arg - return 0 unless empty string. Also sprach Posix */
 if (argc==2) return !(*argv[1]);

 /*
  * The string comparators need to come before -a and -o.  Also sprach Posix
  */
 if (argc==4) /* Basic comparators */
 {
  /* String comparisons, should be obvious */
  if (!strcmp(argv[2], "="))
   return (strcmp(argv[1], argv[3]))?1:0;
  if (!strcmp(argv[2], "==")) /* Berzerkeley! */
   return (strcmp(argv[1], argv[3]))?1:0;
  if (!strcmp(argv[2], "!="))
   return (strcmp(argv[1], argv[3]))?0:1;
  
  /*
   * Numeric comparisons, should also be obvious.
   * Note that results must be INVERTED!
   */
  if (!strcmp(argv[2], "-eq"))
   return !((num_or_die(argv[1]) == num_or_die(argv[3])));
  if (!strcmp(argv[2], "-ne"))
   return !((num_or_die(argv[1]) != num_or_die(argv[3])));
  if (!strcmp(argv[2], "-gt"))
   return !((num_or_die(argv[1]) >  num_or_die(argv[3])));
  if (!strcmp(argv[2], "-ge"))
   return !((num_or_die(argv[1]) >= num_or_die(argv[3])));
  if (!strcmp(argv[2], "-lt"))
   return !((num_or_die(argv[1]) <  num_or_die(argv[3])));
  if (!strcmp(argv[2], "-le"))
   return !((num_or_die(argv[1]) <= num_or_die(argv[3])));
 }

 /* ! - reverse everything following. */
 if (!strcmp(argv[1], "!"))
 {
  argv++;
  argc--;
  t=do_test(argc, argv);
  if (t==2) return t; else return !t;
 }

 /*
  * Exist: do a stat(2) just to check for the presence of the file, and return
  * false if we get back ENOENT (no such file or directory), true otherwise.
  *
  * An error condition other than ENOENT is still a case of the file existing,
  * so still return true.
  */
 if (!strcmp(argv[1], "-e"))
 {
  errno=0;
  if (argc!=3) return 0;
  e=stat(argv[2], &statbuf);
  return (errno==ENOENT)?1:0;
 }

 /*
  * Size: do a stat(2) to check for the size of the file, and return true if
  * non-zero, false otherwise.
  */
 if (!strcmp(argv[1], "-s"))
 {
  if (argc!=3) return 0;
  if (stat(argv[2], &statbuf)) return 1;
  return (statbuf.st_size)?0:1;
 }

 /* File type tests */
 if (!strcmp(argv[1], "-b")) /* Block device */
 {
  if (argc!=3) return 0;
  if (stat(argv[2], &statbuf)) return 1;
  return (statbuf.st_mode & S_IFBLK)?0:1;
 }

 if (!strcmp(argv[1], "-c")) /* Character device */
 {
  if (argc!=3) return 0;
  if (stat(argv[2], &statbuf)) return 1;
  return (statbuf.st_mode & S_IFCHR)?0:1;
 }

 if (!strcmp(argv[1], "-d")) /* Directory */
 {
  if (argc!=3) return 0;
  if (stat(argv[2], &statbuf)) return 1;
  return (statbuf.st_mode & S_IFDIR)?0:1;
 }

 if (!strcmp(argv[1], "-f")) /* File, in the specific sense */
 {
  if (argc!=3) return 0;
  if (stat(argv[2], &statbuf)) return 1;
  return (statbuf.st_mode & S_IFREG)?0:1;
 }

 if (!strcmp(argv[1], "-h")) /* Link (4.4BSD switch); also Posix */
 {
  if (argc!=3) return 0;
  e=lstat(argv[2], &statbuf);
  if (e) return 1;
  return (statbuf.st_mode & S_IFLNK)?0:1;
 }

 if (!strcmp(argv[1], "-L")) /* Link (Solaris switch); also Posix */
 {
  if (argc!=3) return 0;
  e=lstat(argv[2], &statbuf);
  if (e) return 1;
  return (statbuf.st_mode & S_IFLNK)?0:1;
 }

 if (!strcmp(argv[1], "-p")) /* Pipe */
 {
  if (argc!=3) return 0;
  if (stat(argv[2], &statbuf)) return 1;
  return (statbuf.st_mode & S_IFIFO)?0:1;
 }

 if (!strcmp(argv[1], "-S")) /* Socket */
 {
  if (argc!=3) return 0;
  if (stat(argv[2], &statbuf)) return 1;
  return (statbuf.st_mode & S_IFSOCK)?0:1;
 }

 /* File mode tests */
 if (!strcmp(argv[1], "-g")) /* SETGID */
 {
  if (argc!=3) return 0;
  if (stat(argv[2], &statbuf)) return 1;
  return (statbuf.st_mode & S_ISGID)?0:1;
 }

 if (!strcmp(argv[1], "-u")) /* SETUID */
 {
  if (argc!=3) return 0;
  if (stat(argv[2], &statbuf)) return 1;
  return (statbuf.st_mode & S_ISUID)?0:1;
 }

 if (!strcmp(argv[1], "-k")) /* Sticky (System III & V) */
 {
  if (argc!=3) return 0;
  if (stat(argv[2], &statbuf)) return 1;
  return (statbuf.st_mode & S_ISVTX)?0:1;
 }

 if (!strcmp(argv[1], "-r")) /* Readable */
 {
  if (argc!=3) return 0;
  errno=0;
  e=access(argv[2], R_OK);
  if (e)
  {
   if (errno!=EACCES) return 1;
   return 1;
  }
  return 0;
 }

 if (!strcmp(argv[1], "-w")) /* Writable */
 {
  if (argc!=3) return 0;
  errno=0;
  e=access(argv[2], W_OK);
  if (e)
  {
   if (errno!=EACCES) return 1;
   return 1;
  }
  return 0;
 }

 if (!strcmp(argv[1], "-x")) /* Executable */
 {
  if (argc!=3) return 0;
  errno=0;
  e=access(argv[2], X_OK);
  if (e)
  {
   if (errno!=EACCES) return 1;
   return 1;
  }
  return 0;
 }

 if (!strcmp(argv[1], "-t")) /* isatty */
 {
  if (argc!=3) derp();
  e=num_or_die(argv[2]);
  return !isatty(e);
 }

 if (!strcmp(argv[1], "-n")) /* String is non-null */
 {
  if (argc==2) return 1;
  return 0;
 }

 if (!strcmp(argv[1], "-z")) /* String is null */
 {
  if (argc==2) return 0;
  return 1;
 }

 /* Parentheses. */
 if (!strcmp(argv[1], "("))
 {
  int nl;

  /*
   * Look for a matching ).
   * Remember that there may be intervening parentheses, so keep a track of
   * the nesting level so we don't get caught in the parenthesisception.
   */
  nl=1;
  for (t=2; t<argc; t++)
  {
   if (!strcmp(argv[t], "(")) nl++;
   if (!strcmp(argv[t], ")")) nl--;

   /* We've hit nest level 0, so we've got what we came for. */
   if (!nl)
   {
    /* Nothing after this?  Just evaluate it as is */
    if (t==(argc-1))
    {
     argv++;
     argc-=2;
     return do_test(argc, argv);
    }

    /* No room for -a arg or -o arg, so it's not that.  Die screaming. */
    if (argc<(t+2))
     derp();

    /*
     * And/or boolean logic handlers.  There's a second set outside the
     * parentheses handler.
     *
     * Remember that for the shell, 0=true and 1=false, but for C, 1=true and
     * 0=false; therefore all comparisons have to run upside-down.
     * 
     * -a must come before -o.  These must be parsed left-dominant.
     */
    if (!strcmp(argv[t+1], "-a"))
    {
     int e1, e2, t2;

     /* Fold the whole left side. */
     nargc=t-1;
     nargv=calloc(nargc, sizeof(char *));
     if (!nargv) scram();
     nargv[0]=argv[0];
     for (t2=1; t2<nargc; t2++) nargv[t2]=argv[t2+1];
     e1=do_test(nargc, nargv);
     free(nargv);

     /* Now fold the right side. */
     nargc=argc-(t+1);
     nargv=calloc(nargc, sizeof(char *));
     if (!nargv) scram();
     nargv[0]=argv[0];
     for (t2=1; t2<nargc; t2++) nargv[t2]=argv[t2+(t+1)];
     e2=do_test(nargc, nargv);
     free(nargv);

     return !((!e1)&&(!e2));
    }

    if (!strcmp(argv[t+1], "-o"))
    {
     int e1, e2, t2;

     nargc=t-1;
     nargv=calloc(nargc, sizeof(char *));
     if (!nargv) scram();
     nargv[0]=argv[0];
     for (t2=1; t2<nargc; t2++) nargv[t2]=argv[t2+1];
     e1=do_test(nargc, nargv);
     free(nargv);

     nargc=argc-(t+1);
     nargv=calloc(nargc, sizeof(char *));
     if (!nargv) scram();
     nargv[0]=argv[0];
     for (t2=1; t2<nargc; t2++) nargv[t2]=argv[t2+(t+1)];
     e2=do_test(nargc, nargv);
     free(nargv);

     return !((!e1)||(!e2));
    }

    /* We can't understand it, so die screaming. */
    derp();
   }
  }

  /* We hit the end, didn't find a closing parenthesis.  Die screaming. */
  fprintf (stderr, "%s: missing )\n", progname);
  exit(2);
 }

 /* Ands and ors, if no parentheses. */
 for (t=2; t<argc; t++)
 {
  if (!strcmp(argv[t], "-a"))
  {
   int e1, e2, t2;

   nargc=t;
   nargv=calloc(nargc, sizeof(char *));
   if (!nargv) scram();
   nargv[0]=argv[0];
   for (t2=1; t2<nargc; t2++) nargv[t2]=argv[t2];
   e1=do_test(nargc, nargv);
   free(nargv);

   nargc=argc-t;
   nargv=calloc(nargc, sizeof(char *));
   if (!nargv) scram();
   nargv[0]=argv[0];
   for (t2=1; t2<nargc; t2++) nargv[t2]=argv[t2+t];
   e2=do_test(nargc, nargv);
   free(nargv);

   return !((!e1)&&(!e2));
  }

  if (!strcmp(argv[t], "-o"))
  {
   int e1, e2, t2;

   nargc=t;
   nargv=calloc(nargc, sizeof(char *));
   if (!nargv) scram();
   nargv[0]=argv[0];
   for (t2=1; t2<nargc; t2++) nargv[t2]=argv[t2];
   e1=do_test(nargc, nargv);
   free(nargv);

   nargc=argc-t;
   nargv=calloc(nargc, sizeof(char *));
   if (!nargv) scram();
   nargv[0]=argv[0];
   for (t2=1; t2<nargc; t2++) nargv[t2]=argv[t2+t];
   e2=do_test(nargc, nargv);
   free(nargv);

   return !((!e1)&&(!e2));
  }
 }

 /* Huh? */
 return 1; /* observed in bash */
}

/*
 * Set progname correctly.
 * If we are invoked as [, make sure the last argument is ], then eat it.
 * Whatever the invocation, chain into the entry point for the actual main
 * function.
 */
int main (int argc, char **argv)
{
 progname=strrchr(*argv, '/');
 if (progname) progname++; else progname=*argv;

 if (!strcmp(progname, "["))
 {
  if (strcmp(argv[argc-1], "]"))
  {
   fprintf (stderr, "[: missing ]\n");
   return 2;
  }
  argc--; /* Chomp the ] */
 }

 return do_test (argc, argv);
}
