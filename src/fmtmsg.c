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
 * The necessary library support is currently lacking on OpenBSD.
 * It should build anywhere else with a reasonably Posix libc.
 */

/*
 * -c class
 *    hard soft firm
 * -u subclass
 *    Multiple allowed with commas
 *    appl util opsys recov nrecov print console
 *    "appl util opsys" mutually exclusive
 *    "recov nrecov" mutually exclusive
 *    "print console" NOT mutually exclusive.
 * -l label
 * -s severity
 *    halt error warn info
 * -t tag
 * -a action
 *    each prefixed by "TO FIX:"
 *
 *    text (one argument only)
 *
 * Exit code:
 *
 *   0   no error
 *   1   syntax error
 *   2   no output on stderr
 *   4   no output on console
 *   32  failure
 *
 * The function, in fmtmsg.h:
 *    int fmtmsg(long class, const char *label, int severity,
 *               const char *text, const char *action, const char *tag);
 *
 */

#include <errno.h>
#include <fmtmsg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

void usage (void)
{
 fprintf (stderr,
          "%s: usage: %s -a action -c class -l label [-s severity] -t tag\n"
          "                      -u subclass[,subclass...] text\n",
          progname, progname);
 exit(1);
}

/*
 * Generate the appropriate parameter for fmtmsg(3) from the class and
 * subclass options received on the command line.  If any of them fail, then
 * die screaming.
 */
long mkclass (char *class, char *subclass)
{
 char *d;
 char *p, *q;
 long t, tt;

 /* Valid classes. One must be specified (checked by main()). */
 t=0;
 if (!strcmp(class, "hard"))
  t=MM_HARD;
 else if (!strcmp(class, "soft"))
  t=MM_SOFT;
 else if (!strcmp(class, "firm"))
  t=MM_FIRM;
 else
 {
  fprintf(stderr, "%s: invalid class '%s'\n", progname, class);
  exit(1);
 }

 /*
  * Copy the subclass string so we can dink it.
  * Thereafter, work on the copied string, not the original.
  */
 d=strdup(subclass);
 if (!d)
 {
  fprintf(stderr, "%s: out of memory\n", progname);
  exit(32);
 }

 p=d;
 while (*p)
 {
  /*
   * If there is another token, then change the delimiter to a string
   * terminator and move the "next token" pointer appropriately; otherwise,
   * put it at the end of the string.
   */
  q=strchr(p, ',');
  if (q)
   *(q++)=0;
  else
   q=p+strlen(p);

  /*
   * Parse the token, or if we don't understand it, free the copied string,
   * and die screaming.
   */
  if (!strcmp(p, "appl"))
   t|=MM_APPL;
  else if (!strcmp(p, "util"))
   t|=MM_UTIL;
  else if (!strcmp(p, "opsys"))
   t|=MM_OPSYS;
  else if (!strcmp(p, "recov"))
   t|=MM_RECOVER;
  else if (!strcmp(p, "nrecov"))
   t|=MM_NRECOV;
  else if (!strcmp(p, "print"))
   t|=MM_PRINT;
  else if (!strcmp(p, "console"))
   t|=MM_CONSOLE;
  else
  {
   fprintf (stderr, "%s: invalid subclass '%s'\n", progname, p);
   free(d);
   exit(1);
  }

  /* Set the iterative pointer to the next token. */
  p=q;
 }
 /* Free the copiedstring. */
 free(d);

 /* Check for mutually exclusive options. */
 tt=(t&(MM_APPL|MM_UTIL|MM_OPSYS));
 if (tt&&((tt!=MM_APPL)&&(tt!=MM_UTIL)&&(tt!=MM_OPSYS)))
 {
  fprintf (stderr, "%s: 'appl', 'util' and 'opsys' are mutually exclusive\n",
           progname);
  exit(1);
 }
 tt=(t&(MM_RECOVER|MM_NRECOV));
 if (tt==(MM_RECOVER|MM_NRECOV))
 {
  fprintf (stderr, "%s: 'recov' and 'nrecov' are mutually exclusive\n",
           progname);
  exit(1);
 }

 /* We got what we came for.  Return the code we calculated. */
 return t;
}

/*
 * Generate the appropriate parameter for fmtmsg(3) from the severity option
 * received on the command line (or set it to 0 if none is provided).
 *
 * Severities can be either numeric (unsigned) or textual, so check for both.
 * Die screaming if the result is invalid.
 */
int mkseverity (char *severity)
{
 int x;

 errno=0;
 if ((*severity>='0')&&(*severity<='9'))
 {
  x=strtol(severity, 0, 0);
  if (errno)
  {
   fprintf (stderr, "%s: invalid severity '%s'\n", progname, severity);
   exit(1);
  }
 }
 else
 {
  if (!strcmp(severity, "halt"))
   x=MM_HALT;
  else if (!strcmp(severity, "error"))
   x=MM_ERROR;
  else if (!strcmp(severity, "warn"))
   x=MM_WARNING;
  else if (!strcmp(severity, "info"))
   x=MM_INFO;
  else
  {
   fprintf (stderr, "%s: invalid severity '%s'\n", progname, severity);
   exit(1);
  }
 }

 return x;
}

int main (int argc, char **argv)
{
 int e;
 char *got_a, *got_c, *got_s, *got_u, *got_l, *got_t;

 /* Get the basename of the command. */
 progname=strrchr(*argv, '/');
 if (progname) progname++; else progname=*argv;

 /* Set defaults, then iterate over the command line. */
 got_a=got_c=got_s=got_u=got_l=got_t=NULL;
 while (-1!=(e=getopt(argc, argv, "a:c:l:s:t:u:")))
 {
  switch (e)
  {
   case 'a':
    got_a=optarg;
    break;
   case 'c':
    got_c=optarg;
    break;
   case 's':
    got_s=optarg;
    break;
   case 'u':
    got_u=optarg;
    break;
   case 'l':
    got_l=optarg;
    break;
   case 't':
    got_t=optarg;
    break;
  }
 }

 /* Make sure we got everything we need. */
 if (!got_s)
  got_s="0";
 if ((!got_a)||(!got_c)||(!got_u)||(!got_l)||(!got_t))
  usage();
 if ((argc-optind)!=1)
  usage();

 /* Do it, maggot. */
 e=fmtmsg(mkclass(got_c, got_u), got_l, mkseverity(got_s),
          argv[optind], got_a, got_t);

 /* Return appropriate error code. */
 switch (e)
 {
  case MM_OK:
   return 1;
  case MM_NOMSG:
   return 2;
  case MM_NOCON:
   return 4;
  default:
   return 32;
 }
}
