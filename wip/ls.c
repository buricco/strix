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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

/*
 * Does ls(1) REALLY need so many switches?
 * Even Posix has more switches to ls(1) than seems sane, and they tend for a
 * rather barebones implementation.
 * 
 * Some switches are currently partially or totally broken.
 */
                              /***********************************************/
#define MODE_1  0x00000001UL  /* List                                        */
#define MODE_A  0x00000002UL  /* All files but . and ..                      */
#define MODE_C  0x00000004UL  /* Columnar/vertical                           */
#define MODE_F  0x00000008UL  /* Flags                                       */
#define MODE_H  0x00000010UL  /* Resolve links, but only on the command line */
#define MODE_L  0x00000020UL  /* Resolve all links                           */
#define MODE_R  0x00000040UL  /* Recurse                                     */
#define MODE_S  0x00000080UL  /* Size-dominant sort                          */
#define MODE_T  0x00000100UL  /* Full time info in long mode (BSD)           */
#define MODE_a  0x00000200UL  /* All files, even . and ..                    */
#define MODE_b  0x00000400UL  /* Octalize nonprinting characters (SysV)      */
#define MODE_c  0x00000800UL  /* Use ctime instead of mtime                  */
#define MODE_d  0x00001000UL  /* Do not recurse into dirs on command line    */
#define MODE_f  0x00002000UL  /* No sort                                     */
#define MODE_g  0x00004000UL  /* No group field                              */
#define MODE_i  0x00008000UL  /* Display inodes                              */
#define MODE_k  0x00010000UL  /* Kilobyte block size                         */
#define MODE_l  0x00020000UL  /* Long mode                                   */
#define MODE_m  0x00040000UL  /* Stream mode                                 */
#define MODE_n  0x00080000UL  /* Numeric uid and gid                         */
#define MODE_o  0x00100000UL  /* No owner field                              */
#define MODE_p  0x00200000UL  /* Flags, but only for folders                 */
#define MODE_q  0x00400000UL  /* Nonprintable chars become '?'               */
#define MODE_r  0x00800000UL  /* Reverse sort                                */
#define MODE_s  0x01000000UL  /* Show size in blocks                         */
#define MODE_t  0x02000000UL  /* Time-dominant sort                          */
#define MODE_u  0x04000000UL  /* Use atime instead of mtime                  */
#define MODE_x  0x08000000UL  /* Columnar/horizontal                         */
long mode;                    /***********************************************/

int columns;
int nocols, colwid;

int blocksize;
struct stat statbuf;

void xperror (char *filename)
{
 fprintf (stderr, "%s: %s: %s\n", progname, filename, strerror(errno));
}

void scram (void)
{
 fprintf (stderr, "%s: out of memory\n", progname);
 exit(1);
}

/*
 * How many columns do we need to split our table into?
 *   x = the table
 *   count = number of entries
 *   wid = receives the width of each column
 *         (not altered if NULL)
 *   return: number of columns
 */
static int numcols (char **x, int count, int *wid)
{
 int t, m;
 
 m=0;
 
 for (t=0; t<count; t++)
 {
  int l;
  
  l=strlen(x[t])+2;
  if (l>(m+2)) m=l;
 }
 
 if ((m+2)>columns) return 1;
 
 if (wid) *wid=m+2;
 return columns/(m+2);
}

/*
 * How wide is our terminal?
 * If we can't determine by ioctl on stdout, try reading $COLUMNS.
 * If all else fails, return 80.
 */
int getcols (void)
{
 char *p;
 int x;
#ifdef TIOCGWINSZ
 struct winsize winsize;
 
 if (ioctl(1, TIOCGWINSZ, &winsize)!=-1)
  if (winsize.ws_col) return winsize.ws_col;
#endif
 p=getenv("COLUMNS");
 if (!p) return 80;
 errno=0;
 x=strtol(p, 0, 0);
 if (errno) return 80;
 if (x) return x;
 return 80;
}

