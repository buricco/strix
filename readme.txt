This is a collection of rewrites of Unix utilities mostly based on System V
Release 4 and POSIX.1:2017 with some BSD, GNU and personal extensions.  Some
support functions, mostly third-party, all under 2 or 3-clause BSD licenses,
are included for specific support on various Unix-like and Unix-based
operating systems (especially generic SVR4).  A few other commands are also
included which are from other such systems.

ipcs and pgrep do Linux-specific things and I don't know how to do them on
other platforms.  chvt adapts itself to Linux, FreeBSD, OpenBSD or NetBSD but
on any other platform you're on your own.

Most of this will roll, with GCC and some messing around with the command
line, on actual SVR4.  grep/egrep needs the Posix regex functions, and SVR4 is
just too old for that (I might try to bring the Posix regex functions to SVR4
or find someone who already has).  touch uses structures that don't exist on
SVR4 and would probably need a fair bit of reworking, certainly not something
I feel like bumbling around in an ancient version of vi for.

ln ignores -L/-P on SVR4; I'm not sure kernel support exists for the feature
in the first place.

Some of these utilities are braindamaged.

Not all of them are tested, certainly not on ancient SVR4.

Only a couple utilities have manuals.

I do include a couple scripts (not makefiles) to help with rolling the tools.
On Dell Unix 2.2, "build5.sh" will build everything that compiles.
Don't use that on a modern *x.  "build.sh" should work on most of those.
Tested: Debian 11 Linux, NetBSD 9.0, FreeBSD 13.1, OpenBSD 7.2 (all AMD64).

rm(1) has a mollyguard.

dummy.sh exists so that commands built into the shell can be exec(3)'d.
