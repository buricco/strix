/*
 * (C) Copyright 2020, 2022, 2023 S. V. Nickolas.
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

#include <stdio.h>

static char *copyright="@(#) (C) Copyright 2020, 2022, 2023 S. V. Nickolas\n";

int main (int argc, char **argv)
{
 int nflag;
 int t;
 int started;

 /*
  * Reset flags.
  *
  * nflag = suppress newline at the end.
  * started = if off, suppresses the space before the argument,
  *   since that is not wanted before the first argument.
  */
 nflag=started=0;

 for (t=1; t<argc; t++)
 {
  int c,s,o;

  /* If NOT first argument, write a space. */
  if (started) putchar(' '); else started=1;

  for (s=o=c=0; argv[t][c]; c++)
  {
   /*
    * Backslash state machine:
    * 0 = default state
    * 1 = last char was a \
    * 2 = character has been modified, ready to print
    * 3 = character has been processed, eat it
    * 4 = building octal code
    *
    * This code can get a bit ugly.
    */
   if ((argv[t][c]=='\\')&&(!s))
   {
    s=1;
    continue;
   }
   else if (s==1)
   {
    switch (argv[t][c])
    {
     case 'a':
      o='\a'; s=2;
      break;
     case 'b':
      o='\b'; s=2;
      break;
     case 'c': /* Suppress EOL */
      nflag=1; s=3;
      break;
     case 'f':
      o='\f'; s=2;
      break;
     case 'n':
      o='\n'; s=2;
      break;
     case 'r':
      o='\r'; s=2;
      break;
     case 't':
      o='\t'; s=2;
      break;
     case 'v':
      o='\v'; s=2;
      break;
     case '\\':
      o='\\'; s=2;
      break;
     case '0':
      o=0; s=4;
      break;
    }
    if (s==2)
    {
     putchar(o);
     s=0;
     continue;
    }
   }
   if (s==4)
   {
    if ((argv[t][c]>='0')&(argv[t][c]<'8'))
    {
     o<<=3;
     o|=(argv[t][c]-'0');
    }
    else
    {
     s=0;
     putchar(o);
    }
   }
   if ((s)&&(s!=4))
   {
    s=0;
    continue;
   }
   if (!s)
   {
    putchar(argv[t][c]);
   }
  }
  if (s==4) putchar(o);
 }

 /* Write end of line (if not suppressed) and return success. */
 if (!nflag) putchar('\n');
 return 0;
}
