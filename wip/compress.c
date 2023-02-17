/*
 * (C) Copyright 2023 S. V. Nickolas.
 * 
 * This code contains a function (do_inflate()) derived from example code
 * explicitly freed of copyright and dedicated to the public domain on
 * December 11, 2005 by Mark Adler.
 *   * Reference: https://www.zlib.net/zlib_how.html
 *   * Date of Citation: February 8, 2023
 *   * Confirmation: S. V. Nickolas
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
 * In this version of compress, and the various other aliases by which it
 * may be invoked:
 *   * compress/decompress are not yet implemented.
 *   * gzip/gunzip is implemented.
 *     * This is done through zlib.
 */

#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

#define MODE_C 0x0001 /* use stdout                      */
#define MODE_D 0x0002 /* decompress                      */
#define MODE_F 0x0004 /* force                           */
#define MODE_K 0x0008 /* keep                            */
#define MODE_L 0x0010 /* list                            */
#define MODE_N 0x0020 /* place/use name/stamps from file */
#define MODE_Q 0x0040 /* quiet                           */
#define MODE_T 0x0080 /* test                            */
#define MODE_V 0x0100 /* verbose                         */
#define MODE_R 0x0200 /* recursive                       */
#define MODE_E 0x0400 /* extreme                         */
int mode;
int level; /* 1-9/6 for gzip/bzip2, 0-9/6 for xz, 9-16/14 for compress */

enum { CODEC_LZW, CODEC_GZIP } codec;

char outname[PATH_MAX];

char *sfx_default;
char *sfx;

/****************************************************************************
 *                                                                          *
 * UNIVERSAL FUNCTIONS - MUST BE DESCRIBED HERE OR VIA FORWARD DECLARATIONS *
 *                                                                          *
 ****************************************************************************/

char *xfilename (char *filename)
{
 char *x;

 x=filename;
 if (!strcmp(filename, "-")) x="(stdin)";
 return x;
}

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, xfilename(filename), strerror(errno));
}

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

int ckstomp (char *filename)
{
 char lbuf[5];
 int e;
 struct stat statbuf;
 
 if (mode&MODE_F) return 0;
 
 e=stat(filename, &statbuf);
 if (e) return 0;
 
 if (!(isatty(0))) return 0;

 while (1)
 {
  fprintf (stderr, "%s: clobber %s (y/n)? ", progname, filename);
  fgets(lbuf, 4, stdin);
  if ((*lbuf=='y')||(*lbuf=='Y')) return 0;
  if ((*lbuf=='n')||(*lbuf=='N')) return 1;
 }
}

/****************************************************************************
 *                                                                          *
 *                  DUMMY COMPONENT (COMPRESS/DECOMPRESS)                   *
 *                                                                          *
 ****************************************************************************/

int Z_c (char *filename)
{
 fprintf (stderr, "%s: %s: codec not supported\n", progname, filename);
 return 1;
}

int Z_d (char *filename)
{
 fprintf (stderr, "%s: %s: codec not supported\n", progname, filename);
 return 1;
}

/****************************************************************************
 *                                                                          *
 *                       ZLIB COMPONENT (GZIP/GUNZIP)                       *
 *                                                                          *
 ****************************************************************************/
#include <zlib.h>

typedef struct {
 uint8_t  magic[2];
 uint8_t  method;
 uint8_t  fflags;
 uint8_t  time[4];
 uint8_t  cflags;
 uint8_t  srcos;
} gzhdr;

typedef struct {
 uint32_t crc32;
 uint32_t flen;
} gzftr;

#define GZF_TEXT    0x01
#define GZF_HDRCRC  0x02
#define GZF_XTRA    0x04
#define GZF_SRCFNM  0x08
#define GZF_COMMENT 0x10

