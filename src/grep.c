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

/*
 * This is a simple implementation of grep(1), modelled on System V Unix.
 *
 * It has only received minimal testing, but basic functionality works.  Very
 * little beyond the minimal POSIX required functionality is present.  The
 * bones of the implementation (failing the fgrep portion) use <regex.h>, the
 * POSIX regular expression library, so this is mostly just a wrapper around
 * that.  This should (but probably doesn't quite) conform to POSIX.1-2017.
 * Additionally a few particularly common and simple extensions are added.
 *
 *   -E - Act as if invoked as "egrep" and use enhanced regex mode.
 *   -F - Act as if invoked as "fgrep" and do not use regexs at all.
 *   -H - Show filenames, even if only one file is specified.  (GNU/BSDism)
 *   -R - Recurse into directories, following symbolic links.  (GNUism)
 *   -b - Display "block number" before each matching (or mismatching) line.
 *        (from V6, kept in AT&T's grep and still supported by SVR5)
 *        NOTE: This does not always display the same "block number" as the
 *              actual AT&T code; the actual means by which this is computed
 *              on AT&T's version is uncertain but this code attempts to
 *              approximate it.
 *   -c - Display only a count of lines that match one or more patterns
 *        (or with -v, that match no patterns).
 *   -e - Following argument is a pattern.
 *   -f - Following argument is a file containing patterns.
 *   -h - Do not show filenames, even if more than one file is specified.
 *        (Common, but not part of POSIX)
 *   -i - Ignore case when matching patterns.
 *   -l - Display only the names of files that contain at least one match
 *        (or with -v, that contain no matches).
 *   -n - Display line numbers before each matching (or mismatching) line.
 *   -q - Do not display anything.  Only return an exit code.
 *   -r - Recurse into directories; do not follow symbolic links.  (GNUism)
 *   -s - Suppress open or read errors but otherwise act normally.
 *   -v - Reverse match; show mismatching lines instead of matching lines
 *        (or with -l, files with no matches instead of files with matches).
 *   -x - Match only against the whole line, instead of part of it.
 *
 * (Undocumented: -L is treated as -lv.)
 *
 * (Note that, especially with fgrep, the order in which patterns are matched
 *  may not be as you expect; but this is irrelevant as this matching is fully
 *  commutative.)
 *
 * Exit codes:
 *
 *   0  - All files scanned contained at least one line matching the given
 *        criteria.
 *   1  - At least one file scanned contained no lines matching the given
 *        criteria.
 *   2  - An error (of any type) occurred.
 *
 * The specific priority of exit codes depends on whether -q is specified.
 * 0 takes priority over 1, but with -q specified, it also takes priority over
 * 2.  The final code is accumulated.
 *
 * If no patterns are specified by -e or -f, the next parameter is treated as
 * the pattern, and if none is specified here, it is an error condition.
 *
 * Additional parameters are treated as files to scan.  If no files are
 * specified, stdin is scanned.  If the special filename "-" is provided, it
 * is treated as stdin.
 *
 * All of the above behavior is as defined by POSIX, except as marked.
 *
 * Additionally, if this program is invoked as "egrep" or "fgrep", it acts as
 * if it were invoked with the -E respectively -F switch, and the usage
 * diagnostic is updated to not display them.  This is consistent with
 * historic behavior.
 *
 * POSIX specifies that grep can safely assume that all files are text files.
 * The GNU and BSD versions attempt to sanely deal with binary files; this is
 * an extension (and sometimes the detection fails).  We opted not to bother.
 *
 * In several places the code assumes a very braindead compiler (even though
 * on Debian 11, both gcc and clang compile this code with no warnings).  The
 * original author of this code has gained a propensity for assuming the worst
 * in C compiler braindeath from personal experience.
 *
 * Limits:
 *
 *   * Because we cannot know in advance how many times the -e switch will be
 *     used, or how many lines may appear in a file described by the -f
 *     switch, it is possible to overload the check list.  This limit is
 *     configurable at compile time by altering the value of PATTERN_STACK
 *     (default: 128).
 *
 * Other implementation details:
 *
 *   * As with some traditional versions, this program watches for invocation
 *     as egrep or fgrep.  This behavior is officially deprecated, but I see
 *     no reason not to implement it, given how little code it requires and
 *     that it has been for years the preferred method.
 *   * The regex library takes care of the -Eix switches (except in -F mode).
 *   * The diagnostic format, which prepends the name by which the command was
 *     invoked to all error and warning conditions, is consistent with my
 *     other Unix-clone utilities.
 *   * Lines are read with getline(), so any limits in line length come from
 *     there.  The maximum should be significantly higher than the original
 *     256 bytes or the SVR4 limit of 1K.
 *   * Because the files are read in strict ASCII mode and no attempt is made
 *     to preserve context, it is not possible to implement the BSD/GNU -ABC
 *     extensions, and the -DIUZdz switches do not make sense.
 *   * The BSD/GNU -L switch is implemented by acting like the -lv switch was
 *     specified.  As in the BSD and GNU versions, we simply drop out when one
 *     match is found as a speed optimization.
 */