void strmode (mode_t mode, char *buf)
{
 buf[0]=(mode&S_IFBLK)?'b':
        (mode&S_IFCHR)?'c':
        (mode&S_IFDIR)?'d':
        (mode&S_IFLNK)?'l':
        (mode&S_IFIFO)?'p':
        (mode&S_IFSOCK)?'s':
        (mode&S_IFREG)?'-':'?';
 buf[1]=(mode&S_IRUSR)?'r':'-';
 buf[2]=(mode&S_IWUSR)?'w':'-';
 buf[3]=(mode&S_IXUSR)?((mode&S_ISUID)?'s':'x'):((mode&S_ISUID)?'S':'-');
 buf[4]=(mode&S_IRGRP)?'r':'-';
 buf[5]=(mode&S_IWGRP)?'w':'-';
 buf[6]=(mode&S_IXGRP)?((mode&S_ISGID)?'s':'x'):((mode&S_ISGID)?'S':'-');
 buf[7]=(mode&S_IROTH)?'r':'-';
 buf[8]=(mode&S_IWOTH)?'w':'-';
 buf[9]=(mode&S_IXOTH)?((mode&S_ISVTX)?'t':'x'):((mode&S_ISVTX)?'T':'-');
 buf[10]=' ';
 buf[11]=0;
}

/*
 * Alter the string, so that any non-printing characters are replaced by
 * question marks.
 * 
 * As the size remains the same we can do it in place.  If you want to do it
 * to a copy, strdup() it first.
 */
char *questionize (char *x)
{
 for (; *x; x++)
  if (!isprint(*x)) *x='?';
  
 return x;
}

/*
 * Alter the string, so that any non-printing characters are replaced by
 * backslashed octal codes.  (Not done in place.)
 * 
 * This function, unlike questionize(), cannot operate in-place because it
 * expands the string.  Instead, it malloc()s enough bytes to hold the new
 * string, then copies byte-by-byte, expanding as necessary.
 */
char *octalize (char *x)
{
 char *n;
 int t;
 int expand;
 
 expand=1; /* "Though this be madness, yet there is method in't." */
 for (t=0; x[t]; x++)
  if (!isprint(x[t])) expand+=3;
 
 n=malloc(strlen(x)+expand);
 if (!n) scram();
 
 memset(n, 0, strlen(x)+expand);
 
 for (t=0; x[t]; x++)
 {
  if (isprint(x[t]))
   n[strlen(n)]=x[t];
  else
   sprintf(&(n[strlen(n)]), "\\%03o", x[t]);
 }
 
 return n;
}

/* Dump the entire list as one column. */
void drop1 (char **x, int count)
{
 int t;
 
 for (t=0; t<count; t++)
 {
  printf ("%s\n", x[t]);
 }
}

/* Dump the entire list in "stream" mode. (XXX: sometimes lines overflow) */
void dropm (char **x, int count)
{
 int c, l, t;
 
 c=0;
 for (t=0; t<count; t++)
 {
  if (t) putchar (',');
  
  c++;
  l=strlen(x[t]);
  if ((c+l+3)<columns)
  {
   c+=(l+3);
   putchar(' ');
  }
  else
  {
   c=0;
   putchar('\n');
  }
  printf ("%s", x[t]);
 }
 printf ("\n");
}

/* Dump the entire list as multiple columns (horizontal fill). */
void droph (char **x, int count)
{
 int c, t;
 
 c=0;
 for (t=0; t<count; t++)
 {
  char sl[20]; /* Generous, I hope */
  
  c++;
  if (c==nocols) /* Last column, end with newline */
  {
   c=0;
   sprintf (sl, "%%s\n");
  }
  else /* Not last column, space-pad */
   sprintf (sl, "%%-%us  ", colwid);
  printf (sl, x[t]);
 }
 if (c) printf ("\n");
}

/*
 * Dump the entire list as multiple columns (vertical fill).
 * This works by creating a duplicate array with the entries reordered.
 * After that we just dump the transposed list with droph().
 */
void dropv (char **array, int count)
{
 int longcols, collen;
 int t, y, x;
 char **iltab;
 
 /* Allocate enough memory for a duplicate array, or die trying. */
 iltab=calloc(count, sizeof(char *));
 if (!iltab) scram();

 /*
  * There are "collen+1" columns in the first "longcols" columns
  * and "collen" columns in the last.
  */
 collen=count/nocols;
 longcols=count%nocols;
 for (t=0; t<count; t++)
 {
  int loc;
  x=0;
  
  /*
   * Fill down instead of right.
   * But divide the columns evenly across the bottom, as if we were still
   * filling to the right.  This means not only the last column might be an
   * "off" width, so divide isn't possible...
   */
  y=t;
  while (1)
  {
   int colsize;
   
   colsize=collen+(x<longcols);
   if (y>=colsize)
   {
    x++;
    y-=colsize;
   }
   else break;
  }
  loc=(y*nocols)+x;
  iltab[loc]=array[t];
 }
 
 /* Use droph() to display the transposed table, then chuck it and get lost */
 droph(iltab, count);
 free(iltab);
}