int gz_c (char *filename)
{
 FILE *in;
 int e;
 int isstdin;
 gzFile gzhand;
 char modestr[20];
 char *buf;
 struct stat statbuf;
 time_t t;
 size_t l;
 
 buf=malloc(BUFSIZ);
 if (!buf) scram();
 
 /* Input is not stdin */
 if (strcmp(filename, "-"))
 {
  if (strlen(filename)>=strlen(sfx))
  {
   if (!strcmp(filename+strlen(filename)-strlen(sfx), sfx))
   {
    fprintf (stderr, "%s: %s: cowardly refusing to double compress\n",
             progname, filename);
    free(buf);
    return 1;
   }
  }
  
  e=stat(filename, &statbuf);
  if (e)
  {
   xperror(filename);
   free(buf);
   return 1;
  }
  if (strlen(filename)+strlen(sfx)>=PATH_MAX)
  {
   fprintf (stderr, "%s: %s: filename would be too long\n", 
            progname, filename);
   free(buf);
   return 1;
  }
  strcpy(outname, filename);
  strcat(outname, sfx);
  isstdin=0;
  in=fopen(filename, "rb");
  if (!in)
  {
   xperror(filename);
   free(buf);
   return 1;
  }
 }
 else
 {
  isstdin=1;
  rewind(stdin);
 }
 
 sprintf (modestr, "wb%u", level);
 if ((mode&MODE_C)||(isstdin))
 {
  strcpy(outname, "-");
  if (isatty(1)&&(!(mode&MODE_F)))
  {
   fprintf (stderr, 
            "%s: cowardly refusing to write crunched data to a tty"
            " (-f to force)\n", progname);
   free(buf);
   return 1;
  }
  
  gzhand=gzdopen(1, modestr);
 }
 else
 {
  if (ckstomp(outname))
  {
   free(buf);
   return 1;
  }
  gzhand=gzopen(outname, modestr);
 }
 if (!gzhand)
 {
  xperror(filename);
  free(buf);
  return 1;
 }
 
 while (1)
 {
  int m;
  
  l=fread(buf, 1, BUFSIZ, in);
  if (!l)
  {
   if (feof(in))
   {
    if (isstdin)
     rewind(in);
    else
     fclose(in);
    
    break;
   }
   xperror(filename);
   free(buf);
   return 1;
  }
  
  m=gzwrite(gzhand, buf, l);
  if (m<1)
  {
   xperror(outname);
   free(buf);
   return 1;
  }
 }
 
 free(buf);
 gzclose(gzhand);
 if ((!isstdin)&&(!(mode&MODE_C)))
 {
  struct utimbuf u;
  
  t=time(0);
  u.actime=t;
  u.modtime=statbuf.st_mtime;
  utime(outname, &u);
  chmod(outname, statbuf.st_mode);
  
  if (!(mode&MODE_K))
  {
   if (unlink(filename))
   {
    fprintf (stderr, "%s: unable to unlink source file %s\n", 
             progname, filename);
    return 1;
   }
  }
  
  if (mode&MODE_V)
  {
   int p;
   size_t b, a;
   b=statbuf.st_size;
   stat(outname, &statbuf);
   a=statbuf.st_size;
   p=((b-a)*100)/b;
   fprintf (stderr, "%s -> %s: %lu to %lu, %d%% reduction\n",
            filename, outname, b, a, p);
  }
 }
 return 0;
}

