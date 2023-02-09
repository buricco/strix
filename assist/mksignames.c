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
 * This is obviously not suited for cross-compiling and doesn't even work on
 * SVR4 because kill(1) is invoked through its braindead version of /bin/sh,
 * so for SVR4, an actual signames.h is provided.
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char **argv)
{
 FILE *file;
 char buf1[24];
 char buf2[64];
 int t;
 
 printf (
  "/* This header was created automatically for this system. Do not edit */\n"
 );
 
 printf ("#ifndef H_SIGNAMES\n#define H_SIGNAMES\n");
 
 printf ("static const char *signals[]={\n NULL");
 for (t=1; t<
#if defined(__FreeBSD__)||defined(__SVR4__)
             NSIG
#else
             _NSIG
#endif
                  ; t++)
 {
  printf (",\n");
  sprintf (buf1, "kill -l %u", t);
  file=popen(buf1, "r");
  if (file)
  {
   fgets(buf2, 63, file);
   buf2[strlen(buf2)-1]=0; /* strip EOL */
   fclose(file);
   printf (" \"%s\"", buf2);
  }
  else
  {
   perror (buf1);
   printf (" NULL");
  }

 }
 printf ("\n};\n");
 
 printf ("#endif /* H_SIGNAMES */\n");
 return 0;
}