#ifdef linux
#define _XOPEN_SOURCE 500
#define _XOPEN_SOURCE_EXTENDED 1
#define _GNU_SOURCE 1
#endif
#include <ftw.h>

#include <errno.h>         /* used by xperror() */
#include <regex.h>         /* all the documented functions are used here */
#include <stdio.h>
#include <stdlib.h>        /* used by exit() */
#include <string.h>
#include <unistd.h>        /* used by getopt() */

/*
 * Limit of pattern count, because a table of patterns and regexs is
 * statically allocated.
 *
 * Can be overridden at the cc command line without altering the code.
 */
#ifndef PATTERN_STACK
#define PATTERN_STACK 128
#endif
int patstack;

/* Maximum handles for nftw(3) */
#ifndef ITERATE_MAX_HANDLES
#define ITERATE_MAX_HANDLES 5
#endif

/*
 * Flags for switches and whether program was invoked as egrep or fgrep
 * (i.e., to determine how to display usage if there is a syntax error).
 */
                              /********************************************/
#define  IS_EGREP  0x1000     /* Use extended regex mode                  */
#define  IS_FGREP  0x0800     /* Do not use regexs at all                 */
#define  FLAG_R_L  0x0400     /* Recurse (logical mode, -R)               */
#define  FLAG_R_P  0x0200     /* Recurse (physical mode, -r)              */
#define  FLAG_B    0x0100     /* Block number (byte offset/512 plus 1)    */
#define  FLAG_C    0x0080     /* Only show a count of matches             */
#define  FLAG_L    0x0040     /* Only display the names of matching files */
#define  FLAG_Q    0x0020     /* Display nothing                          */
#define  FLAG_I    0x0010     /* Case-insensitive search                  */
#define  FLAG_N    0x0008     /* Number lines                             */
#define  FLAG_S    0x0004     /* Suppress open/read errors                */
#define  FLAG_V    0x0002     /* Invert matches                           */
#define  FLAG_X    0x0001     /* No substring match                       */
unsigned mode, usagemode;     /********************************************/

/*
 * Catches a very specific error condition where -f is used and no patterns
 * are read.
 */
int nulpat;

/*
 * Flags for multifile mode.
 *
 * Usually, if one file is specified, don't show the filename on each match,
 * and if more than one file is specified, show the filename on each match.
 * There are switches to force one or the other behavior (one is a GNUism
 * which the BSD version picked up; the other is a System Vism which both
 * GNU and BSD picked up).
 */
                              /********************************************/
#define  MULFIL_FN 0x04       /* Force "no" (changes to 0 after parsing)  */
#define  MULFIL_FY 0x02       /* Force "yes" (changes to 1 after parsing) */
#define  MULFIL_Y  0x01       /* "Yes" (nonzero for quick comparison)     */
#define  MULFIL_N  0x00       /* "No" (zero for quick comparison)         */
int mulfil;                   /********************************************/

char *patterntable[PATTERN_STACK];
regex_t regextable[PATTERN_STACK];

static char *copyright="@(#) (C) Copyright 2022, 2023 S. V. Nickolas\n";

static char *progname;

static int grep_iterative_cumul;
static int go_fish;

/*
 * On SVR4, these are not part of the base libc (they're in libucb, which is
 * known to have issues, and in some versions is infamously coderotted).
 * Instead, provide our own.  (The code for regex comes from 4.4BSD and is
 * built and linked separately.)
 */
