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
 * This is not (yet) a complete implementation of login(1).
 * It can only handle checking credentials and chaining into the login shell.
 * Not every possible security gotcha is accounted for either.
 * 
 * shadow.h and its functions seem to be specific to SVR4 and Linux.
 */

#include <crypt.h>
#include <pwd.h>
#include <shadow.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <utmpx.h>

#ifndef DEFAULT_PATH
#define DEFAULT_PATH "/bin:/usr/bin"
#endif

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

extern char **environ;

char buf[BUFSIZ];

/*
 * Linux provides a clearenv(3), but if we use it, env(1) will segvee.
 * Instead, provide a more "proper" clearenv(3) that will not segvee env(1).
 */
char	*cleanenv[1];

/* Actual return is unused. */
int clearenv (void)
{
 environ=cleanenv;
 cleanenv[0]=NULL;
 return 0;
}

void urrrk (void)
{
 fprintf (stderr, "Nope.\n");
 exit(1);
}

void agggh (void)
{
 fprintf (stderr, "Init failure\n");
 exit(1);
}

/* Display the MOTD, if we have one. */
void motd (void)
{
 FILE *file;
 
 file=fopen("/etc/motd", "rt");
 if (!file) return;
 
 while (1)
 {
  fgets (buf, BUFSIZ-1, file);
  if (feof(file))
  {
   fclose(file);
   return;
  }
  fputs (buf, stdout);
 }
}

/*
 * Gets a password and checks it.
 * Presence of the password in memory is minimized and not only do we zot it
 * from memory ASAP, we also zot over it 5 times with different bitpatterns to
 * make absolutely sure it's gone.
 * 
 * Returns 0 on success, 1 on failure; but if the failure is fatal, just die
 * screaming rather than let the program take care of it.
 */
int validate (char *user)
{
 int e;
 struct passwd *p;
 struct spwd *spwd;
 struct termios termios;
 char *iter;
 
 /* Highway to the danger zone... Gonna take you right into the danger zone */
 if (!user) agggh();
 p=getpwnam(user);
 e=tcgetattr(0, &termios);
 if (e<0) agggh();
 termios.c_lflag &= ~ECHO;
 e=tcsetattr(0, TCSANOW, &termios);
 if (e<0) agggh();
 e=1;
 printf ("password:");
 fgets(buf, BUFSIZ-1, stdin);
 
 /*
  * WE ARE NOW IN THE DANGER ZONE. PASSWORD PRESENT IN MEMORY UNENCRYPTED
  * It is absolutely VITAL that we minimize the amount of time this is the
  * case, and that we minimize what might potentially be able to duplicate it.
  */

 for (iter=buf;*iter;iter++); --iter; *iter=0;
 if (p)
 {
  spwd=getspnam(p->pw_name);
  if (spwd)
   e=strcmp(spwd->sp_pwdp, crypt(buf, spwd->sp_pwdp));
 }
 
 /* OUT OUT DAMNED SPOT */
 memset(buf, 0, BUFSIZ);
 memset(buf, 0xAA, BUFSIZ);
 memset(buf, 0x55, BUFSIZ);
 memset(buf, 0xFF, BUFSIZ);
 memset(buf, 0, BUFSIZ);
 
 /*
  * OK, we zotted the plaintext copy, now zot the other one.
  * (This isn't necessary if spwd is a null pointer: there's nothing to zot.)
  */
 if (spwd)
 {
  memset(spwd, 0, sizeof(struct spwd));
  memset(spwd, 0x55, sizeof(struct spwd));
  memset(spwd, 0xAA, sizeof(struct spwd));
  memset(spwd, 0xFF, sizeof(struct spwd));
  memset(spwd, 0, sizeof(struct spwd));
 }
 
 /* OK, back to normalcy */
 termios.c_lflag |= ECHO;
 tcsetattr(0, TCSANOW, &termios);
 printf ("\n");
 if (e) return 1;
 
 return 0;
}

int main (int argc, char **argv)
{
 struct passwd *p;
 int tock;
 char *x;
 char *termtype;
 char localbuf[128]; /* does anyone have a username/hostname THAT long? */
 struct utmpx utmpx;
 
 if (geteuid())
 {
  fprintf (stderr, "I need root\n");
  return 127;
 }
 
 signal(SIGQUIT, SIG_IGN);
 signal(SIGINT, SIG_IGN);
 
 termtype=strdup(getenv("TERM"));
 
 clearenv();
 
 tock=3;
 while (tock--)
 {
  if (!gethostname(localbuf, 127))
   printf ("%s ", localbuf);
  printf ("login:");
  fgets(localbuf, 127, stdin);
  localbuf[strlen(localbuf)-1]=0;
  if (!*localbuf) {tock++; continue;}
  if (!validate(localbuf)) break;
  printf ("Denied.\n");
 }
 if (tock<0)
 {
  printf ("Goodbye.\n");
  return 127;
 }
 p=getpwnam(localbuf);
 setgid(p->pw_gid);
 setuid(p->pw_uid);
 
 /* We are now in luser mode. */
 if (chdir(p->pw_dir))
 {
  printf ("User %s's home folder '%s' was nuked\n",
           p->pw_name, p->pw_dir);
  if (chdir(p->pw_dir="/"))
   exit(0);
  printf ("(using / instead)\n");
 }

 /* Skip over the SCCS blat and display the copyright message. */
 printf ("%s", copyright+5);
 
 /* Just in case... */
 if (!(p->pw_shell))
  p->pw_shell="/bin/sh";
 
 if (!(*(p->pw_shell)))
  p->pw_shell="/bin/sh";

 x=strrchr(p->pw_shell, '/');
 if (x) x++; else x=p->pw_shell;
 sprintf (localbuf, "-%s", x);

 /* Set some default environment variables. */
 setenv("SHELL",p->pw_shell,1);
 setenv("HOME",p->pw_dir,1);
 setenv("LOGNAME",p->pw_name,1);
 setenv("USER",p->pw_name,1);
 setenv("PATH",DEFAULT_PATH,1);
 if (termtype) 
 {
  setenv("TERM",termtype,1);
  free(termtype);
 }
 
 motd();
 
 memset(&utmpx, 0, sizeof(utmpx));
 utmpx.ut_type=USER_PROCESS;
 utmpx.ut_pid=getpid();
/*
 if (ttyname(0)
  strcpy(utmpx.ut_line, (ttyname(0))+5);
 strcpy(utmpx.ut_user, p->pw_name);
 */
 utmpx.ut_tv.tv_sec=time(0);

 signal(SIGQUIT, SIG_DFL);
 signal(SIGINT, SIG_DFL);
 
 /* XXX: Register a login in utmp at this point */
 
 /* Chain into the shell. */
 execl(p->pw_shell, localbuf, 0);
 printf ("Failed to chain into login shell.\n");
 return 127;
}
