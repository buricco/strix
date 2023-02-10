/*
 * (C) Copyright 2023 S. V. Nickolas.
 * 
 * This code is a partial rewrite and contains elements of the version placed
 * into the public domain in December 1983 by Mike Muuss.
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
 * There is a man page, but I have never seen binaries for this.
 * Like link and unlink, it is only a basic wrapper around the libc function.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

int main (int argc, char **argv)
{
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 
 if (argc!=3)
 {
  fprintf (stderr, "%s: usage: %s oldname newname\n", progname, progname);
  return 1;
 }
 
 if (rename(argv[1], argv[2]))
 {
  fprintf (stderr, "%s: rename %s to %s: %s\n", 
           progname, argv[1], argv[2], strerror(errno));
  return 1;
 }
 
 return 0;
}
