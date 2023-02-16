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
 * sha2 will not be compiled on SVR4, so no provisions are made for it (e.g.,
 * workarounds for GCC 2.x, or lack of mkstemp(3)).
 * 
 * The program is called "sha2" but will only appear in the tree as "sha224",
 * "sha256", "sha384" and "sha512".
 */

/*
 * To use the librfc6234 functions:
 *   1. Reset the algorithm (SHAxxxReset()).
 *   2. Pump the filter with blocks of data (SHAxxxInput()).
 *      FinalBits isn't needed because we only operate on octets.
 *   3. Hit Result.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sha.h"

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

/* perror(3) with the name of the utility AND the name of the input file. */
void xperror (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";

 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
}

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

int do_sha224 (char *filename, int suppress)
{
 int t;
 SHA224Context myContext;
 uint8_t hash[SHA224HashSize];
 int c, e;
 FILE *file;
 char *slurp;
 size_t l;
 uint8_t mysha1[20];
 uint64_t lb;
 
 if (!strcmp(filename, "-"))
 {
  char foo[19];
  int h;

  /* 19 */
  strcpy(foo, "/var/tmp/shaXXXXXX");

  h=mkstemp(foo);
  if (h<0)
  {
   fprintf (stderr, "%s: could not open holding tank for stdin\n", progname);
   exit(1);
  }
  file=fdopen(h, "w");

  rewind(stdin);
  while ((c=getchar())>=0) fputc(c, file);
  fclose(file);
  e=do_sha224(foo, 1);
  unlink(foo);
  return e;
 }

 /* Slurp file */
 file=fopen(filename, "r");
 if (!file)
 {
  xperror(filename);
  return 1;
 }
 fseek(file, 0, SEEK_END);
 l=ftell(file);
 fseek(file, 0, SEEK_SET);
 slurp=malloc(l);
 if (!slurp) scram();
 if (fread(slurp, 1, l, file)<l)
 {
  free(slurp);
  perror(filename);
  return 1;
 }
 fclose(file);
 
 SHA224Reset(&myContext);
 SHA224Input(&myContext, slurp, l);
 SHA224Result(&myContext, hash);
 
 free(slurp);
 
 for (t=0; t<SHA224HashSize; t++) printf ("%02x", hash[t]);
 if (!suppress) printf ("  %s", filename);
 printf ("\n");

 return 0;
}

int do_sha256 (char *filename, int suppress)
{
 int t;
 SHA256Context myContext;
 uint8_t hash[SHA256HashSize];
 int c, e;
 FILE *file;
 char *slurp;
 size_t l;
 uint8_t mysha1[20];
 uint64_t lb;
 
 if (!strcmp(filename, "-"))
 {
  char foo[19];
  int h;

  /* 19 */
  strcpy(foo, "/var/tmp/shaXXXXXX");

  h=mkstemp(foo);
  if (h<0)
  {
   fprintf (stderr, "%s: could not open holding tank for stdin\n", progname);
   exit(1);
  }
  file=fdopen(h, "w");

  rewind(stdin);
  while ((c=getchar())>=0) fputc(c, file);
  fclose(file);
  e=do_sha256(foo, 1);
  unlink(foo);
  return e;
 }

 /* Slurp file */
 file=fopen(filename, "r");
 if (!file)
 {
  xperror(filename);
  return 1;
 }
 fseek(file, 0, SEEK_END);
 l=ftell(file);
 fseek(file, 0, SEEK_SET);
 slurp=malloc(l);
 if (!slurp) scram();
 if (fread(slurp, 1, l, file)<l)
 {
  free(slurp);
  perror(filename);
  return 1;
 }
 fclose(file);
 
 SHA256Reset(&myContext);
 SHA256Input(&myContext, slurp, l);
 SHA256Result(&myContext, hash);
 
 free(slurp);
 
 for (t=0; t<SHA256HashSize; t++) printf ("%02x", hash[t]);
 if (!suppress) printf ("  %s", filename);
 printf ("\n");

 return 0;
}

