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
 * id [user]
 * id -G [-n] user
 * id -g [-nr] user
 * id -u [-nr] user
 *
 * -G all groups
 * -g effective group only
 * -u effective user only
 *   -n string, not uid, if possible
 *   -r real, not effective
 *
 * "groups" emulates id -Gn.
 * "whoami" emulates id -un.
 *
 * (Internally "A" (for "All") is used for -G to avoid confusion with -g so
 * that all the switches can be described in uppercase.)
 */

#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Our support tree provides getgrouplist(3) from 4.4BSD for SVR4. */
#ifdef __SVR4__
#include <getgrouplist.h>
#endif

#define MODE_A 0x01
#define MODE_G 0x02
#define MODE_U 0x04
#define MODE_N 0x08
#define MODE_R 0x10
int mode;

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

struct passwd *p;
struct group *g;
gid_t gids[NGROUPS_MAX+1];
gid_t mygid;
uid_t myuid;

/*
 * id -G
 * Also used for groups(1).
 * Display a list of all groups to which the user belongs.
 */
int ida (char *who)
{
 int e, t;

 /*
  * Get list of groups.
  * SVR4 lacks getgrouplist() but the 4.4BSD function can be used.
  */
 if (who)
 {
  int n;

  n=NGROUPS_MAX;
  e=getgrouplist(who, mygid, gids, &n);
 }
 else
  e=getgroups(NGROUPS_MAX, gids);

 /* Nope. */
 if (e<0)
 {
  perror(progname);
  return 1;
 }

 /* Print space-separated list of groups, name or number. */
 for (t=0; t<e; t++)
 {
  if (t) putchar(' ');

  if (mode&MODE_N)
  {
   g=getgrgid(gids[t]);
   if (g)
    printf ("%s", g->gr_name);
   else
    printf ("%u", gids[t]);
  }
  else
   printf ("%u", gids[t]);
 }

 printf ("\n");
 return 0;
}

/*
 * id -g
 * Just display the GID (or EGID if applicable).
 */
int idg (void)
{
 if (mode&MODE_N)
 {
  g=getgrgid(mygid);
  if (!g)
  {
   perror(progname);
   return 1;
  }

  printf ("%s\n", g->gr_name);
 }
 else
  printf ("%u\n", mygid);
 return 0;
}

/*
 * id -u
 * Also used for whoami(1).
 * Just display the UID (or EUID if applicable).
 */
int idu (void)
{
 if (mode&MODE_N)
 {
  p=getpwuid(myuid);
  if (!p)
  {
   perror(progname);
   return 1;
  }

  printf ("%s\n", p->pw_name);
 }
 else
  printf ("%u\n", myuid);
 return 0;
}

/*
 * Default mode.
 * 
 * Show the appropriate UID, GID and list of groups in "pretty" format, with
 * both the number and name for each.
 *
 * System V's id(1) will only show groups if "-a" is used.  We support that
 * switch for backward-compatibility but ignore it and always show groups.
 */
int iddef (char *who)
{
 int e, t;
 
 if (who)
 {
  int n;

  n=NGROUPS_MAX;
  e=getgrouplist(who, mygid, gids, &n);
 }
 else
  e=getgroups(NGROUPS_MAX, gids);

 if (e<0)
 {
  perror(progname);
  return 1;
 }

 p=getpwuid(myuid);
 if (p)
  printf ("uid=%u(%s) ", myuid, p->pw_name);
 else
  printf ("uid=%u ", myuid);

 g=getgrgid(mygid);
 if (g)
  printf ("gid=%u(%s) ", mygid, g->gr_name);
 else
  printf ("gid=%u ", mygid);

 printf ("groups=");

 for (t=0; t<e; t++)
 {
  if (t)
   putchar(',');

  g=getgrgid(gids[t]);
  if (g)
   printf ("%u(%s)", gids[t], g->gr_name);
  else
   printf ("%u", gids[t]);
 }

 printf ("\n");

 return 0;
}

