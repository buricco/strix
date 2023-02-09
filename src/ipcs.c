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
 * The one thing I haven't figured out yet...
 *
 * Linux has modes to msgctl(2), shmctl(2) and semctl(2) that provide a
 * maximum entries count, and these are explicitly described in TFM as
 * extensions.  The process I use - just iterate from 0 to max and ignore
 * anything that EINVALs - is also described in TFM.
 *
 * How the Hanoi do I do the same thing on NetBSD?
 *
 * Also, this code is ugly.
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef linux
#define key __key
#else
#define key _key
#endif

#ifndef BUFSIZ
#define BUFSIZ 8192 /* way more than enough */
#endif

#define NBUFSIZ 20

static char lbuf[BUFSIZ];
static char nbuf1[NBUFSIZ], nbuf2[NBUFSIZ];
static char mbuf[12];

                    /************************/
#define FLAG_Q 0x01 /* Message queues       */
#define FLAG_M 0x02 /* Shared memory blocks */
#define FLAG_S 0x04 /* Semaphores           */
#define FLAG_B 0x08 /* Bytes                */
#define FLAG_C 0x10 /* Creators             */
#define FLAG_O 0x20 /* Outstanding          */
#define FLAG_P 0x40 /* PIDs                 */
#define FLAG_T 0x80 /* Times                */
int flags;          /************************/

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

/*
 * Our knowledge of modes is somewhat limited (and comes from both Linux and
 * NetBSD documentation), but does not tell us everything that ipcs is meant
 * to reveal.  Return - as stubs.
 */
void stuffbuf (int mode)
{
 sprintf (mbuf, "%c%c%c%c%c%c%c%c%c%c%c",
          '-' /* waiting for msgsnd() - we can't know that */,
          '-' /* waiting for msgrcv() - we can't know that */,
          (mode&0400)?'r':'-',   /* Owner read */
          (mode&0200)?'w':'-',   /* Owner write */
          (mode&0100)?'a':'-',   /* Owner alter (not on Linux) */
          (mode&0040)?'r':'-',   /* Group read */
          (mode&0020)?'w':'-',   /* Group write */
          (mode&0010)?'a':'-',   /* Group alter (not on Linux) */
          (mode&0004)?'r':'-',   /* Anon read */
          (mode&0002)?'w':'-',   /* Anon write */
          (mode&0010)?'a':'-');  /* Anon alter (not on Linux) */
}

/*
 * Be very careful how you use these: they use two shared buffers,
 * overwritten by each call, for UID numbers.
 */
char *finduser (uid_t uid)
{
 struct passwd *p;

 p=getpwuid(uid);
 if (p) return p->pw_name;

 snprintf(nbuf1, NBUFSIZ-1, "%u", uid);
 return nbuf1;
}

char *findgroup (gid_t gid)
{
 struct group *g;

 g=getgrgid(gid);
 if (g) return g->gr_name;

 snprintf(nbuf2, NBUFSIZ-1, "%u", gid);
 return nbuf2;
}

/* Heading for each section of the output. */
void header (const char *foo, int yn)
{
 printf ("\n%s%s\n", foo, yn?":":" facility not in system.");
}

/*
 * For the following 3 functions the same Linux-specific technique is used to
 * procure IDs to check: get the maximum from a control call (non-portable),
 * then ignore EINVAL errors and continue since not all IDs are actually
 * meaningful.
 *
 * I don't yet have a portable equivalent, nor a system-specific one for any
 * other OS (*cough*NetBSD*cough*).
 */

