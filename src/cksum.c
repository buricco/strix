/*
 * (C) Copyright 2020, 2022, 2023 S. V. Nickolas.
 *
 * This code is adapted from sample code provided in IEEE 1003.1:2017,
 * copyright (C) 2001-2018 IEEE and The Open Group.  The array "crctab" and
 * algorithmic portions of the function "crcop" are derived from this code.
 * A fair use exemption is assumed.
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

/* Thanks: bluesun on Hoshinet and forty on Virtually Fun's Discord */

/*
 * If invoked as sum or md5, acts as the relevant command.
 * Also:
 *   -a posix  -o 0
 *   -a bsd    -o 1
 *   -a sysv   -o 2
 *   -a md5
 * 
 * XXX: something isn't defined right for SVR4 and bogus MD5 checksums are
 *      generated.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __SVR4__
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#else
#include <stdint.h>
#endif

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

static char *progname;

static unsigned long crctab[] = 
{
 0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B,
 0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD, 0x4C11DB70, 0x48D0C6C7,
 0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75,
 0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3,
 0x709F7B7A, 0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
 0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58, 0xBAEA46EF,
 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
 0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB,
 0xCEB42022, 0xCA753D95, 0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1,
 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0,
 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072,
 0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4,
 0x0808D07D, 0x0CC9CDCA, 0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE,
 0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08,
 0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
 0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC,
 0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6,
 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A, 0xE0B41DE7, 0xE4750050,
 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
 0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34,
 0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637,
 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB, 0x4F040D56, 0x4BC510E1,
 0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53,
 0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5,
 0x3F9B762C, 0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E, 0xF5EE4BB9,
 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,
 0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD,
 0xCDA1F604, 0xC960EBB3, 0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7,
 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71,
 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3,
 0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2,
 0x470CDD2B, 0x43CDC09C, 0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
 0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E,
 0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
 0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A,
 0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0,
 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 0xE3A1CBC1, 0xE760D676,
 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4,
 0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662,
 0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,
 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
};


/* prerror(3) with the name of the utility AND the name of the input file. */
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

uint32_t quadize (uint8_t *x)
{
 uint32_t y;
 
 y=x[3];
 y<<=8;
 y|=x[2];
 y<<=8;
 y|=x[1];
 y<<=8;
 y|=x[0];
 return y;
}

void quad16 (uint32_t *tgt, uint8_t *src)
{
 int t;
 
 for (t=0; t<16; t++)
  tgt[t]=quadize(src+(t<<2));
}

uint32_t roll32 (uint32_t in, uint8_t bits)
{
#ifdef __SVR4__
 uint32_t left, right;
 
 right=in<<bits;
 left=in>>(32-bits);
 return left|right;
#else
 uint64_t work;
 
 work=in;
 work<<=bits;
 return (work&0xFFFFFFFF)|(work>>32);
#endif
}

int crcop (char *filename, unsigned long *retval1, size_t *retval2)
{
 int c;
 int isstdin;
 FILE *file;
 size_t l, t;
 unsigned s;
 
 file=fopen(filename, "rb");
 if (!file)
 {
  xperror(filename);
  return 1;
 }
 
 fseek(file, 0, SEEK_END);
 l=ftell(file);
 fseek(file, 0, SEEK_SET);
 
 *retval2=l;
 
 s=0;
 
 for (t=l; t; t--)
 {
  c=fgetc(file);
  if (c<0)
  {
   if (feof(file)) break;
   xperror(filename);
   fclose (file);
   return 1;
  }

  s=(s<<8)^crctab[(s>>24)^(unsigned) c];
 }
 
 /* Extend with the size of the file. */
 while (l)
 {
  c=l&0xFF;
  l>>=8;
  s=(s<<8)^crctab[(s>>24)^(unsigned) c];
 }
 
 fclose (file);
 *retval1=~s;
 return 0;
}

int do_cksum (char *filename, int suppress)
{
 unsigned long c;
 size_t s;
 int e;
 
 if (!strcmp(filename, "-"))
 {
  FILE *file;
  char foo[20];
  int h;

  /* 19 */
  strcpy(foo, "/var/tmp/cksXXXXXX");

#ifdef __SVR4__ /* DO NOT DO THIS, MAGGOT! */
  file=fopen(mktemp(foo), "w");
#else
  h=mkstemp(foo);
  if (h<0)
  {
   fprintf (stderr, "%s: could not open holding tank for stdin\n", progname);
   exit(1);
  }
  file=fdopen(h, "w");
#endif
  rewind(stdin);
  while ((c=getchar())>=0) fputc(c, file);
  fclose(file);
  e=do_cksum(foo, suppress);
  unlink(foo);
  return e;
 }
 
 e=crcop(filename, &c, &s);
 
 printf ("%lu %lu", c, s);
 if (!suppress)
 {
  if (!strcmp(filename, "-"))
   printf (" (stdin)");
  else
   printf (" %s", filename);
 }
 printf ("\n");
 
 return e;
}

