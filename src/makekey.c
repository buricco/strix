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

#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * Needs -lcrypt, at least on Linux.
 */
#if defined(__linux__)||defined(__SVR4__)
#include <crypt.h>
#else
#include <unistd.h>
#endif

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

/*
 * Read 8 to "key"
 * read 2 to "salt".
 * run crypt(key,salt) on that.
 */

int main (int argc, char **argv)
{
 char *progname;
 char *p;
 char key[8], salt[2];

 /*
  * Process the name of the program.
  * A BSD-style libc actually will do this for us and return it in __progname.
  */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (argc!=1)
 {
  fprintf (stderr, "%s: usage: %s\n", progname, progname);
  return 1;
 }

 if (fread(key,1,8,stdin)!=8)
 {
  fprintf (stderr, "%s: stdin: short read key: %s\n",
           progname, strerror(errno));
  return 1;
 }

 if (fread(salt,1,2,stdin)!=2)
 {
  fprintf (stderr, "%s: stdin: short read salt: %s\n",
           progname, strerror(errno));
  return 1;
 }

 p=crypt(key, salt);
 if (fwrite(p, 1, strlen(p), stdout)!=strlen(p))
 {
  fprintf (stderr, "%s: stdout short write: %s\n", progname, strerror(errno));
  return 1;
 }

 return 0;
}
