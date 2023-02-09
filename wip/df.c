/*
 * (C) Copyright 2020, 2023 S. V. Nickolas.
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

#include <sys/stat.h>
#include <sys/statfs.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mntent.h>
#include <unistd.h>

#ifndef _PATH_MOUNTED
#define _PATH_MOUNTED "/etc/mtab"
#endif

static char *copyright="@(#) (C) Copyright 2020, 2023 S. V. Nickolas\n";

static char *progname;

int h,k,P;

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

int df (char *what, char *where)
{
 struct statfs sfs;
 
 int e;
 int bsz;
 double multiplier;
 long long s,u,a;
 int p;
 int pint;
 
 e=statfs(what,&sfs);
 s=sfs.f_blocks;
 u=s-sfs.f_bfree;
 a=sfs.f_bavail;
 if (s) p=((s-u)*1000/s); else return 0;
 
 bsz=sfs.f_bsize;

 if (bsz!=512)
 {
  multiplier=(double)bsz/512;
  s*=multiplier;
  u*=multiplier;
  a*=multiplier;
 }
 
 if (k) {s>>=1; u>>=1; a>>=1;}
 
 pint=100-((p+5)/10);
 
 if (e)
  xperror(what);
 else
 {
  if (!h)
  {
   h=1;
   if (P) /* Posix me harder */
    printf ("Filesystem %d-blocks Used Available Capacity Mounted on\n",
            k?1024:512);
   else
    printf ("Filesystem            %s         Used    Available   Capacity  "
            "Mounted on\n",k?"1024-blocks":" 512-blocks");
  }
  if (P) /* Posix me harder */
   printf ("%s %llu %llu %llu %d%% %s\n",where,s,u,a,pint,what);
  else
   printf ("%-20s %12llu %12llu %12llu       %3d%%  %s\n",
           where,s,u,a,pint,what);
 }
 
 return e;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-kP] path ...\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 FILE *file;
 struct mntent *m;
 int e,t;
 e=k=h=P=0;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 while (-1!=(e=getopt(argc, argv, "kP")))
 {
  switch (e)
  {
   case 'k':
    k=1;
    break;
   case 'P':
    P=1;
    break;
   default:
    usage();
  }
 }
  
 e=0;
 
 if (argc==optind)
 {
  file=setmntent(_PATH_MOUNTED,"r");
  if (!file) {xperror(_PATH_MOUNTED); return 1;}
  while (0!=(m=getmntent(file)))
   e+=df(m->mnt_dir,m->mnt_fsname);
  endmntent(file);
 }
 else
 {
  for (t=optind; t<argc; t++)
  {
   struct stat s1, s2;
   file=setmntent(_PATH_MOUNTED,"r");
   if (!file) {xperror(_PATH_MOUNTED); return 1;}
   if ((lstat(argv[t],&s1)<0)||(S_ISCHR(s1.st_mode)))
   {
    fprintf (stderr, "%s: %s: not block device, directory or mountpoint\n", 
             progname, argv[t]);
    e++;
   }
   else
   {
    while (0!=(m=getmntent(file)))
     if (lstat(m->mnt_fsname,&s2)>=0)
      if (!memcmp(&(s1.st_dev), &(s2.st_rdev), sizeof(s1.st_dev)))
       e+=df(m->mnt_dir,m->mnt_fsname);

    endmntent(file);
   }
  }
 }
 
 return e;
}