void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-Ggu [-nr]] [user]\n", progname, progname);
 exit(1);
}

int main (int argc, char **argv)
{
 int e, r, t;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /*
  * whoami takes no parameters, and is equivalent to id -un.
  * groups takes no switches but does take a parameter; it is equivalent to
  *   id -Gn.
  *
  * If we weren't called as one of these, then parse switches.
  * Calling getopt(3) even as groups sets optind and does preliminary syntax
  * checking for us.
  */
 if (!strcmp(progname, "whoami"))
 {
  if (argc!=1)
  {
   fprintf (stderr, "%s: usage: %s\n", progname, progname);
   return 1;
  }
  mode=MODE_U|MODE_N;
 }
 else if (!strcmp(progname, "groups"))
 {
  mode=MODE_A|MODE_N;

  if (-1!=(e=getopt(argc, argv, "")))
  {
   fprintf (stderr, "%s: usage: %s [name]\n", progname, progname);
   return 1;
  }
 }
 else
 {
  mode=0;
  while (-1!=(e=getopt(argc, argv, "Gagunr")))
  {
   switch (e)
   {
    case 'G':
     mode |= MODE_A;
     break;
    case 'g':
     mode |= MODE_G;
     break;
    case 'u':
     mode |= MODE_U;
     break;
    case 'n':
     mode |= MODE_N;
     break;
    case 'r':
     mode |= MODE_R;
     break;
    case 'a': /* undocumented NOP for System V compatibility */
     break;
    default:
     usage();
   }
  }

  /* Only one non-switch parameter allowed. */
  if (argc-optind>1) usage();
 }

 /*
  * If -r is used, then a parameter cannot also be used.
  * (BSD and GNU will simply ignore -r in that case.)
  */
 if (argc>optind)
 {
  if (mode&MODE_R)
  {
   fprintf (stderr, "%s: -r and name are mutually exclusive\n", progname);
   return 1;
  }

  /*
   * Get default UID and GID for the requested user.
   * If no user was specified, get our effective UID and GID
   *   (or with -r, our real UID and GID).
   */
  p=getpwnam(argv[optind]);
  if (!p)
  {
   fprintf (stderr, "%s: %s: no such user\n", progname, argv[optind]);
   return 1;
  }
  mygid=p->pw_gid;
  myuid=p->pw_uid;
 }
 else
 {
  mygid=(mode&MODE_R)?getgid():getegid();
  myuid=(mode&MODE_R)?getuid():geteuid();
 }
 
 /*
  * Now validate our switches.
  * 
  * -n and -r are not valid in the default mode, so if we get either of them,
  * just display the usage diagnostic and die screaming.
  */
 if (!(mode&(MODE_A|MODE_G|MODE_U)))
 {
  if (mode&(MODE_N|MODE_R))
   usage();

  return iddef((argc>optind)?argv[optind]:0);
 }

 /*
  * It does not make sense to accept more than one mode switch, so if we get
  * more than one, die screaming.
  * 
  * id -Gr is invalid.  (BSD and GNU ignore -r; we die screaming.)
  */
 e=mode&(MODE_A|MODE_G|MODE_U);
 if ((e!=MODE_A)&&(e!=MODE_G)&&(e!=MODE_U))
 {
  fprintf (stderr, "%s: -G, -g and -u are mutually exclusive\n", progname);
  return 1;
 }
 if ((mode&(MODE_A|MODE_R))==(MODE_A|MODE_R))
 {
  fprintf (stderr, "%s: -r is incompatible with -G\n", progname);
  return 1;
 }

 /* Chain into the appropriate function for the current mode. */
 switch (mode&(MODE_A|MODE_G|MODE_U))
 {
  case MODE_A:
   return ida((argc>optind)?argv[optind]:0);
  case MODE_G:
   return idg();
  case MODE_U:
   return idu();
  default:
   return 1;
 }
}