void dropl (char **x, int count)
{
 long total;
 int t;
 char modebuf[12];
 char datebuf[24];
 int caughtlink;
 
 total=0;
 
 /* Come, Mr. Tally Man, tally me banana */
 for (t=0; t<count; t++)
 {
  size_t l;
  
  /* Get block count */
  if (!(lstat(x[t], &statbuf)))
  {
   if (mode&MODE_L)
   {
    if (statbuf.st_mode&S_IFLNK)
    {
     if (stat(x[t], &statbuf)) continue;
    }
   }
   l=statbuf.st_size;
   total+=(l/blocksize);
   if (l%blocksize) total++;
  }
 }
 
 printf ("total %lu\n", total);
 for (t=0; t<count; t++)
 {
  struct passwd *p;
  struct group *g;
  time_t tx;
  
  caughtlink=0;
  if (lstat(x[t], &statbuf)) continue;
  if (mode&MODE_L)
  {
   if (statbuf.st_mode&S_IFLNK)
   {
    caughtlink=1;
    if (stat(x[t], &statbuf)) continue;
   }
  }
  if (mode&MODE_i)
   printf ("%10lu  ", statbuf.st_ino);
  strmode(statbuf.st_mode, modebuf);
  printf ("%s %u ", modebuf, statbuf.st_nlink);
  if (mode&MODE_n)
  {
   p=NULL;
   g=NULL;
  }
  else
  {
   p=getpwuid(statbuf.st_uid);
   g=getgrgid(statbuf.st_gid);
  }
  if (!(mode&MODE_o))
  {
   if (p) 
    printf ("%-8s ", p->pw_name); 
   else 
    printf ("%-10u ", statbuf.st_uid);
  }
  if (!(mode&MODE_g))
  {
   if (g) 
    printf ("%-8s ", g->gr_name); 
   else 
    printf ("%-10u ", statbuf.st_gid);
  }
  printf ("%12llu ", statbuf.st_size);

  /*
   * Lazy approximation of 6 months in seconds (actually 180 days in seconds).
   * Also sprach Posix: if the file is less than 6 months old, show date minus
   * year, plus time, in this exact format; if older, show year instead of
   * time.  There's probably more precise ways to compute 6 months.
   */
  if (mode&MODE_c)
   tx=statbuf.st_ctime;
  else if (mode&MODE_u)
   tx=statbuf.st_atime;
  else
   tx=statbuf.st_mtime;
  if (mode&MODE_T)
  {
   strftime(datebuf, 23, "%a %b %e %H:%M:%S %Y", localtime(&tx));
  }
  else
  {
   if ((time(0)-tx)<15552000) 
    strftime(datebuf, 23, "%b %e %H:%M", localtime(&tx));
   else
    strftime(datebuf, 23, "%b %e %Y", localtime(&tx));
  }
  
  printf ("%-12s %s", datebuf, x[t]);
  if (mode&MODE_p)
  {
   if (statbuf.st_mode&S_IFDIR) printf ("/");
  }
  else if (mode&MODE_F)
  {
   if (statbuf.st_mode&S_IFDIR) printf ("/");
   /* if (statbuf.st_mode&S_IFLNK) printf ("@"); */
   /* if (statbuf.st_mode&S_IFIFO) printf ("|"); */
   /* if (statbuf.st_mode&S_IFSOCK) printf ("="); */
   /* if (access(x[t], X_OK)&&(!(statbuf.st_mode&S_IFDIR))) printf ("*"); */
  }
  if (mode&MODE_l)
  {
   if (caughtlink)
   {
    char lnkto[PATH_MAX+1];
    
    readlink(x[t], lnkto, PATH_MAX);
    printf (" -> %s", lnkto);
   }
  }
  printf ("\n");
 }
}