/* -q switch: show message queues */
int do_queues (void)
{
 int e1, e2, t;
 struct msginfo msginfo;
 struct msqid_ds msqbuf;

 /* Do we have this? (MSG_INFO is Linux-specific) */
 e1=msgctl(0, MSG_INFO, (void *)&msginfo);
 if (e1<0)
 {
  header ("Message Queue", 0);
  return 1;
 }
 header ("Message Queues", 1);

 memset(lbuf, 0, BUFSIZ);
 strcpy (lbuf,
         "T  ID        KEY                 MODE         OWNER     GROUP     ");
 if (flags&FLAG_C)
  strcat (lbuf, "CREATOR   CGROUP    ");
 if (flags&FLAG_O)
  strcat (lbuf, "QNUM      ");
 if (flags&FLAG_B)
  strcat (lbuf, "QBYTES    ");
 if (flags&FLAG_P)
  strcat (lbuf, "LSPID     LRPID     ");
 if (flags&FLAG_T)
  strcat (lbuf, "STIME     RTIME     CTIME     ");
 while (lbuf[strlen(lbuf)-1]==' ') lbuf[strlen(lbuf)-1]=0;
 printf ("%s\n", lbuf);

 for (t=0; t<e1; t++)
 {
  e2=msgctl(t, IPC_STAT, &msqbuf);
  if (errno==EINVAL) continue; /* just means invalid token */
  if (e2)
  {
   fprintf (stderr, "%s: IPC_STAT failed: %s\n", progname, strerror(errno));
   return 1;
  }

  memset(lbuf, 0, BUFSIZ);
  stuffbuf(msqbuf.msg_perm.mode);
  sprintf (lbuf, "q  %-9u 0x%016x  %s  %-9s %-9s ", t,
           msqbuf.msg_perm.key, mbuf, finduser(msqbuf.msg_perm.uid),
           findgroup(msqbuf.msg_perm.gid));
  if (flags&FLAG_C)
   sprintf (lbuf+strlen(lbuf), "%-9s %-9s ",
            finduser(msqbuf.msg_perm.cuid), findgroup(msqbuf.msg_perm.cgid));
  if (flags&FLAG_O)
   sprintf (lbuf+strlen(lbuf), "%-9lu ", msqbuf.msg_qnum);
  if (flags&FLAG_B)
   sprintf (lbuf+strlen(lbuf), "%-9lu ", msqbuf.msg_qbytes);
  if (flags&FLAG_P)
   sprintf (lbuf+strlen(lbuf), "%-9u %-9u ",
            msqbuf.msg_lspid, msqbuf.msg_lrpid);
  if (flags&FLAG_T)
  {
   if (msqbuf.msg_stime)
   {
    strftime(lbuf+strlen(lbuf), 10, "%k:%M:%S",
             localtime(&(msqbuf.msg_stime)));
    strcat(lbuf, "  ");
   }
   else
   {
    strcat(lbuf, " no-entry ");
   }
   if (msqbuf.msg_rtime)
   {
    strftime(lbuf+strlen(lbuf), 10, "%k:%M:%S",
             localtime(&(msqbuf.msg_rtime)));
    strcat(lbuf, "  ");
   }
   else
   {
    strcat(lbuf, " no-entry ");
   }
   if (msqbuf.msg_ctime)
   {
    strftime(lbuf+strlen(lbuf), 10, "%k:%M:%S",
             localtime(&(msqbuf.msg_ctime)));
    strcat(lbuf, "  ");
   }
   else
   {
    strcat(lbuf, " no-entry ");
   }
  }
  while (lbuf[strlen(lbuf)-1]==' ') lbuf[strlen(lbuf)-1]=0;
  puts (lbuf);
 }

 return 0;
}

