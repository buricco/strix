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
 * Untested -I switch not documented.
 * System V -r switch (same as POSIX -R switch) not documented.
 * 4.4BSD -P switch not supported, and you're daft if you think it offers any
 * real security on modern drives.  No, I'm not making it a NOP, if you're
 * damn fool enough to use it, you deserve an error message.
 * 
 * Known bug: you'll get an "override?" prompt for a broken link.
 */

#ifdef __linux__
#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE 1
#define _GNU_SOURCE 1
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __SVR4__
typedef signed long ssize_t;
#endif

#define ITERATE_MAX_HANDLES 5

#define MODE_F 0x01
#define MODE_I 0x02
#define MODE_R 0x04
#define MODE_D 0x08 /* BSD: remove directories too */
#define MODE_X 0x10 /* FreeBSD, NetBSD: stay on one partition */
#define MODE_V 0x20 /* FreeBSD, OpenBSD, NetBSD: verbose */
#define MODE_SAFER 0x40
int mode;

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

static int global_e;
char i_can_fgets[3];

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

void cowardice (void)
{
 fprintf (stderr, "%s: cannot remove '.' or '..'\n", progname);
}

int do_rm_onefile (char *filename, int allow_directory)
{
 int axs;

 if (!strcmp(filename, ".")) {cowardice(); return 1;}
 if (!strcmp(filename, "..")) {cowardice(); return 1;}
 if (strlen(filename)>=2)
 {
  char *p;
  p=(filename+strlen(filename))-2;
  if (!strcmp(p, "/.")) {cowardice(); return 1;}
  if (p!=filename)
  {
   p--;
   if (!strcmp(p, "/..")) {cowardice(); return 1;}
  }
 }

 if ((!allow_directory)&&(!(mode&MODE_D)))
 {
  struct stat statbuf;

  axs=lstat(filename, &statbuf);
  if (axs)
  {
   if (mode&MODE_F) return 0;
   xperror(filename);
   return 1;
  }

  if (statbuf.st_mode&S_IFDIR)
  {
   errno=EISDIR;
   xperror(filename);
   return 1;
  }
 }

 axs=access(filename, W_OK);
 if ((axs)&&(!(mode&MODE_F)))
  fprintf (stderr, "override ");
 if ((mode&MODE_I)||(axs&&(!(mode&MODE_F))))
 {
  ssize_t e;
  size_t x;
  char *t;

  x=0;

  if (allow_directory) fprintf(stderr, "directory ");
  fprintf (stderr, "%s (y/n)? ", filename);
  fflush(stderr);
  while (1)
  {
   fflush(stdin);
   fgets(i_can_fgets, 2, stdin);
   if (feof(stdin)) exit(1); /* EOF, scram */
   if (*(i_can_fgets)=='\n') continue;
   if (*(i_can_fgets)!='y') return 1;
   break;
  }
 }

 if (remove(filename))
 {
  if (mode&MODE_F) return 0;
  xperror(filename);
  return 1;
 }
 if (mode&MODE_V) printf ("%s\n", filename);

 return 0;
}

int iterate_hit (const char *filename, const struct stat *statptr,
                 int fileflags, struct FTW *pftw)
{
 int e;

 if (!strcmp(filename, ".")) return 0;
 if (!strcmp(filename, "..")) return 0;
 if (strlen(filename)>=2)
 {
  char *p;
  p=((char *)filename+strlen(filename))-2;
  if (!strcmp(p, "/.")) return 0;
  if (p!=filename)
  {
   p--;
   if (!strcmp(p, "/..")) return 0;
  }
 }

 if (statptr->st_mode&S_IFDIR)
 {
  if (mode&MODE_SAFER)
  {
   fprintf (stderr, "directory %s (y/n)? ", filename);
   fflush(stderr);
   while (1)
   {
    fflush(stdin);
    fgets(i_can_fgets, 2, stdin);
    if (feof(stdin)) exit(1); /* EOF, scram */
    if (*(i_can_fgets)=='\n') continue;
    if (*(i_can_fgets)!='y') return 1;
    break;
   }
  }
  e=do_rm_onefile((char *)filename, 1);
 }
 else
  e=do_rm_onefile((char *)filename, 0);
 if (e>global_e) global_e=e;

 return 0;
}

int do_rm (char *filename)
{
 if (mode&MODE_R)
 {
  int m;

  m=FTW_DEPTH | FTW_PHYS;
  if (mode&MODE_X) m|=FTW_MOUNT;

  global_e=0;
  nftw(filename, iterate_hit, ITERATE_MAX_HANDLES, m);
  return global_e;
 }
 else
 {
  return do_rm_onefile (filename, 0);
 }
}

/*
 * Mollyguard to protect against removing all files in /.
 * This code is intentionally made hard to read.
 */
#ifndef NO_SUCKERS
void lollipop(int argc,char**argv){int r,t;r=0;for(t=optind;t<argc;t++){char*p;
p=strrchr(argv[t],'/');if(p==argv[t])r++;else if(p){while(p>argv[t])if(*p=='/')
p--;if((p==argv[t])&&(*p=='/'))r++;}}if(r>1){fprintf(stderr,"%s: cowardly refu"
"sing to remove multiple files from /\n",progname);exit(1);}}
#endif

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-Rdfivx] file...\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e, t;

 progname=strrchr(*argv, '/');
 if (progname) progname++; else progname=*argv;

 mode=0;

 while (-1!=(e=getopt(argc, argv, "IRdfirvx")))
 {
  switch (e)
  {
   case 'd':
    mode|=MODE_D;
    break;
   case 'f':
    mode&=~MODE_I; /* Posix me harder */
    mode|=MODE_F;
    break;
   case 'I':
    mode|=MODE_SAFER;
    break;
   case 'i':
    mode&=~MODE_F;
    mode|=MODE_I;
    break;
   case 'r': /* FALL THROUGH */
   case 'R':
    mode|=MODE_R;
    break;
   case 'v':
    mode|=MODE_V;
    break;
   case 'x':
    mode|=MODE_X;
    break;
   default:
    usage();
  }
 }

 /* No need for -d if -R; but silently ignore it */
 if (mode&MODE_R)
 {
  mode&=~MODE_D;
 }

 if (mode&MODE_SAFER)
 {
  mode&=~MODE_F;
  if (mode&MODE_I) mode&=~MODE_SAFER; /* Redundant */
 }

 if (optind==argc)
 {
  if (mode&MODE_F) return 0; /* Posix me harder */
  usage();
 }

#ifndef NO_SUCKERS
 lollipop(argc, argv);
#endif

 if ((mode&MODE_SAFER)&&((argc-optind)>2))
 {
  fprintf (stderr, "%u files specified, delete anyway (y/n)? ", argc-optind);
  fflush(stderr);
  while (1)
  {
   fflush(stdin);
   fgets(i_can_fgets, 2, stdin);
   if (feof(stdin)) exit(1); /* EOF, scram */
   if (*(i_can_fgets)=='\n') continue;
   if (*(i_can_fgets)!='y') return 1;
   break;
  }
 }

 e=0;
 for (t=optind; t<argc; t++)
 {
  int r;

  if (!strcmp(argv[t], "/"))
  {
   fprintf (stderr, "%s: cowardly refusing to remove '/'\n", progname);
   return 1;
  }

  r=do_rm(argv[t]);
  if (r>e) e=r;
 }

 return e;
}
