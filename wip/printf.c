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

/* This code is very buggy and "%b" is unimplemented. */

/* Extension beyond Posix, octal values do not need to start with 0. */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copyright="@(#) (C) Copyright 2023 S. V. Nickolas\n";

static char *progname;

int main (int argc, char **argv)
{
 int t;
 int o;
 char *i, *j;
 char buf[BUFSIZ];
 char fbuf[BUFSIZ];
 enum
 {
  STATE_NORMAL,
  STATE_BACKSLASH,
  STATE_PERCENT,
  STATE_OCTAL
 } state, state2;
 
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 memset(buf, 0, BUFSIZ);
 state=STATE_NORMAL;
 
 if (argc==1)
 {
  fprintf (stderr, "%s: usage: %s format [args ...]\n", progname, progname);
  return 1;
 }
 
 t=2;
 
 while (1)
 {
  i=argv[1];
  
  while (*i)
  {
   switch (state)
   {
    case STATE_NORMAL:
     if (*i=='%')
     {
      i++;
      state=STATE_PERCENT;
      memset(fbuf, 0, BUFSIZ);
      *fbuf='%';
      continue;
     }
     if (*i=='\\')
     {
      i++;
      state=STATE_BACKSLASH;
      continue;
     }
     buf[strlen(buf)]=*i++;
     break;
    case STATE_BACKSLASH:
     if ((*i>='0')&&(*i<='7'))
     {
      o=*(i++)-'0';
      state=STATE_OCTAL;
      continue;
     }
     state=STATE_NORMAL;
     switch (*i)
     {
      case '\\':
       buf[strlen(buf)]='\\';
       i++;
       continue;
      case 'a':
       buf[strlen(buf)]='\a';
       i++;
       continue;
      case 'b':
       buf[strlen(buf)]='\b';
       i++;
       continue;
      case 'f':
       buf[strlen(buf)]='\f';
       i++;
       continue;
      case 'n':
       buf[strlen(buf)]='\n';
       i++;
       continue;
      case 'r':
       buf[strlen(buf)]='\r';
       i++;
       continue;
      case 't':
       buf[strlen(buf)]='\t';
       i++;
       continue;
      case 'v':
       buf[strlen(buf)]='\v';
       i++;
       continue;
      default:
       /*
        * Also sprach Posix: "The interpretation of a <backslash> followed by
        * any other sequence of characters is unspecified."
        * 
        * My interpretation: Spew it untouched.
        */
       memcpy(&(buf[strlen(buf)]), i-1, 2);
     }
     i++;
     continue;
    case STATE_OCTAL:
     if ((*i>='0')&&(*i<='7'))
     {
      if (((o<<3)|(*i-'0'))<0xFF)
      {
       o<<=3;
       o|=(*i-'0');
       i++;
       continue;
      }
     }
     buf[strlen(buf)]=o;
     state=STATE_NORMAL;
     /* Do not increment i; go back and rescan it. */
     continue;
    case STATE_PERCENT:
     /*
      * Also sprach Posix: "The a, A, e, E, f, F, g and G conversion specifiers
      * need not be supported." (It encourages them to be supported, but does
      * not require it, and recommends use of awk(1) if FP support is needed.)
      * 
      * Also sprach Posix: "If a character sequence in the format operand
      * begins with a '%' character, but does not form a valid conversion
      * specification, the behavior is unspecified."
      * 
      * My interpretation: Die screaming.
      */
     
     /* % - literal percent sign */
     if (*i=='%')
     {
      buf[strlen(buf)]='%';
      state=STATE_NORMAL;
     }
     
     fbuf[strlen(fbuf)]=*i;
     switch (*i)
     {
      case '+':
      case '-':
      case '.':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'h':
      case 'j':
      case 'l':
      case 't':
      case 'z':
      case 'L':
       i++;
       continue;
      case 'b':
       /*
        * Also sprach Posix: "An additional conversion specifier character, b,
        * shall be supported as follows. The argument shall be taken to be a
        * string that can contain <backslash>-escape sequences."
        * 
        * It also specifies that '\c' should serve as a force terminator.
        * 
        * This code is unfinished.
        */
       if (t<=argc)
       {
        j=argv[t++];
        
        state2=STATE_NORMAL;
        while (*j)
        {
         j++;
        }
       }
       state=STATE_NORMAL;
       continue;
      case 'c':
       sprintf (&(buf[strlen(buf)]), "%c", (t<argc)?argv[t++][0]:0);
       state=STATE_NORMAL;
       i++;
       continue;
      case 'n':
       strcat(fbuf, "d");
       sprintf (&(buf[strlen(buf)]), fbuf, strlen(buf));
       state=STATE_NORMAL;
       i++;
       continue;
      case 'd':
      case 'i': /* do people actually use %i instead of %d? */
      case 'o':
      case 'p': /* really, now? */
      case 'u':
      case 'x':
      case 'X':
       errno=0;
       sprintf (&(buf[strlen(buf)]), fbuf, (t<argc)?strtol(argv[t], 0, 0):0);
       if (errno)
        fprintf (stderr, "%s: bogus numeric value '%s'\n", progname, argv[t]);
       t++;
       state=STATE_NORMAL;
       i++;
       continue;
      case 's':
       sprintf (&(buf[strlen(buf)]), fbuf, (t<argc)?argv[t]:0);
       t++;
       state=STATE_NORMAL;
       i++;
       continue;
       
      /* we don't support "wide chars" or "long strings" */
      case 'C':
      case 'S':
       state=STATE_NORMAL;
     }
     i++;
   }
  }
  if (state==STATE_OCTAL)
   buf[strlen(buf)]=o;
  else if (state!=STATE_NORMAL)
  {
   fprintf (stderr, "%s: ran out of input in the middle of a modifier\n",
            progname);
   return 1;
  }
  
  fwrite(buf, 1, strlen(buf), stdout);
  if (argc<=t) break;
  t++;
 }
 
 return 0;
}
