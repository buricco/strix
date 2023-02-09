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
 * mknod.
 *
 * mknod filename b MAJ MIN (S_IFBLK)
 * mknod filename c MAJ MIN (S_IFCHR)
 * mknod filename p         (S_IFIFO)
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __GLIBC__ /* ( n O.O b ) */
#include <sys/sysmacros.h>
#endif

#ifdef __SVR4__
#include <sys/mkdev.h>
#endif

static char *copyright="@(#) (C) Copyright 2022, 2023 S. V. Nickolas\n";

static char *progname;

void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s name {b | c} major minor\n"
                  "%s: usage: %s name p\n",
                  progname, progname, progname, progname);
 exit(2);
}

int main (int argc, char **argv)
{
 mode_t mode;
 dev_t dev;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if ((argc!=3)&&(argc!=5)) usage();
 if ((argc==5)&&(!strcmp(argv[2], "p"))) usage();
 if ((argc==3)&&(strcmp(argv[2], "p"))) usage();
 if (strcmp(argv[2],"b")&&strcmp(argv[2],"c")&&strcmp(argv[2],"p")) usage();

 if (*(argv[2])=='p')
 {
  mode=S_IFIFO;
  dev=0;
 }
 else
 {
  if (*(argv[2])=='b') mode=S_IFBLK;
  if (*(argv[2])=='c') mode=S_IFCHR;
  dev=makedev(atoi(argv[3]), atoi(argv[4]));
 }

 if (mknod(argv[1], mode, dev))
 {
  xperror(argv[1]);
  return 2;
 }

 return 0;
}