/* -m switch: show SHM blocks */
int do_shmsegs (void)
{
 int e1, e2, t;
 struct shminfo shminfo;
 struct shmid_ds shmbuf;

 /* Do we have this? (SHM_INFO is Linux-specific) */
 e1=shmctl(0, SHM_INFO, (void *)&shminfo);
 if (e1<0)
 {
  header ("Shared Memory", 0);
  return 1;
 }
 header ("Shared Memory", 1);

 memset(lbuf, 0, BUFSIZ);
 strcpy (lbuf,
         "T  ID        KEY                 MODE         OWNER     GROUP     ");
 if (flags&FLAG_C)
  strcat (lbuf, "CREATOR   CGROUP    ");
 if (flags&FLAG_O)
  strcat (lbuf, "NATTCH    ");
 if (flags&FLAG_B)
  strcat (lbuf, "SEGSZ     ");
 if (flags&FLAG_P)
  strcat (lbuf, "CPID      LPID      ");
 if (flags&FLAG_T)
  strcat (lbuf, "ATIME     DTIME     CTIME     ");
 while (lbuf[strlen(lbuf)-1]==' ') lbuf[strlen(lbuf)-1]=0;
 printf ("%s\n", lbuf);

 for (t=0; t<e1; t++)
 {
  e2=shmctl(t, IPC_STAT, &shmbuf);
  if (e2)
  {
   if (errno==EINVAL) continue; /* just means invalid token */
   fprintf (stderr, "%s: IPC_STAT failed: %s\n", progname, strerror(errno));
   return 1;
  }

  memset(lbuf, 0, BUFSIZ);
  stuffbuf(shmbuf.shm_perm.mode);
  sprintf (lbuf, "m  %-9u 0x%016x  %s  %-9s %-9s ", t,
           shmbuf.shm_perm.key, mbuf, finduser(shmbuf.shm_perm.uid),
           findgroup(shmbuf.shm_perm.gid));
  if (flags&FLAG_C)
   sprintf (lbuf+strlen(lbuf), "%-9s %-9s ",
            finduser(shmbuf.shm_perm.cuid), findgroup(shmbuf.shm_perm.cgid));
  if (flags&FLAG_O)
   sprintf (lbuf+strlen(lbuf), "%-9lu ", shmbuf.shm_nattch);
  if (flags&FLAG_B)
   sprintf (lbuf+strlen(lbuf), "%-9lu ", shmbuf.shm_segsz);
  if (flags&FLAG_P)
   sprintf (lbuf+strlen(lbuf), "%-9u %-9u ",
            shmbuf.shm_cpid, shmbuf.shm_lpid);
  if (flags&FLAG_T)
  {
   if (shmbuf.shm_atime)
   {
    strftime(lbuf+strlen(lbuf), 10, "%k:%M:%S",
             localtime(&(shmbuf.shm_atime)));
    strcat(lbuf, "  ");
   }
   else
   {
    strcat(lbuf, " no-entry ");
   }
   if (shmbuf.shm_dtime)
   {
    strftime(lbuf+strlen(lbuf), 10, "%k:%M:%S",
             localtime(&(shmbuf.shm_dtime)));
    strcat(lbuf, "  ");
   }
   else
   {
    strcat(lbuf, " no-entry ");
   }
   if (shmbuf.shm_ctime)
   {
    strftime(lbuf+strlen(lbuf), 10, "%k:%M:%S",
             localtime(&(shmbuf.shm_ctime)));
    strcat(lbuf, "  ");
   }
   else
   {
    strcat(lbuf, " no-entry ");
   }
  }
  while (lbuf[strlen(lbuf)-1]==' ') lbuf[strlen(lbuf)-1]=0;
  puts (lbuf);
 }

 return 0;
}