void sortlist (char **x, size_t *y, time_t *z, int count)
{
 char *p;
 size_t q;
 time_t r;
 int i, j, clean;
 int cmp;
 
 /*
  * Also sprach Posix: Even if we are sorting by date or size, we still need
  * to start out by doing a standard filename sort.
  * 
  * -r reverses the order of all sorting.
  */
 clean=0;
 while (!clean)
 {
  clean=1;
  
  for (i=0; i<count; i++)
  {
   for (j=i+1; j<count; j++)
   {
    if (mode&MODE_r)
     cmp=(strcmp(x[i], x[j])<0);
    else
     cmp=(strcmp(x[i], x[j])>0);
    if (cmp)
    {
     p=x[i]; x[i]=x[j]; x[j]=p;
     q=y[i]; y[i]=y[j]; y[j]=q;
     r=z[i]; z[i]=z[j]; z[j]=r;
     
     clean=0;
    }
   }
  }
 }
 
 if (mode&(MODE_S|MODE_t))
 {
  int cmp;
  clean=0;
  
  while (!clean)
  {
   clean=1;
   
   for (i=0; i<count; i++)
   {
    for (j=i+1; j<count; j++)
    {
     if (mode&MODE_r)
     {
      if (mode&MODE_S) cmp=(y[i]<y[j]);
      if (mode&MODE_t) cmp=(z[i]<z[j]);
     }
     else
     {
      if (mode&MODE_S) cmp=(y[i]<y[j]);
      if (mode&MODE_t) cmp=(z[i]<z[j]);
     }
     
     if (cmp)
     {
      p=x[i]; x[i]=x[j]; x[j]=p;
      q=y[i]; y[i]=y[j]; y[j]=q;
      r=z[i]; z[i]=z[j]; z[j]=r;
      
      clean=0;
     }
    }
   }
  }
 }
}

int dumpdir (char *path)
{
 DIR *dir;
 struct dirent *dirent;
 int e, t;
 int pwdhand;
 int tally;
 char **table;
 
 size_t *table_sizes;
 time_t *table_times;
 
 e=0;
 
 blocksize=512;
 
 pwdhand=open(".", O_DIRECTORY);
 if (!pwdhand)
 {
  xperror("could not gain handle to current directory");
  return 1;
 }
 columns=getcols();
 dir=opendir(path);
 if (!dir) 
 {
  xperror(path);
  close(pwdhand);
  return 1;
 }
 chdir(path);
 
 tally=0;
 while (1)
 {
  dirent=readdir(dir);
  errno=0;
  if (!dirent)
  {
   if (errno)
   {
    xperror(path);
    close(pwdhand);
    return 1;
   }
   break;
  }
  
  if (lstat(dirent->d_name, &statbuf))
  {
   e=1;
   xperror(dirent->d_name);
   continue;
  }
  
  if (!(mode&MODE_a))
   if ((!strcmp(dirent->d_name, "."))||(!strcmp(dirent->d_name, ".."))) 
    continue;
  if (!(mode&(MODE_a|MODE_A)))
   if (*dirent->d_name=='.') continue;
  tally++;
 }
 rewinddir(dir);
 
 table=calloc(tally, sizeof (char*));
 if (!table) scram();
 
 table_sizes=calloc(tally, sizeof (size_t));
 if (!table_sizes) scram();
 
 table_times=calloc(tally, sizeof (time_t));
 if (!table_times) scram();
 
 for (t=0; t<tally; t++)
 {
  dirent=readdir(dir);
  errno=0;
  if (!dirent)
  {
   if (errno)
   {
    xperror(path);
    close(pwdhand);
    return 1;
   }
   break;
  }
  
  /* We already screamed, don't scream again. */
  if (lstat(dirent->d_name, &statbuf))
   continue;
  if (!(mode&MODE_a))
   if ((!strcmp(dirent->d_name, "."))||(!strcmp(dirent->d_name, ".."))) 
   {
    t--;
    continue;
   }
  if (!(mode&(MODE_a|MODE_A)))
   if (*dirent->d_name=='.')
   {
    t--;
    continue;
   }
   
  table[t]=strdup(dirent->d_name);
  if (!table[t]) scram();
  table_sizes[t]=statbuf.st_size;
  table_times[t]=(mode&MODE_c)?statbuf.st_ctime:
                 (mode&MODE_u)?statbuf.st_atime:statbuf.st_mtime;
 }

 nocols=numcols(table, tally, &colwid);
 
 if (!(mode&MODE_f)) 
  sortlist(table, table_sizes, table_times, tally);
 
 if (mode&MODE_1)
  drop1(table, tally);
 else if (mode&MODE_C)
  dropv(table, tally);
 else if (mode&MODE_x)
  droph(table, tally);
 else if (mode&MODE_l)
  dropl(table, tally);
 else if (mode&MODE_m)
  dropm(table, tally);
 else /* DEFAULT MODE */
  dropv(table, tally);
 
 if (mode&MODE_R)
 {
  for (t=0; t<tally; t++)
  {
   if (!((mode&MODE_L)?stat:lstat)(table[t], &statbuf))
   {
    if (statbuf.st_mode&S_IFDIR)
    {
     printf ("\n%s:\n", table[t]);
     e+=dumpdir(table[t]);
     e=!!e;
    }
   }
  }
 }
 
 fchdir(pwdhand);
 close(pwdhand);
 free(table);
 free(table_sizes);
 free(table_times);
 return e;
}

