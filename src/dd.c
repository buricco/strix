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
 * I don't know if everything I'm doing is accurate.  Here's how I understand
 * the various switches...
 *
 * if       Input filename. (Default is stdin)
 * of       Output filename. (Default is stdout)
 * ibs      Input blocksize. (Default is 512)
 * obs      Output blocksize. (Default is 512)
 * bs       Input and output blocksize.
 * cbs      Conversion blocksize.  (Default is 0)
 *            We don't actually use this for anything
 * skip     Skip blocks in input.
 * seek     Skip blocks in output.  Also "oseek" in SVR4
 * count    Maximum number of blocks to copy.
 * conv     Conversion modes, comma-separated:
 *          ascii, ebcdic, ibm (these are mutually exclusive)
 *          block, unblock (variable width to fixed width)
 *          lcase, ucase (case smash)
 *          swab (word swap)
 *          noerror (do not die on an I/O error)
 *          notrunc (do not clobber when opening an existing file)
 *          sync (pad input blocks with NUL, or space with block)
 *            (also take isync as an extension)
 *          osync (pad output blocks with NUL, or space with block)
 *            (4.4BSD-Lite extension)
 *
 * Following a blocksize with "k" multiplies it by 1024.
 * Following a blocksize with "b" multiplies it by 512.
 * Following a blocksize with "w" multiplies it by 2 (SVR4).
 * Following a blockside with "d" multiplies it by 4 (my extension).
 * Two numbers divided by "x" are multiplied together.
 * Posix defines the conversion tables.  To "IBM" is nonreversible,
 *   but "ASCII" and "EBCDIC" are reversible.
 */

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

#ifdef __SVR4__
#define strtoxl(x) strtoul(x,0,0)
typedef unsigned long xlong;
#else
#define strtoxl(x) strtoull(x,0,0)
typedef unsigned long long xlong;
#endif

#define MODE_ASCII   0x0001
#define MODE_EBCDIC  0x0002
#define MODE_IBM     0x0004
#define MODE_BLOCK   0x0008
#define MODE_UNBLOCK 0x0010
#define MODE_LCASE   0x0020
#define MODE_UCASE   0x0040
#define MODE_SWAB    0x0080
#define MODE_NOERROR 0x0100
#define MODE_NOTRUNC 0x0200
#define MODE_SYNC    0x0400
#define MODE_OSYNC   0x0800
int mode;
xlong ibs, obs, cbs;
xlong bskip, bseek, bcount;
char *fn_in, *fn_out;
xlong igotf, igotp, ogotf, ogotp, truncs;
int isstdin, isstdout;
FILE *in, *out;

