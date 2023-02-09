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

#include <crypt.h>
#include <pwd.h>
#include <shadow.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

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

/*
 * Gets a password and checks it.
 * Presence of the password in memory is minimized and not only do we zot it
 * from memory ASAP, we also zot over it 5 times with different bitpatterns to
 * make absolutely sure it's gone.
 * 
 * Returns 0 on success, 1 on failure; but if the failure is fatal, just die
 * screaming rather than let the program take care of it.
 */
int validate (void)
{
 int e;
 struct passwd *p;
 struct spwd *spwd;
 struct termios termios;
 char *iter;
 
 /* Highway to the danger zone... Gonna take you right into the danger zone */
 p=getpwuid(0);
 if (!p)
 {
  fprintf (stderr, 
           "Cannot read superuser credentials!\NForcing maintenance mode.\n");
  return 0;
 }
 e=tcgetattr(0, &termios);
 termios.c_lflag &= ~ECHO;
 e=tcsetattr(0, TCSANOW, &termios);
 e=1;
 printf ("Press <Ctrl-D> to resume normal startup\n"
         "(or enter root password for system maintenance): ");
 fflush(stdin);
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

 if ((argc!=1)||(geteuid()))
 {
  fprintf (stderr, "This program should never be invoked directly.\n");
  return 1;
 }

 signal(SIGQUIT, SIG_IGN);
 signal(SIGINT, SIG_IGN);
 
 clearenv();
 while (1)
 {
  if (!validate()) break;
  if (feof(stdin)) return 1;
  printf ("Login incorrect\n");
 }
 p=getpwuid(0);
 setgid(p->pw_gid);
 setuid(p->pw_uid);
 signal(SIGQUIT, SIG_DFL);
 signal(SIGINT, SIG_DFL);
 printf ("Entering system maintenance mode.\n");
 execl ("/bin/sh", "-sh", "-", 0);
 fprintf (stderr, "sulogin: Internal error: could not exec /bin/sh\n");
 return 1;
}
