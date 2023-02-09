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

/*
 * This command is specific to the following operating systems.
 *   FreeBSD
 *   Linux
 *   NetBSD
 *   OpenBSD
 *
 * It may be possible to get it to work on others, but I have not done so.
 */

/*
 * This #include seems to be position-sensitive:
 *   if it is included later, it won't define VT_ACTIVATE for us.
 *   And that would be Very Bad®™©.
 */
#ifdef __FreeBSD__
#include <sys/consio.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <linux/vt.h>
#define DEV_CONSOLE "/dev/console"
#endif

#ifdef __NetBSD__
#include <dev/wscons/wsdisplay_usl_io.h>
#define DEV_CONSOLE "/dev/ttyEcfg"
#endif

#ifdef __OpenBSD__
#include <dev/wscons/wsdisplay_usl_io.h>
#define DEV_CONSOLE "/dev/ttyCcfg"
#endif

static char *copyright="@(#) (C) Copyright 2020, 2023 S. V. Nickolas\n";

static char *progname;

void die (char *x)
{
 fprintf (stderr, "%s: %s: %s\n", progname, x, strerror(errno));
 exit(1);
}

int main (int argc, char **argv)
{
 int e, x, h;

 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 if (argc!=2)
 {
  fprintf (stderr, "%s: usage: %s vt\n", progname, progname);
  return 1;
 }

 /*
  * FreeBSD seems to want us to run this call on handle 0, instead of a
  * dedicated console file.  I don't know if that's 100% accurate, but we'll
  * do that.
  *
  * NetBSD and OpenBSD do things the same as Linux, but they use different
  * filenames and require a different #include to bring in the constants.
  */
#ifndef __FreeBSD__
 h=open(DEV_CONSOLE, O_RDWR);
 if (h==-1) die(DEV_CONSOLE);
#endif
 x=atoi(argv[1]);
#ifdef __FreeBSD__
 e=ioctl(0, VT_ACTIVATE, x);
 if (e) die(progname);
#else
 e=ioctl(h, VT_ACTIVATE, x) || ioctl(h, VT_WAITACTIVE, x);
 if (e) die(DEV_CONSOLE);
#endif
 return 0;
}