static unsigned char convtab_ascii[256]={
 0x00, 0x01, 0x02, 0x03, 0x9C, 0x09, 0x86, 0x7F,
 0x97, 0x8D, 0x8E, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
 0x10, 0x11, 0x12, 0x13, 0x9D, 0x85, 0x08, 0x87,
 0x18, 0x19, 0x92, 0x8F, 0x1C, 0x1D, 0x1E, 0x1F,
 0x80, 0x81, 0x82, 0x83, 0x84, 0x0A, 0x17, 0x1B,
 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x05, 0x06, 0x07,
 0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04,
 0x98, 0x99, 0x9A, 0x9B, 0x14, 0x15, 0x9E, 0x1A,
 0x20, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
 0xA7, 0xA8, 0xD5, 0x2E, 0x3C, 0x28, 0x2B, 0x7C,
 0x26, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
 0xB0, 0xB1, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x7E,
 0x2D, 0x2F, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
 0xB8, 0xB9, 0xCB, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,
 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1,
 0xC2, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,
 0xC3, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
 0x68, 0x69, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
 0xCA, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
 0x71, 0x72, 0x5E, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
 0xD1, 0xE5, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
 0x79, 0x7A, 0xD2, 0xD3, 0xD4, 0x5B, 0xD6, 0xD7,
 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0x5D, 0xE6, 0xE7,
 0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
 0x48, 0x49, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED,
 0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
 0x51, 0x52, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3,
 0x5C, 0x9F, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
 0x59, 0x5A, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9,
 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
 0x38, 0x39, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static unsigned char convtab_ebcdic[256]={
 0x00, 0x01, 0x02, 0x03, 0x37, 0x2D, 0x2E, 0x2F,
 0x16, 0x05, 0x25, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
 0x10, 0x11, 0x12, 0x13, 0x3C, 0x3D, 0x32, 0x26,
 0x18, 0x19, 0x3F, 0x27, 0x1C, 0x1D, 0x1E, 0x1F,
 0x40, 0x5A, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D,
 0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,
 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
 0xF8, 0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F,
 0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
 0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
 0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
 0xE7, 0xE8, 0xE9, 0xAD, 0xE0, 0xBD, 0x9A, 0x6D,
 0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
 0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,
 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
 0xA7, 0xA8, 0xA9, 0xC0, 0x4F, 0xD0, 0x5F, 0x07,
 0x20, 0x21, 0x22, 0x23, 0x24, 0x15, 0x06, 0x17,
 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x09, 0x0A, 0x1B,
 0x30, 0x31, 0x1A, 0x33, 0x34, 0x35, 0x36, 0x08,
 0x38, 0x39, 0x3A, 0x3B, 0x04, 0x14, 0x3E, 0xE1,
 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
 0x49, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
 0x58, 0x59, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
 0x68, 0x69, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
 0x76, 0x77, 0x78, 0x80, 0x8A, 0x8B, 0x8C, 0x8D,
 0x8E, 0x8F, 0x90, 0x6A, 0x9B, 0x9C, 0x9D, 0x9E,
 0x9F, 0xA0, 0xAA, 0xAB, 0xAC, 0x4A, 0xAE, 0xAF,
 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xA1, 0xBE, 0xBF,
 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xDA, 0xDB,
 0xDC, 0xDD, 0xDE, 0xDF, 0xEA, 0xEB, 0xEC, 0xED,
 0xEE, 0xEF, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static unsigned char convtab_ibm[256]={
 0x00, 0x01, 0x02, 0x03, 0x37, 0x2D, 0x2E, 0x2F,
 0x16, 0x05, 0x25, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
 0x10, 0x11, 0x12, 0x13, 0x3C, 0x3D, 0x32, 0x26,
 0x18, 0x19, 0x3F, 0x27, 0x1C, 0x1D, 0x1E, 0x1F,
 0x40, 0x5A, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D,
 0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,
 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
 0xF8, 0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F,
 0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
 0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
 0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
 0xE7, 0xE8, 0xE9, 0xAD, 0xE0, 0xBD, 0x5F, 0x6D,
 0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
 0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,
 0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
 0xA7, 0xA8, 0xA9, 0xC0, 0x4F, 0xD0, 0xA1, 0x07,
 0x20, 0x21, 0x22, 0x23, 0x24, 0x15, 0x06, 0x17,
 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x09, 0x0A, 0x1B,
 0x30, 0x31, 0x1A, 0x33, 0x34, 0x35, 0x36, 0x08,
 0x38, 0x39, 0x3A, 0x3B, 0x04, 0x14, 0x3E, 0xE1,
 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
 0x49, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
 0x58, 0x59, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
 0x68, 0x69, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
 0x76, 0x77, 0x78, 0x80, 0x8A, 0x8B, 0x8C, 0x8D,
 0x8E, 0x8F, 0x90, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E,
 0x9F, 0xA0, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xDA, 0xDB,
 0xDC, 0xDD, 0xDE, 0xDF, 0xEA, 0xEB, 0xEC, 0xED,
 0xEE, 0xEF, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

unsigned char *ibuf, *obuf, *cbuf;

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

xlong getsize (char *foo, int ok0)
{
 int mulfac;
 xlong t;

 if (strchr(foo, 'x'))
 {
  char *p, *q;

  if (NULL==(p=strdup(foo))) scram();
  q=strchr(p, 'x');
  *(q++)=0;
  t=getsize(p,ok0)*getsize(q,ok0);
  free(p);
  return t;
 }

 mulfac=0;
 switch (foo[strlen(foo)-1])
 {
  case 'b':
   mulfac=9;
   foo[strlen(foo)]=0;
  case 'd':
   mulfac=2;
   foo[strlen(foo)]=0;
  case 'w':
   mulfac=1;
   foo[strlen(foo)]=0;
  case 'k':
   mulfac=10;
   foo[strlen(foo)]=0;
 }
 errno=0;
 t=strtoxl(foo);
 if (errno)
 {
  fprintf (stderr, "%s: bogus size '%s'\n", progname, foo);
  exit (1);
 }

 if ((!t)&&(!ok0))
 {
  fprintf (stderr, "%s: value cannot be zero\n", progname);
  exit(1);
 }
 return t<<mulfac;
}

char *findtok (char *against, char *mytok)
{
 if (!strncmp(against, mytok, strlen(mytok)))
  if (against[strlen(mytok)]=='=')
   return against+strlen(mytok)+1;

 return NULL;
}

int max (int a, int b)
{
 return (a>b)?a:b;
}

void block (void)
{
 memset(obuf, ' ', obs);
 memcpy(obuf, ibuf, max(obs, strlen((char *)ibuf)));
 if (strlen((char *)ibuf)>obs) truncs++;
}

void unblock (void)
{
 unsigned t;

 memset(obuf, 0, obs);
 for (t=ibs; t&&(ibuf[t-1]==' '); t--);
 memcpy(obuf, ibuf, max(t, obs));
 if (t>obs) truncs++;
}

/* Not "bcopy" because that means "memmove" on BSD. */
void blkcpy (void)
{
 memset(obuf, 0, obs);
 memcpy(obuf, ibuf, max(ibs, obs));
 if (ibs>obs) truncs++;
}

void trans (unsigned char *table)
{
 unsigned t;

 for (t=0; t<ibs; t++) ibuf[t]=table[ibuf[t]];
}

void bucase (void)
{
 unsigned t;

 for (t=0; t<ibs; t++) ibuf[t]=toupper(ibuf[t]);
}

void blcase (void)
{
 unsigned t;

 for (t=0; t<ibs; t++) ibuf[t]=tolower(ibuf[t]);
}

void bswab (void)
{
 unsigned t, u;

 for (t=0; t<ibs; t+=2)
 {
  u=ibuf[t];
  ibuf[t]=ibuf[t+1];
  ibuf[t+1]=u;
 }
}

void synopsis (void)
{
#ifdef __SVR4__
 fprintf (stderr, "%lu+%lu records in\n%lu+%lu records out\n",
          igotf, igotp, ogotf, ogotp);
 if (truncs)
  fprintf (stderr, "%lu truncated record%s\n", truncs, truncs!=1?"s":"");
#else
 fprintf (stderr, "%llu+%llu records in\n%llu+%llu records out\n",
          igotf, igotp, ogotf, ogotp);
 if (truncs)
  fprintf (stderr, "%llu truncated record%s\n", truncs, truncs!=1?"s":"");
#endif
}

void shutdown (void)
{
 if (!isstdin) fclose(in);
 if (!isstdout) fclose(out);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void freakout (int unused)
{
 synopsis();
 shutdown();
 exit(1);
}
#pragma clang diagnostic pop

int process (void)
{
 xlong counter;
 size_t s;

 isstdin=isstdout=0;
 if (fn_in)
 {
  in=fopen(fn_in, "rb");
  if (!in)
  {
   xperror(fn_in);
   exit(1);
  }
 }
 else
 {
  fn_in="(stdin)";
  isstdin=1;
  in=stdin;
 }

 if (fn_out)
 {
  out=fopen(fn_out, (mode&MODE_NOTRUNC)?"ab":"wb");
  if (!out)
  {
   xperror(fn_out);
   exit(1);
  }
 }
 else
 {
  fn_out="(stdout)";
  isstdout=1;
  out=stdout;
 }

 fseek(in, bskip*ibs, SEEK_SET);
 fseek(out, bseek*obs, SEEK_SET);

 counter=bcount;
 while (1)
 {
  if (feof(in)) break;
  if (mode&MODE_BLOCK)
  {
   fgets((char *)ibuf, ibs, in);
   s=strlen((char *)ibuf);
   ibuf[s-1]=0;
  }
  else
   s=fread(ibuf, 1, ibs, in);
  if (!s) continue;
  if (ferror(in))
  {
   if (!(mode&MODE_NOERROR))
   {
    xperror(fn_in?fn_in:"-");
    exit(1);
   }
  }
  if (s<ibs) igotp++; else igotf++;

  /* processing goes here */
  if (mode&MODE_ASCII) trans(convtab_ascii);

  if (mode&MODE_LCASE) blcase();
  if (mode&MODE_UCASE) bucase();
  if (mode&MODE_SWAB) bswab();

  if (mode&MODE_EBCDIC) trans(convtab_ebcdic);
  if (mode&MODE_IBM) trans(convtab_ibm);

  blkcpy();

  if (mode&MODE_UNBLOCK)
   s=fprintf(out, "%s\n", obuf);
  else
   s=fwrite(obuf, 1, ((!(mode&MODE_OSYNC))&&(s<ibs))?s:obs, out);

  if (ferror(out))
  {
   if (!(mode&MODE_NOERROR))
   {
    xperror(fn_out?fn_out:"-");
    exit(1);
   }
  }
  if (s<obs) ogotp++; else ogotf++;
  if (bcount)
  {
   if (!(--counter)) break;
  }
 };

 synopsis();
 shutdown();
 return 0;
}

void getconv (char *p)
{
 char *q, *r, *s;

 q=strdup(p);
 if (!q) scram();

 s=q;
 while (1)
 {
  r=s;
  if (!r) break;

  s=strchr(r, ',');
  if (s)
   *(s++)=0;
  else
   s=r+strlen(r);

  if (!strcmp(r, "ascii"))   {mode|=MODE_ASCII;   continue;}
  if (!strcmp(r, "ebcdic"))  {mode|=MODE_EBCDIC;  continue;}
  if (!strcmp(r, "ibm"))     {mode|=MODE_IBM;     continue;}
  if (!strcmp(r, "block"))   {mode|=MODE_BLOCK;   continue;}
  if (!strcmp(r, "unblock")) {mode|=MODE_UNBLOCK; continue;}
  if (!strcmp(r, "lcase"))   {mode|=MODE_LCASE;   continue;}
  if (!strcmp(r, "ucase"))   {mode|=MODE_UCASE;   continue;}
  if (!strcmp(r, "swab"))    {mode|=MODE_SWAB;    continue;}
  if (!strcmp(r, "noerror")) {mode|=MODE_NOERROR; continue;}
  if (!strcmp(r, "notrunc")) {mode|=MODE_NOTRUNC; continue;}
  if (!strcmp(r, "sync"))    {mode|=MODE_SYNC;    continue;}
  if (!strcmp(r, "isync"))   {mode|=MODE_SYNC;    continue;}
  if (!strcmp(r, "osync"))   {mode|=MODE_OSYNC;   continue;}
  fprintf (stderr, "%s: unknown 'conv' subcommand '%s'\n", progname, r);
  exit(1);
 }

 free(q);
}

int main (int argc, char **argv)
{
 int t;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /*
  * Set defaults:
  *   Filenames to NULL
  *   Block sizes to 512
  *   Skip, seek and count to 0
  *   I/O counters to 0
  *   Mode to default
  */
 fn_in=fn_out=NULL;
 ibs=obs=cbs=512;
 bskip=bseek=bcount=0;
 igotf=igotp=ogotf=ogotp=truncs=0;
 mode=0;

 for (t=1; t<argc; t++)
 {
  char *p;

  if (NULL!=(p=findtok(argv[t], "if")))    {fn_in=p;                continue;}
  if (NULL!=(p=findtok(argv[t], "of")))    {fn_out=p;               continue;}
  if (NULL!=(p=findtok(argv[t], "bs")))    {ibs=obs=cbs=
                                                      getsize(p,0); continue;}
  if (NULL!=(p=findtok(argv[t], "ibs")))   {ibs=getsize(p,0);       continue;}
  if (NULL!=(p=findtok(argv[t], "obs")))   {obs=getsize(p,0);       continue;}
  if (NULL!=(p=findtok(argv[t], "cbs")))   {cbs=getsize(p,0);       continue;}
  if (NULL!=(p=findtok(argv[t], "skip")))  {bskip=getsize(p,1);     continue;}
  if (NULL!=(p=findtok(argv[t], "seek")))  {bseek=getsize(p,1);     continue;}
  if (NULL!=(p=findtok(argv[t], "count"))) {bcount=getsize(p,0);    continue;}
  if (NULL!=(p=findtok(argv[t], "conv")))  {getconv(p);             continue;}
  fprintf (stderr, "%s: invalid command '%s'\n", progname, argv[t]);
  exit(1);
 }

 /* Sanity check */
 switch (mode&(MODE_ASCII|MODE_EBCDIC|MODE_IBM))
 {
  case 0:
  case MODE_ASCII:
  case MODE_EBCDIC:
  case MODE_IBM:
   break;
  default:
   fprintf (stderr, "%s: 'ascii', 'ebcdic' and 'ibm' are mutually exclusive\n",
            progname);
   return 1;
 }

 if ((mode&(MODE_LCASE|MODE_UCASE))==(MODE_LCASE|MODE_UCASE))
 {
  fprintf (stderr,
           "%s: 'lcase' and 'ucase' are mutually exclusive\n", progname);
  return 1;
 }

 if ((mode&(MODE_BLOCK|MODE_UNBLOCK))==(MODE_BLOCK|MODE_UNBLOCK))
 {
  fprintf (stderr,
           "%s: 'block' and 'unblock' are mutually exclusive\n", progname);
  return 1;
 }

 /* Allocate buffers. */
 if (NULL==(ibuf=malloc(ibs))) scram();
 if (NULL==(obuf=malloc(obs))) scram();
 if (NULL==(cbuf=malloc(cbs))) scram();

 signal(SIGINT, freakout);
 t=process();

 free(ibuf);
 free(obuf);
 free(cbuf);

 return t;
}