/* BSD algorithm version */
int rsum (FILE *file)
{
 unsigned short c;
 long s;
 unsigned char buf[512];

 c=0;
 s=0;
 while (1)
 {
  int e, t;

  e=fread(buf,1,512,file);
  if (ferror(file)) return 1;
  if (e<=0)
   break;

  s++;
  for (t=0; t<e; t++)
  {
   unsigned short xbit;

   xbit=(c&1)<<15;
   c>>=1;
   c|=xbit;
   c+=buf[t];
  }
  if (e<512) break;
 }

 printf("%.5u%6lu\n", c, s);
 return 0;
}

/* SysV algorithm version */
int sum (FILE *file, char *dispname)
{
 unsigned long c;
 long s;
 unsigned char buf[512];

 c=0;
 s=0;
 while (1)
 {
  int e, t;

  e=fread(buf,1,512,file);
  if (ferror(file)) return 1;
  if (e<=0)
   break;

  s++;
  for (t=0; t<e; t++) c+=buf[t];
  if (e<512) break;
 }

 c=(c&0xFFFF)+(c>>16);

 printf ("%lu %lu %s\n", c, s, dispname);
 return 0;
}

int sum_main (int argc, char **argv)
{
 int e, t, bsd;

 /* Set defaults; detect -r switch (getopt(3) would be overkill here). */
 e=0;
 bsd=0;

 if (argc>1)
 {
  if (argv[1][0]=='-')
  {
   if (argv[1][1]=='r')
   {
    bsd=1;
    argc--;
    argv++;
   }
  }
 }

 /*
  * No arguments: operate on stdin.
  * Otherwise iterate.
  */
 if (argc==1)
 {
  e=(bsd)?rsum(stdin):sum(stdin,"");
 }
 else for (t=1; t<argc; t++)
 {
  FILE *file;

  file=fopen(argv[t],"rb");
  if (!file)
  {
   xperror(argv[t]);
   e++;
  }
  else
  {
   int f;

   f=bsd?rsum(file):sum(file,argv[t]);
   if (f) xperror(argv[t]);
   e+=f;
   fclose(file);
  }
 }

 return e;
}

void md5 (uint8_t *buffer, size_t buflen, uint8_t *output)
{
 uint8_t digest[16];
 uint8_t lastblock[64];
 uint32_t A, B, C, D;
 uint32_t i;
 uint32_t a0, b0, c0, d0;
#ifdef __SVR4__
 uint32_t iter;
 uint32_t buflen_bits;
#else
 uint64_t iter;
 uint64_t buflen_bits;
#endif

 const uint32_t s[64]={
  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
 };

 const uint32_t K[64]={
  0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
  0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
  0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
  0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
  0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
  0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
  0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
  0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
  0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
  0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
  0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
  0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
  0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
  0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
  0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
  0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
 };
 
 a0=0x67452301;
 b0=0xEFCDAB89;
 c0=0x98BADCFE;
 d0=0x10325476;
 
 for (iter=0; iter<buflen; iter+=64)
 {
  uint32_t M[16];
  
  quad16(M, (buffer+iter));
  
  A=a0; B=b0; C=c0; D=d0;

  for (i=0; i<64; i++)
  {
   uint32_t F, g;
   
   if (i<16)
   {
    F=(B&C)|((~B)&D);
    g=i;
   }
   else if (i<32)
   {
    F=(D&B)|((~D)&C);
    g=((i*5)+1)%16;
   }
   else if (i<48)
   {
    F=B^C^D;
    g=((3*i)+5)%16;
   }
   else if (i<64)
   {
    F=C^(B|(~D));
    g=(7*i)%16;
   }
   F+=A+K[i]+M[g];
   A=D;
   D=C;
   C=B;
   B+=roll32(F, s[i]);
  }
  a0+=A;
  b0+=B;
  c0+=C;
  d0+=D;
 }

 /* Truncate into bytes, force low endian */
 digest[0x0]=a0; digest[0x1]=a0>>8; digest[0x2]=a0>>16; digest[0x3]=a0>>24;
 digest[0x4]=b0; digest[0x5]=b0>>8; digest[0x6]=b0>>16; digest[0x7]=b0>>24;
 digest[0x8]=c0; digest[0x9]=c0>>8; digest[0xA]=c0>>16; digest[0xB]=c0>>24;
 digest[0xC]=d0; digest[0xD]=d0>>8; digest[0xE]=d0>>16; digest[0xF]=d0>>24;
 
 memcpy(output, digest, 16);
}