/*
 * We just about puke out the whole alphabet here.  Geez, that's a lot of
 * switches.  Oy bloody vey.
 */
void usage (void)
{
 fprintf (stderr, 
          "%s: usage: %s [-1ACFHLPRSTabcdfgiklmnopqrstux] [path ...]\n",
          progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e, r, t;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 mode=0;
 
 /* Different defaults if invoked as dir or vdir; GNU */
 if (!strcmp(progname, "dir")) mode=MODE_C|mode_b;
 if (!strcmp(progname, "vdir")) mode=MODE_l|mode_b;
 
 while (-1!=(e=getopt(argc, argv, "1ACFHLPRSTabcdfgiklmnopqrstux")))
 {
  switch (e)
  {
   case '1':
    mode |= MODE_1;
    mode &= ~(MODE_C|MODE_m|MODE_x);
    break;
   case 'A':
    mode |= MODE_A;
    mode &= ~(MODE_a);
    break;
   case 'C':
    mode |= MODE_C;
    mode &= ~(MODE_1|MODE_l|MODE_m|MODE_x);
    break;
   case 'F':
    mode |= MODE_F;
    break;
   case 'H':
    mode |= MODE_H;
    mode &= ~(MODE_L);
    break;
   case 'L':
    mode |= MODE_L;
    mode &= ~(MODE_H);
    break;
   case 'P':
    mode &= ~(MODE_H|MODE_L);
    break;
   case 'R':
    mode |= MODE_R;
    break;
   case 'S':
    mode |= MODE_S;
    break;
   case 'T':
    mode |= MODE_T;
    break;
   case 'a':
    mode |= MODE_a;
    mode &= ~(MODE_A);
    break;
   case 'b':
    mode |= MODE_b;
    break;
   case 'c':
    mode |= MODE_c;
    break;
   case 'd':
    mode |= MODE_d;
    break;
   case 'f':
    mode |= MODE_f;
    break;
   case 'g':
    mode |= MODE_g|MODE_l;
    mode &= ~(MODE_C|MODE_m|MODE_x);
    break;
   case 'i':
    mode |= MODE_i;
    break;
   case 'k':
    mode |= MODE_k;
    break;
   case 'l':
    mode |= MODE_l;
    mode &= ~(MODE_C|MODE_m|MODE_x);
    break;
   case 'm':
    mode |= MODE_m;
    mode &= ~(MODE_1|MODE_C|MODE_l|MODE_x);
    break;
   case 'n':
    mode |= MODE_l|MODE_n;
    mode &= ~(MODE_C|MODE_m|MODE_x);
    break;
   case 'o':
    mode |= MODE_l|MODE_o;
    mode &= ~(MODE_C|MODE_m|MODE_x);
    break;
   case 'p':
    mode |= MODE_p;
    break;
   case 'q':
    mode |= MODE_q;
    break;
   case 'r':
    mode |= MODE_r;
    break;
   case 's':
    mode |= MODE_s;
    break;
   case 't':
    mode |= MODE_t;
    break;
   case 'u':
    mode |= MODE_u;
    break;
   case 'x':
    mode |= MODE_x;
    mode &= ~(MODE_1|MODE_C|MODE_l|MODE_m);
    break;
   case '?':
    usage();
  }
 }
 
 /*
  * Also sprach Posix: "The use of -R with -d or -f produces unspecified
  * results."  We are going to handle -Rf absolutely straight, but we will
  * consider -Rd an error condition.  The above description permits this.
  * 
  * As seen above, other cases of duelling switches are "last man standing."
  */
 if ((mode&(MODE_R|MODE_d))==(MODE_R|MODE_d))
 {
  fprintf (stderr, "%s: -R and -d are mutually exclusive\n", progname);
  return 1;
 }
 
 /* If -f set, turn off all other sort-related options. */
 if (mode&MODE_f)
  mode &= ~(MODE_r|MODE_S|MODE_t);
 
 if (argc==optind)
  return dumpdir(".");
 
 r=0;
 for (t=optind; t<argc; t++)
 {
  if (t>optind) printf ("\n");
  if ((argc-optind)>1) printf ("%s:\n", argv[t]);
  e=dumpdir(argv[t]);
  if (e>r) r=e;
 }
 
 return r;
}
