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
 * mkfifo(filename, mode); (0 on OK)
 *
 * Use -lbsd on Linux for getmode() and setmode();
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__linux__)||defined(__SVR4__)
#include <setmode.h>
#endif

#define BASE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

mode_t mode;

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
 fprintf(stderr, "%s: usage: %s [-m mode] filename...\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 void *x;
 int e, t;

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 mode=BASE_MODE;

 while (-1!=(e=getopt(argc, argv, "m:")))
 {
  switch (e)
  {
   case 'm':
    x=setmode(optarg);
    if (!x)
    {
     xperror("failed to parse mode");
     return 1;
    }
    mode=getmode(x, mode);
    break;
   default:
    usage();
  }
 }

 if (optind==argc)
  usage();

 e=0;
 for (t=optind; t<argc; t++)
 {
  if (mkfifo(argv[t], mode))
  {
   e++;

   xperror(argv[t]);
  }
 }

 return e?1:0;
}