int do_md5 (char *filename, int suppress)
{
 int c, e;
 FILE *file;
 char *slurp;
 size_t l, lx;
 uint8_t mymd5[16];

#ifdef __SVR4__
 uint32_t lb;
#else
 uint64_t lb;
#endif
 
 if (!strcmp(filename, "-"))
 {
  char foo[19];
  int h;

  /* 19 */
  strcpy(foo, "/var/tmp/md5XXXXXX");

#ifdef __SVR4__ /* DO NOT DO THIS, MAGGOT! */
  file=fopen(mktemp(foo), "w");
#else
  h=mkstemp(foo);
  if (h<0)
  {
   fprintf (stderr, "%s: could not open holding tank for stdin\n", progname);
   exit(1);
  }
  file=fdopen(h, "w");
#endif
  rewind(stdin);
  while ((c=getchar())>=0) fputc(c, file);
  fclose(file);
  e=do_md5(foo, 1);
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
 lx=l+9;
 if (lx%64) lx+=64-(lx%64);
 slurp=calloc(lx, 1);
 
 if (!slurp) scram();
 if (fread(slurp, 1, l, file)<l)
 {
  free(slurp);
  perror(filename);
  return 1;
 }
 fclose(file);

 slurp[l]=0x80;
 lb=l<<3;
 
 slurp[lx-8]=lb;
 slurp[lx-7]=lb>>8;
 slurp[lx-6]=lb>>16;
 slurp[lx-5]=lb>>24;
#ifdef __SVR4__
 memset(&(slurp[lx-4]), 0, 4);
#else
 slurp[lx-4]=lb>>32;
 slurp[lx-3]=lb>>40;
 slurp[lx-2]=lb>>48;
 slurp[lx-1]=lb>>56;
#endif

 md5((uint8_t *) slurp, lx, mymd5);
 free(slurp);
 
 for (c=0; c<16; c++) printf ("%02x", mymd5[c]);
 if (!suppress) printf ("  %s", filename);
 printf ("\n");

 return 0;
}

int md5_main (int argc, char **argv)
{
 int e, r, t;
 
 if (argc==1) return do_md5("-", 0);

 r=0;
 for (t=1; t<argc; t++)
 {
  e=do_md5(argv[t], 0);
  if (r<e) r=e;
 }

 return r;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-a name | -o {1 | 2}] [file ...]\n", 
          progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int a, e, m, r, t;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (!strcmp(progname, "md5"))
  return md5_main(argc, argv);
 
 if (!strcmp(progname, "sum")) 
  return sum_main(argc, argv);

 a=m=0;
 while (-1!=(e=getopt(argc, argv, "a:o:")))
 {
  switch (e)
  {
   case 'a':
    a|=1;
    if (!strcmp(optarg, "posix"))
     m=0;
    if (!strcmp(optarg, "bsd"))
     m=1;
    if (!strcmp(optarg, "sysv"))
     m=2;
    if (!strcmp(optarg, "md5"))
     m=3;
    break;
   case 'o':
    a|=2;
    if (!strcmp(optarg, "0"))
     m=0;
    else if (!strcmp(optarg, "1"))
     m=1;
    else if (!strcmp(optarg, "2"))
     m=2;
    break;
   default:
    usage();
  }
 }
 
 if (a==3)
 {
  fprintf (stderr, "%s: -a and -o are mutually exclusive\n", progname);
  return 1;
 }
 
 /* 1: rsum(), 2: sum() */
 
 if (m==1)
 {
  if (argc==optind) return rsum(stdin);
  for (t=optind; t<argc; t++)
  {
   FILE *file;

   file=fopen(argv[t],"rb");
   if (!file)
   {
    xperror(argv[t]);
    e++;
   }
   else
   {
    int f;

    f=rsum(file);
    if (f) xperror(argv[t]);
    e+=f;
    fclose(file);
   }
  }
  return e;
 }
 
 if (m==2)
 {
  if (argc==optind) return sum(stdin, argv[t]);
  for (t=optind; t<argc; t++)
  {
   FILE *file;

   file=fopen(argv[t],"rb");
   if (!file)
   {
    xperror(argv[t]);
    e++;
   }
   else
   {
    int f;

    f=sum(file, argv[t]);
    if (f) xperror(argv[t]);
    e+=f;
    fclose(file);
   }
  }
  return e;
 }
 
 if (m==3) return md5_main(argc-(optind-1) , argv+(optind-1));
 
 if (argc==optind) return do_cksum("-", 1);
 
 r=0;
 for (t=optind; t<argc; t++)
 {
  e=do_cksum(argv[t], 0);
  if (r<e) r=e;
 }
 return r;
}