#ifdef __SVR4__
#include <ctype.h>
#define strcasecmp  int_strcasecmp
#define strncasecmp int_strncasecmp

static int int_strcasecmp (const char *a, const char *b)
{
 char *aa, *bb;

 aa=(char *)a; bb=(char *)b;
 while (toupper(*aa++)==toupper(*bb++))
  if (!*aa) return 0; /* Match. */
 bb--;
 return *aa-*bb;
}

static int int_strncasecmp (const char *a, const char *b, size_t n)
{
 char *aa, *bb;

 aa=(char *)a; bb=(char *)b;
 while ((--n)&&(toupper(*aa++)==toupper(*bb++)))
  if (!*aa) return 0; /* Match. */
 bb--;
 return *aa-*bb;
}
#endif

/*
 * Equivalent to GNU's version.
 *
 * In the case that we are using fgrep mode, -i is specified and -x is not,
 * we need to be able to find a substring without case sensitivity.  This
 * method should work well enough though probably far from the most efficient.
 */
char *int_strcasestr (char *str, char *what)
{
 int z=0;

 while (strlen(str)>=strlen(what))
 {
  if (!strncasecmp(str, what, strlen(what)))
  {
   z=1;
   break;
  }
  str++;
 }

 return z?str:0;
}

/*
 * Modified version of perror(3) that shows the program name
 * (as uso believes a *x app should do with all diagnostics).
 */
void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

/* Method of dying screaming when the Grim Memory Reaper comes to call. */
void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(2);
}

/* The meat of the program. */
int do_grep (char *filename)
{
 int r, t;
 char *realname;
 char *curlin;
 int is_stdin;
 FILE *file;
 long e;
 unsigned long l, x, c, bn;

 l=c=0;

 r=1;

 is_stdin=0;

 /*
  * Treat a filename of "-" as referring to stdin.
  *
  * Note that POSIX specifies that the file is to be treated as text in all
  * cases (some versions will special-case a file if they think it is binary).
  *
  * As for the string "(standard input)", I would prefer "(stdin)", but the
  * standard specifies exact verbiage here.
  */
 if (*filename && strcmp(filename, "-"))
 {
  realname=filename;
  file=fopen(filename, "rt");
  if (!file)
  {
   if (!(mode&(FLAG_Q|FLAG_S))) xperror (filename);
   return 2;
  }
 }
 else
 {
  is_stdin=1;
  realname="(standard input)"; /* POSIX me harder */
  file=stdin;
  rewind(file);
 }

 curlin=0;
 while (1)
 {
  l++;
  x=0;
  if (curlin) free (curlin);
  e=getline(&curlin, &x, file);
  bn=((ftell(file)+1)>>9);
  if (e==-1)
  {
   if (!((mode&(FLAG_Q|FLAG_S))||feof(file))) xperror(realname);
   break;
  }
  else
  {
   if (curlin[strlen(curlin)-1]=='\n') curlin[strlen(curlin)-1]=0;

   /*
    * Modes:
    *   * fgrep: use strstr / strcasestr (-i)
    *                !strcmp (-x) / !strcasecmp (-ix)
    *   * otherwise: !regexec
    *
    * Invert the result if -v supplied (but do it differently for -l!)
    */
   if (mode&IS_FGREP)
   {
    e=0;

    for (t=0; t<patstack; t++)
    {
     if (mode&FLAG_X)
     {
      if (!(((mode&FLAG_I)?strcasecmp:strcmp)(curlin, patterntable[t]))) e=1;
     }
     else
     {
      if (mode&FLAG_I)
      {
       if (int_strcasestr(curlin, patterntable[t])) e=1;
      }
      else
      {
       if (strstr(curlin, patterntable[t])) e=1;
      }
     }
    }
   }
   else
   {
    e=0;

    for (t=0; t<patstack; t++)
     if (!regexec(&regextable[t], curlin, 0, 0, 0)) e=1;
   }
   if ((mode&FLAG_V)&&(!(mode&FLAG_L))) e=!e;

   /*
    * Display matching line, if appropriate.
    * (POSIX specifies %d, but it should really be %u as this is an unsigned
    *  value. Herp de derp.)
    */
   if (e)
   {
    c++;
    if (r==1) r=0;

    if (!(mode&(FLAG_C|FLAG_Q|FLAG_L)))
    {
     if (mulfil) printf ("%s:", realname);
     if (mode&FLAG_B) printf ("%lu:", bn);
     if (mode&FLAG_N) printf ("%lu:", l);
     printf ("%s\n", curlin);
    }
   }

   /*
    * Speed optimization.
    *
    * This technique is documented in at least one other version:  If we are
    * just looking for *if* a file matches, we only have to find one match to
    * know if it does or not.  (This, regardless of the presence of the -v
    * switch.)
    */
   if ((mode&FLAG_L)&&e) break;
  }
 }

 /* Close the file.  If stdin, just reset the stream. */
 if (is_stdin) rewind(file); else fclose(file);

 /*
  * If -l was set, then we did not reverse the "lines found" flag even if -v
  * was set, so do it now.
  */
 if (((mode&(FLAG_V|FLAG_L))==(FLAG_V|FLAG_L)) && (r<2)) r=!r;

 /*
  * For -c, display the sum total of lines found.
  * (Behavior has been tested and matches GNU grep.)
  */
 if (mode&FLAG_C)
 {
  if (mulfil) printf ("%s:", realname);
  printf ("%lu\n", c);
 }

 /*
  * For -l, display the names of any files containing matching lines
  * (or, with -v, no matching lines).
  *
  * This is the reason -lv has to be special-cased.
  */
 if ((!r) && (mode&FLAG_L)) printf ("%s\n", realname);

 /* Return exit code. */
 return r;
}