#ifdef OLD_GUNZIP
int gz_d (char *filename)
{
 int isstdin;
 FILE *out;
 gzFile gzhand;
 struct stat statbuf;
 char *buf;
 size_t l, b, a;
 time_t t;
 
 buf=malloc(BUFSIZ);
 if (!buf) scram();
 
 /* Generate filenames */
 strcpy(outname, filename);
 if (strcmp(filename, "-"))
 {
  isstdin=0;
  if (strlen(filename)>strlen(sfx))
  {
   if (strcmp(filename+(strlen(filename)-strlen(sfx)), sfx))
   {
    if (strlen(filename)+strlen(sfx)>=PATH_MAX)
    {
     fprintf (stderr, "%s: %s: filename would be too long\n", 
              progname, filename);
     free(buf);
     return 1;
    }
    strcat(filename, sfx);
   }
   else
   {
    outname[strlen(outname)-strlen(sfx)]=0;
   }
  }
  else
  {
   strcpy(filename, sfx);
  }
 }
 else
 {
  isstdin=1;
  
  if (isatty(1)&&(!(mode&MODE_F)))
  {
   fprintf (stderr, 
            "%s: cowardly refusing to read crunched data from a tty"
            " (-f to force)\n", progname);
   free(buf);
   return 1;
  }
  rewind(stdin);
 }
 
 if (isstdin||(mode&MODE_C))
  strcpy(outname, "-");
 else
 {
  int e;
  
  e=stat(filename, &statbuf);
  if (e)
  {
   xperror(filename);
   free(buf);
   return 1;
  }
 }

 if (isstdin)
  gzhand=gzdopen(0, "rb");
 else
  gzhand=gzopen(filename, "rb");
 
 if (!gzhand)
 {
  free(buf);
  xperror(filename);
  return 1;
 }
 
 if (isstdin||(mode&MODE_C))
  out=stdout;
 else
 {
  if (ckstomp(outname))
  {
   free(buf);
   return 1;
  }
  out=fopen(outname, "wb");
  if (!out)
  {
   free(buf);
   xperror(outname);
   return 1;
  }
 }
 
 while (1)
 {
  size_t m;
  
  l=gzread(gzhand, buf, BUFSIZ);
  if (l<1)
  {
   if (gzeof(gzhand))
    break;
   if (!isstdin) 
    gzclose(gzhand);
   else
    rewind(stdin);
   xperror(filename);
   free(buf);
   return 1;
  }
  m=fwrite(buf, 1, l, out);
  if (m<l)
  {
   free(buf);
   xperror(outname);
   return 1;
  }
 }

 if (!isstdin) 
  gzclose(gzhand);
 else
  rewind(stdin);
 
 if (!(isstdin||(mode&MODE_C)))
  fclose(out);
 
 free(buf);
 if ((!isstdin)&&(!(mode&MODE_C)))
 {
  struct utimbuf u;
  
  t=time(0);
  u.actime=t;
  u.modtime=statbuf.st_mtime;
  utime(outname, &u);
  chmod(outname, statbuf.st_mode);
  
  if (!(mode&MODE_K))
  {
   if (unlink(filename))
   {
    fprintf (stderr, "%s: unable to unlink source file %s\n", 
             progname, filename);
    return 1;
   }
  }
  
  if (mode&MODE_V)
  {
   int p;
   size_t b, a;
   b=statbuf.st_size;
   stat(outname, &statbuf);
   a=statbuf.st_size;
   p=((a-b)*100)/a;
   fprintf (stderr, "%s -> %s: %lu to %lu, %d%% reduction\n",
            filename, outname, b, a, p);
  }
 }
 return 0;
}
#else
/*
 * From https://www.zlib.net/zlib_how.html
 * Original code bears an explicit PD declaration/disclaimer of copyright
 */