int do_sha384 (char *filename, int suppress)
{
 int t;
 SHA384Context myContext;
 uint8_t hash[SHA384HashSize];
 int c, e;
 FILE *file;
 char *slurp;
 size_t l;
 uint8_t mysha1[20];
 uint64_t lb;
 
 if (!strcmp(filename, "-"))
 {
  char foo[19];
  int h;

  /* 19 */
  strcpy(foo, "/var/tmp/shaXXXXXX");

  h=mkstemp(foo);
  if (h<0)
  {
   fprintf (stderr, "%s: could not open holding tank for stdin\n", progname);
   exit(1);
  }
  file=fdopen(h, "w");

  rewind(stdin);
  while ((c=getchar())>=0) fputc(c, file);
  fclose(file);
  e=do_sha384(foo, 1);
  unlink(foo);
  return e;
 }

 /* Slurp file */
 file=fopen(filename, "r");
 if (!file)
 {
  xperror(filename);
  return 1;
 }
 fseek(file, 0, SEEK_END);
 l=ftell(file);
 fseek(file, 0, SEEK_SET);
 slurp=malloc(l);
 if (!slurp) scram();
 if (fread(slurp, 1, l, file)<l)
 {
  free(slurp);
  perror(filename);
  return 1;
 }
 fclose(file);
 
 SHA384Reset(&myContext);
 SHA384Input(&myContext, slurp, l);
 SHA384Result(&myContext, hash);
 
 free(slurp);
 
 for (t=0; t<SHA384HashSize; t++) printf ("%02x", hash[t]);
 if (!suppress) printf ("  %s", filename);
 printf ("\n");

 return 0;
}

int do_sha512 (char *filename, int suppress)
{
 int t;
 SHA512Context myContext;
 uint8_t hash[SHA512HashSize];
 int c, e;
 FILE *file;
 char *slurp;
 size_t l;
 uint8_t mysha1[20];
 uint64_t lb;
 
 if (!strcmp(filename, "-"))
 {
  char foo[19];
  int h;

  /* 19 */
  strcpy(foo, "/var/tmp/shaXXXXXX");

  h=mkstemp(foo);
  if (h<0)
  {
   fprintf (stderr, "%s: could not open holding tank for stdin\n", progname);
   exit(1);
  }
  file=fdopen(h, "w");

  rewind(stdin);
  while ((c=getchar())>=0) fputc(c, file);
  fclose(file);
  e=do_sha512(foo, 1);
  unlink(foo);
  return e;
 }

 /* Slurp file */
 file=fopen(filename, "r");
 if (!file)
 {
  xperror(filename);
  return 1;
 }
 fseek(file, 0, SEEK_END);
 l=ftell(file);
 fseek(file, 0, SEEK_SET);
 slurp=malloc(l);
 if (!slurp) scram();
 if (fread(slurp, 1, l, file)<l)
 {
  free(slurp);
  perror(filename);
  return 1;
 }
 fclose(file);
 
 SHA512Reset(&myContext);
 SHA512Input(&myContext, slurp, l);
 SHA512Result(&myContext, hash);
 
 free(slurp);
 
 for (t=0; t<SHA512HashSize; t++) printf ("%02x", hash[t]);
 if (!suppress) printf ("  %s", filename);
 printf ("\n");

 return 0;
}

int sha224_main (int argc, char **argv)
{
 int e, r, t;
 
 if (argc==1) return do_sha224("-", 0);

 r=0;
 for (t=1; t<argc; t++)
 {
  e=do_sha224(argv[t], 0);
  if (r<e) r=e;
 }

 return r;
}

int sha256_main (int argc, char **argv)
{
 int e, r, t;
 
 if (argc==1) return do_sha256("-", 0);

 r=0;
 for (t=1; t<argc; t++)
 {
  e=do_sha256(argv[t], 0);
  if (r<e) r=e;
 }

 return r;
}

int sha384_main (int argc, char **argv)
{
 int e, r, t;
 
 if (argc==1) return do_sha384("-", 0);

 r=0;
 for (t=1; t<argc; t++)
 {
  e=do_sha384(argv[t], 0);
  if (r<e) r=e;
 }

 return r;
}

int sha512_main (int argc, char **argv)
{
 int e, r, t;
 
 if (argc==1) return do_sha512("-", 0);

 r=0;
 for (t=1; t<argc; t++)
 {
  e=do_sha512(argv[t], 0);
  if (r<e) r=e;
 }

 return r;
}

void sha_usage (void)
{
 fprintf (stderr, 
          "%s: usage: %s {sha224 | sha256 | sha384 | sha512} filename ...\n",
          progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (!strcmp(progname, "sha224")) return sha224_main(argc, argv);
 if (!strcmp(progname, "sha256")) return sha256_main(argc, argv);
 if (!strcmp(progname, "sha384")) return sha384_main(argc, argv);
 if (!strcmp(progname, "sha512")) return sha512_main(argc, argv);
 
 if (argc==1) sha_usage();
 
 argc--;
 argv++;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (!strcmp(progname, "sha224")) return sha224_main(argc, argv);
 if (!strcmp(progname, "sha256")) return sha256_main(argc, argv);
 if (!strcmp(progname, "sha384")) return sha384_main(argc, argv);
 if (!strcmp(progname, "sha512")) return sha512_main(argc, argv);

 fprintf (stderr, "%s: unknown algorithm '%s'\n", progname, argv[1]);
 return 1;
}