/* Used for -R.  Callback from nftw(). */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
int iterate_hit (const char *filename, const struct stat *statptr,
                 int fileflags, struct FTW *pftw)
{
 if (fileflags == FTW_F)
 {
  int r;

  /* Mark we found something. */
  go_fish=0;
  
  /* Do the grep and mark if there was an error. */
  r=do_grep((char *)filename);
  if (r>grep_iterative_cumul) grep_iterative_cumul=r;
 }

 return 0;
}
#pragma clang diagnostic pop

/* Used for -R.  Handles iteration into folders. */
int iterative_grep (char *filename, int flags)
{
 grep_iterative_cumul=0;
 
 /* stdin pseudofile. */
 if (!strcmp(filename, "-")) return do_grep(filename);

 /* Set default to "we didn't find anything", then start searching. */
 go_fish=1;
 if (nftw(filename, iterate_hit, ITERATE_MAX_HANDLES, flags)==-1)
 {
  xperror(filename);
  return 2;
 }

 /* If we didn't find anything, say "No such file or directory" */
 if (go_fish)
 {
  fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(ENOENT));
  grep_iterative_cumul=2;
 }
 return grep_iterative_cumul;
}

/*
 * Method of dying screaming if a syntax error is detected.
 *
 * Spacing of the last line assumes argv[0] is 4 chars, 5 for egrep/fgrep.
 * (This function has long lines.)
 */
void usage (void)
{
 if (!(mode&(IS_EGREP|IS_FGREP)))
 {
  fprintf(stderr, "%s: usage: %s [-E|-F] [-c|-l|-q] [-HRbhinrsvx] pattern [file ...]\n"
                  "%s: usage: %s [-E|-F] [-c|-l|-q] [-HRbhinrsvx]\n"
                  "                  {-e pattern | -f patternfile} [...] [file ...]\n",
                  progname, progname, progname, progname);
 }
 else
 {
  fprintf(stderr, "%s: usage: %s [-c|-l|-q] [-HRbhinrsvx] pattern [file ...]\n"
                  "%s: usage: %s [-c|-l|-q] [-HRbhinrsvx]\n"
                  "                    {-e pattern | -f patternfile} [...] [file ...]\n",
                  progname, progname, progname, progname);
 }
 exit(2);
}

/* Method of dying screaming if conflicting options are specified. */
void exclusive (char *args)
{
 fprintf (stderr, "%s: switches %s are mutually exclusive\n", progname, args);
 exit(2);
}

/*
 * Entry point.
 * Resolve the command line, and act accordingly.
 */