int do_inflate (FILE *source, FILE *dest)
{
 int ret;
 unsigned have;
 z_stream strm;
 unsigned char in[BUFSIZ];
 unsigned char out[BUFSIZ];

 strm.zalloc = Z_NULL;
 strm.zfree = Z_NULL;
 strm.opaque = Z_NULL;
 strm.avail_in = 0;
 strm.next_in = Z_NULL;
 ret = inflateInit2(&strm, 16);
 if (ret != Z_OK) return ret; 
 do
 {
  strm.avail_in = fread(in, 1, BUFSIZ, source);
  if (ferror(source)) 
  {
   (void)inflateEnd(&strm);
   return Z_ERRNO;
  }
  if (strm.avail_in == 0) break;
  strm.next_in = in;
  do
  {
   strm.avail_out = BUFSIZ;
   strm.next_out = out;  
   
   ret = inflate(&strm, Z_NO_FLUSH);
   assert(ret != Z_STREAM_ERROR); /* state not clobbered */
   switch (ret) 
   {
    case Z_NEED_DICT:
     ret = Z_DATA_ERROR; /* FALL THROUGH */
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
     (void)inflateEnd(&strm);
     return ret;
   }
   have = BUFSIZ - strm.avail_out;
   if (fwrite(out, 1, have, dest) != have || ferror(dest)) 
   {
    (void)inflateEnd(&strm);
    return Z_ERRNO;
   }   
  } while (!strm.avail_out);
 } while (ret!=Z_STREAM_END);
 (void)inflateEnd(&strm);
 return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int gz_d (char *filename)
{
 int isstdin, isstdout;
 gzhdr mygzhdr;
 FILE *in, *out;
 unsigned long xtra;
 int r;
 int tank;
 struct stat statbuf;
 char iname[PATH_MAX], oname[PATH_MAX], rname[BUFSIZ];
 char wmode[3];
 time_t t;
 
 strcpy(wmode, "wb");
 
 /* Construct actual filenames (input must have suffix, output not). */
 strcpy(iname, filename);
 strcpy(oname, filename);
 if (strlen(oname)>strlen(sfx))
 {
  if (!strcmp(&(oname[strlen(oname)-strlen(sfx)]), sfx))
   oname[strlen(oname)-strlen(sfx)]=0;
 }
 if (strlen(iname)>strlen(sfx))
 {
  if (strcmp(&(iname[strlen(iname)-strlen(sfx)]), sfx))
   strcat(iname, sfx);
 }
 else
 {
  if (strcmp(iname, "-"))
   strcat(iname, sfx);
 }
 
 if (mode&MODE_C) strcpy(oname, "-");
 
 if (!strcmp(iname, "-"))
 {
  if (isatty(1)&&(!(mode&MODE_F)))
  {
   fprintf (stderr, 
            "%s: cowardly refusing to read crunched data from a tty"
            " (-f to force)\n", progname);
   return 1;
  }
 }
 
 if (strcmp(filename, "-"))
 {
  in=fopen(iname, "rb");
  if (!in)
  {
   xperror(iname);
   return 1;
  }
  isstdin=0;

  if (stat(filename, &statbuf))
  {
   xperror(filename);
   return 1;
  }
 }
 else
 {
  isstdin=1;
  rewind(stdin);
  in=stdin;
 }

 if (fread(&mygzhdr, 10, 1, in)!=1)
 {
  fprintf (stderr, "%s: %s: could not read header\n", 
           progname, xfilename(iname));
  if (!isstdin) fclose(in);
  return 1;
 }
 
 if ((mygzhdr.magic[0]!=0x1F)||(mygzhdr.magic[1]!=0x8B))
 {
  fprintf (stderr, "%s: %s: not a gzip file\n",
           progname, xfilename(iname));
  if (!isstdin) fclose(in);
  return 1;
 }
 
 if (mygzhdr.method!=8)
 {
  fprintf (stderr, "%s: %s: unknown compression type 0x%02X\n",
           progname, xfilename(iname), mygzhdr.method);
  if (!isstdin) fclose(in);
  return 1;
 }
 
 if (mygzhdr.fflags&GZF_TEXT)
  wmode[1]='t';
 
 /* Ignore any XTRA fields */
 if (mygzhdr.fflags&GZF_XTRA)
 {
  tank=fgetc(in);
  xtra=fgetc(in);
  xtra<<=8;
  xtra|=tank;
  while (tank--) fgetc(in);
 }
 
 /* Get filename, if stored */
 if (mygzhdr.fflags&GZF_SRCFNM)
 {
  char *p;
  
  p=rname;
  tank=-1;
  while (tank)
  {
   tank=fgetc(in);
   
   if (tank<0)
   {
    xperror(iname);
    if (!isstdin) fclose(in);
    return 1;
   }
   *p++=tank;
  }
 }
 else
 {
  strcpy(rname, oname);
 }
 
 if (mode&MODE_N) strcpy(oname, rname);
 
 if (strcmp(oname, "-"))
 {
  out=fopen(oname, wmode);
  if (!out)
  {
   xperror(oname);
   if (!isstdin) fclose(in);
   return 1;
  }
  isstdout=0;
 }
 else
 {
  isstdout=1;
  out=stdout;
 }
 
 fseek(in, 0, SEEK_SET);

 r=do_inflate(in, out);
 
 if (!isstdin) fclose(in);
 if (!isstdout)
 {
  struct utimbuf u;

  fclose(out);
  
  t=time(0);
  u.actime=t;
  u.modtime=statbuf.st_mtime;
  if (mode&MODE_N)
  {
   t=mygzhdr.time[3];
   t<<=8;
   t|=mygzhdr.time[2];
   t<<=8;
   t|=mygzhdr.time[1];
   t<<=8;
   t|=mygzhdr.time[0];
   if (t) u.modtime=t;
  }
  utime(oname, &u);
  chmod(oname, statbuf.st_mode);
  
  if (!(mode&MODE_K))
  {
   if (unlink(filename))
   {
    fprintf (stderr, "%s: unable to unlink source file %s\n", 
             progname, filename);
    return 1;
   }
  }
  
  if (mode&MODE_V)
  {
   int p;
   size_t b, a;
   b=statbuf.st_size;
   stat(outname, &statbuf);
   a=statbuf.st_size;
   p=((a-b)*100)/a;
   fprintf (stderr, "%s -> %s: %lu to %lu, %d%% reduction\n",
            iname, oname, b, a, p);
  }
 }
 return r;
}
#endif

/****************************************************************************
 *                                                                          *
 *                                 FRONT END                                *
 *                                                                          *
 ****************************************************************************/

int autodec (char *filename)
{
 FILE *file;
 char buf[2];
 
 file=fopen(filename, "rb");
 if (!file)
 {
  xperror(filename);
  return 1;
 }
 fread(buf, 1, 2, file);
 fclose(file);
 
 if ((buf[0]==0x1F)&&(buf[1]==0x9D))
  return Z_d(filename);
 if ((buf[0]==0x1F)&&(buf[1]==0x8B))
  return gz_d(filename);
 
 fprintf (stderr, "%s: %s: unknown compressed file format\n", 
          progname, filename);
 return 1;
}

int do_the_needful (char *filename)
{
 int e;
 char *xfilename;
 
 if (mode&MODE_D)
 {
  xfilename=malloc(strlen(filename)+strlen(sfx)+1);
  if (!xfilename) scram();
  strcpy(xfilename, filename);
 }

#if 0
 if (mode&MODE_D) return autodec(filename?xfilename:"-");
#else
 if (mode&MODE_D)
 {
  switch (codec)
  {
   case CODEC_LZW:
    e=Z_d (filename?filename:"-");
    break;
   case CODEC_GZIP:
    e=gz_d (filename?filename:"-");
    break;
  }
  
  return e;
 }
#endif

 switch (codec)
 {
  case CODEC_LZW:
   e=Z_c (filename?filename:"-");
   break;
  case CODEC_GZIP:
   e=gz_c (filename?filename:"-");
   break;
 }
 
 if (mode&MODE_D) free(xfilename);
 return e;
}

void usage (void)
{
 fprintf (stderr, 
          "%s: usage: %s [-cdfgkqv] [-b level] [-m method] [file ...]\n",
          progname, progname);
 exit(1);
}

void warnnop (char *s)
{
 fprintf (stderr, "%s: '%s' ignored\n", progname, s);
}

void ckcodec (char *arg)
{
 if (!strcmp(arg, "lzw")) {codec=CODEC_LZW; return;}
 if (!strcmp(arg, "compress")) {codec=CODEC_LZW; return;}
 if (!strcmp(arg, "deflate")) {codec=CODEC_GZIP; return;}
 if (!strcmp(arg, "gzip")) {codec=CODEC_GZIP; return;}
 fprintf (stderr, "%s: invalid codec '%s'\n", progname, arg);
 exit(1);
}

int cknum (char *arg)
{
 int e;
 
 errno=0;
 e=strtol(arg, 0, 0);
 if (!errno) return e;
 fprintf (stderr, "%s: bogus numeric value '%s'\n", progname, arg);
 exit(1);
}

int main (int argc, char **argv)
{
 int e, r, t;
 int digitlevel;
 int threads; /* ignored, for xz compatibility */
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];
 
 /*
  * Set defaults, and mark some values as untouched.
  * Those values will be set to defaults after switch checks.
  */
 mode=digitlevel=0;
 level=-1;
 codec=CODEC_LZW;
 sfx=0;
 threads=1;

 if (!strcmp(progname, "uncompress"))
  mode=MODE_D;
 if (!strcmp(progname, "gzip"))
  codec=CODEC_GZIP;
 if (!strcmp(progname, "gunzip"))
 {
  mode=MODE_D;
  codec=CODEC_GZIP;
 }
 if ((!strcmp(progname, "zcat"))||(!strcmp(progname, "gzcat")))
  mode=MODE_C|MODE_D;
 
 /*
  * Parse switches.
  * Some of these are undocumented, and some are nonfunctional.
  */
 while (-1!=(e=getopt(argc, argv, "0123456789b:cdefgklm:nNqRrS:sT:tv")))
 {
  switch (e)
  {
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
    if (digitlevel)
    {
     level *= 10;
     level += (e-'0');
     break;
    }
    digitlevel=1;
    level=e-'0';
    break;
   case 'b':
    digitlevel=0;
    level=cknum(optarg);
    break;
   case 'c':
    mode |= MODE_C;
    break;
   case 'd':
    mode |= MODE_D;
    break;
   case 'e':
    mode |= MODE_E;
    break;
   case 'f':
    mode |= MODE_F;
    break;
   case 'g':
    codec=CODEC_GZIP;
    break;
   case 'k':
    mode |= MODE_K;
    break;
   case 'l':
    mode |= MODE_L;
    break;
   case 'm':
    ckcodec(optarg);
    break;
   case 'n':
    warnnop("-n");
    break;
   case 'N':
    warnnop("-N");
    break;
   case 'q':
    mode |= MODE_Q;
    mode &= ~MODE_V;
    break;
   case 'R':
   case 'r':
    mode |= MODE_R;
    break;
   case 'S':
    sfx=optarg;
    break;
   case 's': /* NOP, but don't warn */
    break;
   case 'T':
    warnnop("-T");
    threads=cknum(optarg);
    break;
   case 't':
    mode |= MODE_T;
    break;
   case 'v':
    mode &= ~MODE_Q;
    mode |= MODE_V;
    break;
   default:
    usage();
  }
 }
 
 /* Set codec-specific defaults and check codec-specific values. */
 switch (codec)
 {
  case CODEC_LZW:
   sfx_default=".Z";
   if (level==-1) level=14;
   if ((level<9)||(level>16))
   {
    fprintf (stderr, "%s: compression level must be between 9-16: '%u'\n",
             progname, level);
    return 1;
   }
   break;
  case CODEC_GZIP:
   sfx_default=".gz";
   if (level==-1) level=6;
   if ((level<1)||(level>9))
   {
    fprintf (stderr, "%s: compression level must be between 1-9: '%u'\n",
             progname, level);
    return 1;
   }
   break;
 }
 
 if (!sfx) sfx=sfx_default;
 
 if (argc==optind) return do_the_needful("-");
 
 r=0;
 for (t=optind; t<argc; t++)
 {
  e=do_the_needful(argv[t]);
  if (r<e) r=e;
 }
 return r;
}
