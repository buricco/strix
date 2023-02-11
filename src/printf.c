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
 * Notes:
 * 
 * Posix requires octal escapes to start \0.  We don't; \1, \2, \3, \4, \5, \6
 * and \7 are also fine (as is a cat) but \8 and \9, obviously, aren't.
 * 
 * Floating point is NOT supported.  Also sprach Posix: It would be nice, but
 * it is not required, just recommended.
 */

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
 int got;
 char *i, *j;
 
 /* Buffer sizes are intentionally excessive. */
 char buf[BUFSIZ];  /* Main buffer   */
 char fbuf[BUFSIZ]; /* Format buffer */
 char bbuf[BUFSIZ]; /* %b buffer     */

 /*
  * state uses all these modes.  state2 (used by %b) only uses NORMAL and
  * OCTAL (with BACKSLASH handled immediately).
  */
 enum
 {
  STATE_NORMAL,
  STATE_BACKSLASH,
  STATE_PERCENT,
  STATE_OCTAL
 } state, state2;
 
 /* Get the basename of the command. */
 progname=strrchr(argv[0], '/');
 if (progname) progname++; else progname=argv[0];

 /* 
  * Zot out the buffer (forcing it to be null-terminated), and set the state
  * machine to its default mode.
  */
 memset(buf, 0, BUFSIZ);
 state=STATE_NORMAL;
 
 /*
  * Mark if we get any % signs, because otherwise, if there are further
  * parameters on the command line, we'll loop.
  */
 got=0;
 
 /* No args: die screaming with usage diagnostic. */
 if (argc==1)
 {
  fprintf (stderr, "%s: usage: %s format [args ...]\n", progname, progname);
  return 1;
 }
 
 /* Argument pointer starts at 2, because 1 is the format string. */
 t=2;
 
 /*
  * Loop until we're out of arguments.  But if there aren't any escapes that
  * require reading arguments, ignore the arguments.
  * 
  * If we run out of arguments before we run out of such escapes, replace any
  * further numeric values with 0 and strings with "".
  */
 while (1)
 {
  i=argv[1];
  
  while (*i)
  {
   switch (state)
   {
    case STATE_NORMAL:
     if (*i=='%') /* Percent escape, to take a parameter */
     {
      got=1;
      i++;
      state=STATE_PERCENT;
      memset(fbuf, 0, BUFSIZ);
      *fbuf='%';
      continue;
     }
     if (*i=='\\') /* Backslash escape */
     {
      i++;
      state=STATE_BACKSLASH;
      continue;
     }
     buf[strlen(buf)]=*i++;
     break;
    case STATE_BACKSLASH:
     if ((*i>='0')&&(*i<='7')) /* Octal escape */
     {
      o=*(i++)-'0';
      state=STATE_OCTAL;
      continue;
     }
     state=STATE_NORMAL;
     
     /*
      * All of these are handled by the C compiler.
      * This comment will not be repeated for its second usage in \b.
      *   \\ = literal backslash    \n = newline
      *   \a = alert (beep)         \r = carriage return
      *   \b = backspace            \t = horizontal tab
      *   \f = form feed            \v = vertical tab
      */
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
    case STATE_OCTAL: /* Keep gathering up an octal code */
     if ((*i>='0')&&(*i<='7'))
     {
      if (((o<<3)|(*i-'0'))<=0xFF) /* Doesn't overflow a byte, add it */
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
     
     /* %% - literal percent sign */
     if (*i=='%')
     {
      buf[strlen(buf)]='%';
      state=STATE_NORMAL;
     }
     
     fbuf[strlen(fbuf)]=*i;
     switch (*i)
     {
      /*
       * Format modifiers.
       * If they form an invalid combo, let sprintf(3) take care of that and
       * the chips will fall where they may...
       */
      case '+':  case '-':  case '.':  case '0':
      case '1':  case '2':  case '3':  case '4':
      case '5':  case '6':  case '7':  case '8':
      case '9':  case 'h':  case 'j':  case 'l':
      case 't':  case 'z':  case 'L':
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
        * Parsing the result further as a %s string is the norm for GNU and
        * FreeBSD implementations (4.4BSD and SVR4 don't know %b yet) plus the
        * versions in bash and the Almquist, Korn and Watanabe shells.
        */
       if (t<=argc)
       {
        int o;
        
        /* Clear the %b buffer and set defaults. */
        memset(bbuf, 0, BUFSIZ);
        
        if (t<argc) 
         j=argv[t++];
        else
         j="";
        
        state2=STATE_NORMAL;
        
        /*
         * Iterate over the matched argument.
         * Relevant comments above regarding backslash and octal escapes also
         * apply here.
         */
        while (*j)
        {
         if (state2==STATE_OCTAL)
         {
          if ((*j>='0')&&(*j<='7'))
          {
           if (((o<<3)|(*j-'0'))<=0xFF)
           {
            o<<=3;
            o|=(*j-'0');
            j++;
            continue;
           }
          }
          bbuf[strlen(buf)]=o;
          state2=STATE_NORMAL;
          /* Do not increment j; go back and rescan it. */
          continue;
         }
         if (*j=='\\')
         {
          j++;
          switch (*j)
          {
           case 0: /* Oops, ran over the edge. */
            strcat(bbuf, "\\");
            continue; /* Let the loop take care of it. */
           case '0':
           case '1':
           case '2':
           case '3':
           case '4':
           case '5':
           case '6':
           case '7':
            state2=STATE_OCTAL;
            o=*j-'0';
            j++;
            continue;
           case 'c': /* Stop parsing immediately, write what we have, die. */
            fbuf[strlen(fbuf)-1]='s';
            sprintf (buf+strlen(buf), fbuf, bbuf);
            fwrite(buf, 1, strlen(buf), stdout);
            exit(0);
           case '\\':
            bbuf[strlen(bbuf)]='\\';
            j++;
            continue;
           case 'a':
            bbuf[strlen(bbuf)]='\a';
            j++;
            continue;
           case 'b':
            bbuf[strlen(bbuf)]='\b';
            j++;
            continue;
           case 'f':
            bbuf[strlen(bbuf)]='\f';
            j++;
            continue;
           case 'n':
            bbuf[strlen(bbuf)]='\n';
            j++;
            continue;
           case 'r':
            bbuf[strlen(bbuf)]='\r';
            j++;
            continue;
           case 't':
            bbuf[strlen(bbuf)]='\t';
            j++;
            continue;
           case 'v':
            bbuf[strlen(bbuf)]='\v';
            j++;
            continue;
           default:
            bbuf[strlen(bbuf)]='\\';
            /* FALL OUT */
          }
         }
         bbuf[strlen(bbuf)]=*(j++);
        }
        fbuf[strlen(fbuf)-1]='s'; /* so modifiers work */
        sprintf (buf+strlen(buf), fbuf, bbuf);
       }
       state=STATE_NORMAL;
       i++;
       continue;
      case 'C': /* Wide char; we don't support so just use regular char */
      case 'c': /* Char */
       fbuf[strlen(fbuf)-1]='c';
       sprintf (&(buf[strlen(buf)]), fbuf, (t<argc)?argv[t++][0]:0);
       state=STATE_NORMAL;
       i++;
       continue;
      case 'n': /* Number of chars "output" so far. */
       fbuf[strlen(fbuf)-1]='d';
       sprintf (&(buf[strlen(buf)]), fbuf, strlen(buf));
       state=STATE_NORMAL;
       i++;
       continue;
      case 'd': /* Decimal */
      case 'i': /* Do people actually use %i instead of %d? */
      case 'o': /* Octal */
      case 'p': /* Really, now? (Pointer) */
      case 'u': /* Unsigned */
      case 'x': /* Hex */
      case 'X': /* Hex, uppercase */
       errno=0;
       /*
        * In this mode, strtol() handles octals and hex values for us, so we
        * don't have to worry about that.
        * 
        * Also sprach Posix: If we're out of parameters, fill in with 0.
        */
       sprintf (&(buf[strlen(buf)]), fbuf, (t<argc)?strtol(argv[t], 0, 0):0);
       if (errno)
        fprintf (stderr, "%s: bogus numeric value '%s'\n", progname, argv[t]);
       t++;
       state=STATE_NORMAL;
       i++;
       continue;
      case 'S': /* Wide string; we don't support so just use regular string */
      case 's': /* String */
       fbuf[strlen(fbuf)-1]='s';
       sprintf (&(buf[strlen(buf)]), fbuf, (t<argc)?argv[t++]:"");
       state=STATE_NORMAL;
       i++;
       continue;
     }
     i++;
   }
  }
  
  /*
   * So, we've hit the end of the format string.
   * 
   * What to do if we hit the end before we finish parsing?  If it's an octal
   * escape, that's fine; the null terminator is our end marker.  Otherwise,
   * die screaming; the format string is malformed.
   */
  if (state==STATE_OCTAL)
   buf[strlen(buf)]=o;
  else if (state!=STATE_NORMAL)
  {
   fprintf (stderr, "%s: ran out of input in the middle of a modifier\n",
            progname);
   return 1;
  }
  
  /*
   * Either we don't have anything that needs argument parsing, or we've hit
   * the end of our argument list, so we're done.  (Otherwise, reset the index
   * so we can go back, Jack, and do it again.)
   */
  if (!((t<argc)&&got)) break;
 }
 
 /* Dump the buffer and run. */
 fwrite(buf, 1, strlen(buf), stdout);
 return 0;
}
