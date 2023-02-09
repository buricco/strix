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
 * This is not a complete implementation of cp(1) - it only copies ONE file
 * from source to target, doesn't recognize that it might be a path, won't
 * prompt for overwrite, anything.  It's just the rib cage.
 * 
 * mv(1) also needs this functionality.  BSD's mv (since Reno) will use an
 * internal file copier to handle single files across devices but will shell
 * out to cp and rm to handle trees - I don't think this is wise.  Rather,
 * my preferred approach is to integrate cp and mv into a single utility which
 * uses the same exact code to handle copying trees.
 */

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char *progname;

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

/*
 * Routine to copy a file.
 * 
 * If copying all attributes is desired (cp -p), set attr to nonzero.
 * Otherwise, only basic permissions will be copied (as with regular cp).
 * Returns 0=success, 1=warnings, 2=failure.
 */
int copyfile (char *from, char *to, int attr)
{
 int e;
 int r;
 FILE *in, *out;
 struct stat statbuf;
 char cpbuf[BUFSIZ];
 uid_t u;
 gid_t g;
 off_t l;
 mode_t m;
 
#ifdef CP_OLD_UTIME
 struct utimbuf T;
#else
 struct timespec T[2];
#endif
 
 r=0;
 
 if (stat(from, &statbuf))
 {
  xperror(from);
  return 2;
 }
 
 l=statbuf.st_size;
 u=statbuf.st_uid;
 g=statbuf.st_gid;
 m=statbuf.st_mode;
#ifdef CP_OLD_UTIME
 T.actime=statbuf.st_atime;
 T.modtime=statbuf.st_mtime;
#else
 T[0]=statbuf.st_atim;
 T[1]=statbuf.st_mtim;
#endif
 
 in=fopen(from, "rb");
 if (!in)
 {
  xperror(from);
  return 2;
 }
 
 out=fopen(to, "wb");
 if (!out)
 {
  xperror(to);
  fclose(in);
  return 2;
 }
 
 while (l>BUFSIZ)
 {
  e=fread(cpbuf, 1, BUFSIZ, in);
  if (e<BUFSIZ)
  {
   fclose(in);
   fclose(out);
   xperror(from);
   return 2;
  }
  
  e=fwrite(cpbuf, 1, BUFSIZ, out);
  if (e<BUFSIZ)
  {
   fclose(in);
   fclose(out);
   xperror(to);
   return 2;
  }
  
  l-=BUFSIZ;
 }
 
 if (l)
 {
  e=fread(cpbuf, 1, l, in);
  if (e<l)
  {
   fclose(in);
   fclose(out);
   xperror(from);
   return 2;
  }
  
  e=fwrite(cpbuf, 1, l, out);
  if (e<l)
  {
   fclose(in);
   fclose(out);
   xperror(to);
   return 2;
  }
 }
 
 r=0;
 
 if (attr)
 {
  /* Change the owner; if we can't, disable SUID-SGID */
  e=fchown(fileno(out), u, g);
  if (e)
  {
   fprintf (stderr, "%s: %s: cannot chown: %s\n", 
            progname, to, strerror(errno));
   m&=~(S_ISUID|S_ISGID);
   r=1;
  }
 }
 else
 {
  /* Strip the permissions we copy down to within the basic 777. */
  m&=0777;
 }
 
 e=fchmod(fileno(out), m);
 if (e)
 {
  fprintf (stderr, "%s: %s: cannot chmod: %s\n",
           progname, to, strerror(errno));
  r=1;
 }
 
 fclose(in);
 fclose(out);
 
 if (!attr) return r;
 
 /* Copy timestamps */
#ifdef CP_OLD_UTIME
 e=utime(to, T);
#else
 e=utimensat(AT_FDCWD, to, T, 0);
#endif
 return r;
}

/* Test routine */
int main (int argc, char **argv)
{
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (argc!=3)
 {
  fprintf (stderr, "%s: usage: %s source target\n", progname, progname);
  return 2;
 }
 return copyfile(argv[1], argv[2], 1);
}
