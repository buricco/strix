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

#include <signal.h>
#include <string.h>
#include "signames.h"

#if defined(__FreeBSD__)||defined(__SVR4__)
#define SIGMAX NSIG
#else
#define SIGMAX _NSIG
#endif

#ifdef __SVR4__
#include <ctype.h>

static int strcasecmp (const char *a, const char *b)
{
 char *aa, *bb;

 aa=(char *)a; bb=(char *)b;
 while (toupper(*aa++)==toupper(*bb++))
  if (!*aa) return 0; /* Match. */
 bb--;
 return *aa-*bb;
}
#endif

int matchsig (char *foo)
{
 int t;
 
 for (t=1; t<SIGMAX; t++)
 {
  if (!(signals[t])) continue;
  if (!strcasecmp(foo, signals[t])) return t;
 }
 return 0;
}

char *unmatchsig (int foo)
{
 if (!foo) return 0;
 if (foo<SIGMAX) return (char *)signals[foo];
 return 0;
}