/* -s switch: show semaphores */
int do_semaphores (void)
{
 int e1, e2, t;
 struct seminfo seminfo;
 struct semid_ds sembuf;

 /* Do we have this? (SEM_INFO is Linux-specific) */
 e1=semctl(0, 0, SEM_INFO, &seminfo);
 if (e1<0)
 {
  header ("Semaphore", 0);
  return 1;
 }
 header ("Semaphores", 1);

 memset(lbuf, 0, BUFSIZ);
 strcpy (lbuf,
         "T  ID        KEY                 MODE         OWNER     GROUP     ");
 if (flags&FLAG_C)
  strcat (lbuf, "CREATOR   CGROUP    ");
 if (flags&FLAG_B)
  strcat (lbuf, "NSEMS     ");
 if (flags&FLAG_T)
  strcat (lbuf, "OTIME     CTIME     ");
 while (lbuf[strlen(lbuf)-1]==' ') lbuf[strlen(lbuf)-1]=0;
 printf ("%s\n", lbuf);

 for (t=0; t<e1; t++)
 {
  e2=semctl(t, 0, IPC_STAT, &sembuf);
  if (e2)
  {
   if (errno==EINVAL) continue; /* just means invalid token */
   fprintf (stderr, "%s: IPC_STAT failed: %s\n", progname, strerror(errno));
   return 1;
  }

  memset(lbuf, 0, BUFSIZ);
  stuffbuf(sembuf.sem_perm.mode);
  sprintf (lbuf, "s  %-9u 0x%016x  %s  %-9s %-9s ", t,
           sembuf.sem_perm.key, mbuf, finduser(sembuf.sem_perm.uid),
           findgroup(sembuf.sem_perm.gid));
  if (flags&FLAG_C)
   sprintf (lbuf+strlen(lbuf), "%-9s %-9s ",
            finduser(sembuf.sem_perm.cuid), findgroup(sembuf.sem_perm.cgid));
  if (flags&FLAG_B)
   sprintf (lbuf+strlen(lbuf), "%-9lu ", sembuf.sem_nsems);
  if (flags&FLAG_T)
  {
   if (sembuf.sem_otime)
   {
    strftime(lbuf+strlen(lbuf), 10, "%k:%M:%S",
             localtime(&(sembuf.sem_otime)));
    strcat(lbuf, "  ");
   }
   else
   {
    strcat(lbuf, " no-entry ");
   }
   if (sembuf.sem_ctime)
   {
    strftime(lbuf+strlen(lbuf), 10, "%k:%M:%S",
             localtime(&(sembuf.sem_ctime)));
    strcat(lbuf, "  ");
   }
   else
   {
    strcat(lbuf, " no-entry ");
   }
  }
  while (lbuf[strlen(lbuf)-1]==' ') lbuf[strlen(lbuf)-1]=0;
  puts (lbuf);
 }

 return 0;
}

/* Invalid usage: die screaming */
void usage (void)
{
 fprintf (stderr, "%s: usage: %s [-mqs] [-abcopt]\n", progname, progname);
 exit (1);
}

/* Entry point */
int main (int argc, char **argv)
{
 int e;
 time_t t;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 flags=0;

 while (-1!=(e=getopt(argc, argv, "abcmopqst")))
 {
  switch (e)
  {
   case 'a': /* Same as -bcopt. */
    flags |= (FLAG_B | FLAG_C | FLAG_O | FLAG_P | FLAG_T);
    break;
   case 'b':
    flags |= FLAG_B;
    break;
   case 'c':
    flags |= FLAG_C;
    break;
   case 'M':
    flags |= FLAG_M;
    break;
   case 'o':
    flags |= FLAG_O;
    break;
   case 'p':
    flags |= FLAG_P;
    break;
   case 'q':
    flags |= FLAG_Q;
    break;
   case 's':
    flags |= FLAG_S;
    break;
   case 't':
    flags |= FLAG_T;
    break;
   default:
    usage(); /* NO RETURN */
  }
 }

 /* Default if nothing specified. */
 if (!(flags&(FLAG_M | FLAG_Q | FLAG_S))) flags |= (FLAG_M | FLAG_Q | FLAG_S);

 t=time(NULL);

 /* Newline comes from ctime(3). */
 printf ("IPC status from <running system> as of %s", ctime(&t));

 /* Do each, get return code for each, and return 1 for any failure. */
 e=0;
 if (flags & FLAG_Q) e+=do_queues();
 if (flags & FLAG_M) e+=do_shmsegs();
 if (flags & FLAG_S) e+=do_semaphores();

 return e?1:0;
}