int main (int argc, char **argv)
{
 int e, r, t;

 mulfil=mode=usagemode=patstack=0;

 /*
  * Find whether we were invoked as "egrep" or "fgrep".
  *
  * usagemode is set here so that if there is a syntax error, we get specific
  * usage directions for grep, egrep and fgrep.  We do not want egrep/fgrep
  * instructions for grep -E or grep -F.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 if (!strcmp(progname, "egrep")) mode = usagemode = IS_EGREP;
 if (!strcmp(progname, "fgrep")) mode = usagemode = IS_FGREP;

 /*
  * Evaluate options.
  * If conflicting options are specified, die screaming.
  * If -E or -F is specified when in egrep/fgrep mode, die screaming.
  */
 while (-1!=(e=getopt(argc, argv, "EFHLRbce:f:hlinqrsvx")))
 {
  switch (e)
  {
   case 'E':
    if (usagemode) usage();
    mode |= IS_EGREP;
    if ((mode&(IS_EGREP|IS_FGREP))==(IS_EGREP|IS_FGREP))
     exclusive("-E and -F");
    break;
   case 'F':
    if (usagemode) usage();
    mode |= IS_FGREP;
    if ((mode&(IS_EGREP|IS_FGREP))==(IS_EGREP|IS_FGREP))
     exclusive("-E and -F");
    break;
   case 'H':
    if (mulfil) exclusive("-H and -h");
    mulfil = MULFIL_FY;
    break;
   case 'L':
    mode |= (FLAG_L|FLAG_V);
    if ((mode&(FLAG_C|FLAG_L|FLAG_Q))!=FLAG_L) exclusive("-c, -l and -q");
    break;
   case 'R':
    mulfil = MULFIL_FY;
    mode |= FLAG_R_L;
    break;
   case 'b':
    mode |= FLAG_B;
    break;
   case 'c':
    mode |= FLAG_C;
    if ((mode&(FLAG_C|FLAG_L|FLAG_Q))!=FLAG_C) exclusive("-c, -l and -q");
    break;
   case 'e':
    patterntable[patstack]=strdup(optarg);
    if (!patterntable[patstack]) scram();
    patstack++;
    if (patstack>=PATTERN_STACK) scram();
    break;
   case 'f':
   {
    FILE *file;

    nulpat=1;

    file=fopen(optarg, "rt");
    if (!file)
    {
     xperror(optarg);
     return 2;
    }

    while (1)
    {
     size_t l;
     ssize_t n;

     n=getline(&(patterntable[patstack]), &l, file);
     if (n<0)
     {
      fclose(file);
      
      /*
       * Also sprach Posix:
       *   "A null pattern can be specified by an empty line in pattern_file"
       * 
       * Also sprach Posix:
       *   "the pattern_file's contents shall consist of one or more patterns
       *    terminated by a <newline> character"
       * 
       * NetBSD says "the behaviour of the -f flag when using an empty pattern
       * file is left undefined", but the standard does say "one or more", and
       * zero isn't "one or more", so I would argue that zero lines in the
       * pattern file is an error condition.
       */
      if (nulpat)
      {
       fprintf (stderr, "%s: %s: empty pattern file\n", progname, optarg);
       return 2;
      }
      break;
     }
     nulpat=0;
     
     /* Strip off the newline. */
     if (patterntable[patstack][strlen(patterntable[patstack])-1]=='\n')
      patterntable[patstack][strlen(patterntable[patstack])-1]=0;
     
     patstack++;
     if (patstack>=PATTERN_STACK) scram();
    }
    break;
   }
   case 'h':
    if (mulfil) exclusive("-H and -h");
    mulfil = MULFIL_FN;
    break;
   case 'l':
    mode |= FLAG_L;
    if ((mode&(FLAG_C|FLAG_L|FLAG_Q))!=FLAG_L) exclusive("-c, -l and -q");
    break;
   case 'i':
    mode |= FLAG_I;
    break;
   case 'n':
    mode |= FLAG_N;
    break;
   case 'q':
    mode |= FLAG_Q;
    if ((mode&(FLAG_C|FLAG_L|FLAG_Q))!=FLAG_Q) exclusive("-c, -l and -q");
    break;
   case 'r':
    mulfil = MULFIL_FY;
    mode |= FLAG_R_P;
    break;
   case 's':
    mode |= FLAG_S;
    break;
   case 'v':
    mode |= FLAG_V;
    break;
   case 'x':
    mode |= FLAG_X;
    break;
   default:
    usage();
  }
 }

 if ((mode&(FLAG_R_L|FLAG_R_P))==(FLAG_R_L|FLAG_R_P)) exclusive ("-r and -R");

 /*
  * No -e or -f switches were supplied, so treat the next argument as if it
  * were passed to -e.  If there is no next argument, die screaming.
  */
 if (!patstack)
 {
  if (argc==optind) usage();
  patterntable[0]=strdup(argv[optind++]);
  if (!patterntable[0]) scram();
  patstack=1;
 }

 /*
  * Because patstack may change, don't evaluate it right away
  * (assumes compiler is braindead and only evaluates it once).
  * The same kind of compiler braindeath is assumed in the internal loop.
  *
  * If running in fgrep mode, manually split patterns if separators are found.
  * Otherwise, precompile all the regexs.
  */
 if (mode & IS_FGREP)
 {
  for (t=0; ; t++)
  {
   if (t>=patstack) break;

   for (e=0; ; e++)
   {
    if (!patterntable[t][e]) break;

    if ((patterntable[t][e]=='|')||(patterntable[t][e]=='\n'))
    {
     char *p;

     p=&(patterntable[t][e]);
     *(p++)=0;
     if (patstack>=PATTERN_STACK) scram();
     patterntable[patstack]=strdup(p);
     patstack++;
    }
   }
  }
 }
 else
 {
  for (t=0; t<patstack; t++)
  {
   /*
    * Precompile the regexs, taking note of any flags we were passed that
    * might affect the compilation.  REG_EXTENDED here is the only difference
    * between running in grep(1) mode and running in egrep(1) mode.
    */
   e=regcomp(&regextable[t], patterntable[t], ((mode&IS_EGREP)?REG_EXTENDED:0)
                                             |((mode&FLAG_I)?REG_ICASE:0)
                                             |((mode&FLAG_X)?REG_NOSUB:0));
   /*
    * Die screaming if the regex precompilation failed.
    *
    * First get the length of the error message, then set aside that much
    * memory.  Then get the error message and display it.  Once we're done,
    * release the memory and die.
    */
   if (e)
   {
    char *es;
    size_t s;

    s=regerror(e, &regextable[t], 0, 0);
    es=malloc(s+1);
    if (!es) scram();
    regerror(e, &regextable[t], es, s);
    fprintf (stderr, "%s: %s\n", progname, es);
    free(es);
    return 2;
   }
  }
 }

 /*
  * Multifile mode: prefix appropriate lines with filenames.
  * Override with -H or -h.
  */
 if (mulfil&(MULFIL_FY|MULFIL_FN))
  mulfil=(mulfil==MULFIL_FY);
 else
  if ((argc-optind)>1) mulfil=MULFIL_Y;

 /*
  * Execute the actual grep operation, and aggregate our return code.
  *
  * If no file was specified, use stdin; else iterate over files specified.
  * Exit priority is 0>2>1 with -q; 2>0>1 without (as per POSIX).
  */
 r=1;
 if (argc==optind)
 {
  if (mode&FLAG_R_P)
   r=iterative_grep(".", FTW_PHYS);
  else if (mode&FLAG_R_L)
   r=iterative_grep(".", 0);
  else
   r=do_grep("-");
 }
 else for (t=optind; t<argc; t++)
 {
  if (mode&FLAG_R_P)
   e=iterative_grep(argv[optind], FTW_PHYS);
  else if (mode&FLAG_R_L)
   e=iterative_grep(argv[optind], 0);
  else
   e=do_grep(argv[optind]);
  if (mode&FLAG_Q)
  {
   if (!e) r=0;
  }
  else
  {
   if ((!e)&&(r==1))
    r=0;
   else
    if (e>r) r=e;
  }
 }

 /*
  * Free dynamically allocated memory.
  *
  * Except with fgrep (for obvious reasons) we could probably free the pattern
  * table BEFORE we execute, since we have already compiled it into a regex
  * table.  Are we bothered, though?
  */
 while (patstack)
 {
  free(patterntable[--patstack]);
  if (!(mode&IS_FGREP)) regfree(&regextable[patstack]);
 }

 /* Return exit code. */
 return r;
}
