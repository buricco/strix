#!/bin/sh
#
# (C) Copyright 2023 S. V. Nickolas.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#   1. Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimer.
#   2. Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED.
# 
# IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Build script for Linux and reasonably modern systems.
# Do not use this on old SVR4-based Unices, except maybe Solaris.
#
# ipcs and pgrep dig a bit too deep into the bowels of Linux and will not roll
# anywhere else (this needs to be fixed, especially for ipcs).

[ "$0" = "../build.sh" ] && {
 cd ..
 exec ./build.sh
}

[ "`basename "$0"`" = "build.sh" ] || {
 echo "Cowardly refusing to run under a name other than build.sh."
 exit 1
}

UNAME="`uname -s`"
[ "$UNAME" != Linux -a "$UNAME" != "OpenBSD" -a "$UNAME" != "NetBSD" -a "$UNAME" != "FreeBSD" ] && {
 echo "I don't know this OS, edit this script or try build5.sh"
 exit 1
}

[ -n "$1" ] && [ "$1" = "clean" ] && {
 rm -f support/signames.h
 rm -rf bin obj lib
 exit 0
}

[ -z $CC ] && CC=cc
[ -z $AR ] && AR=ar

cd assist
$CC -o mksignames mksignames.c
./mksignames > ../support/signames.h
rm mksignames
cd ..

mkdir bin 2>/dev/null
mkdir obj 2>/dev/null
mkdir lib 2>/dev/null
cd support/librfc6234
$CC -c -o ../../obj/hkdf.o hkdf.c
$CC -c -o ../../obj/hmac.o hmac.c
$CC -c -o ../../obj/sha1.o sha1.c
$CC -c -o ../../obj/sha224-256.o sha224-256.c
$CC -c -o ../../obj/sha384-512.o sha384-512.c
$CC -c -o ../../obj/usha.o usha.c
cd ../../obj
$AR r ../lib/librfc6234.a hkdf.o hmac.o sha1.o sha224-256.o sha384-512.o usha.o
cd ../src
cp true.sh ../bin/true
cp false.sh ../bin/false
cp mvdir.sh ../bin/mvdir
$CC -o ../bin/asa asa.c
$CC -o ../bin/banner banner.c
$CC -o ../bin/basename basename.c
$CC -o ../bin/cal cal.c
$CC -o ../bin/cat cat.c
[ "$UNAME" = Linux ] && $CC -I../support -o ../bin/chmod chmod.c ../support/setmode.c
[ "$UNAME" = Linux ] || $CC -o ../bin/chmod chmod.c
$CC -o ../bin/chown chown.c
$CC -o ../bin/chroot chroot.c
$CC -o ../bin/chvt chvt.c
$CC -o ../bin/cksum cksum.c
$CC -o ../bin/cmp cmp.c
$CC -o ../bin/comm comm.c
$CC -o ../bin/cp cp.c
$CC -o ../bin/date date.c
$CC -o ../bin/dd dd.c
$CC -o ../bin/dircmp dircmp.c
$CC -o ../bin/dispgid dispgid.c
$CC -o ../bin/dispuid dispuid.c
$CC -o ../bin/du du.c
$CC -o ../bin/echo echo.c
$CC -o ../bin/env env.c
$CC -o ../bin/factor factor.c
[ "$UNAME" = OpenBSD ] && $CC -I../support -o ../bin/fmtmsg fmtmsg.c ../support/fmtmsg.c
[ "$UNAME" = OpenBSD ] || $CC -o ../bin/fmtmsg fmtmsg.c
$CC -o ../bin/fold fold.c
$CC -o ../bin/getopt getopt.c
$CC -o ../bin/grep grep.c
$CC -o ../bin/head head.c
$CC -o ../bin/hostid hostid.c
$CC -o ../bin/id id.c
$CC -o ../bin/ipcrm ipcrm.c
[ "$UNAME" = Linux ] && $CC -o ../bin/ipcs ipcs.c
$CC -I../support -o ../bin/kill kill.c ../support/signames.c
$CC -I../support -o ../bin/killall killall.c ../support/signames.c
$CC -o ../bin/line line.c
$CC -o ../bin/link link.c
$CC -o ../bin/ln ln.c
$CC -o ../bin/logname logname.c
[ "$UNAME" = OpenBSD ] && $CC -o ../bin/makekey makekey.c
[ "$UNAME" = OpenBSD ] || $CC -o ../bin/makekey makekey.c -lcrypt
$CC -o ../bin/mesg mesg.c
[ "$UNAME" = Linux ] && $CC -I../support -o ../bin/mkdir mkdir.c ../support/setmode.c
[ "$UNAME" = Linux ] || $CC -o ../bin/mkdir mkdir.c
[ "$UNAME" = Linux ] && $CC -I../support -o ../bin/mkfifo mkfifo.c ../support/setmode.c
[ "$UNAME" = Linux ] || $CC -o ../bin/mkfifo mkfifo.c
$CC -o ../bin/mknod mknod.c
$CC -o ../bin/mv mv.c
$CC -o ../bin/newer newer.c
$CC -o ../bin/news news.c
$CC -o ../bin/nice nice.c
$CC -o ../bin/nohup nohup.c
[ "$UNAME" = Linux ] && $CC -I../support -o ../bin/pgrep pgrep.c ../support/signames.c
$CC -o ../bin/printf printf.c
$CC -o ../bin/pwd pwd.c
$CC -o ../bin/random random.c
$CC -o ../bin/readlink readlink.c
$CC -o ../bin/realpath realpath.c
$CC -o ../bin/rename rename.c
$CC -o ../bin/renice renice.c
$CC -o ../bin/rm rm.c
$CC -o ../bin/rmdir rmdir.c
$CC -o ../bin/setpgrp setpgrp.c
$CC -I../support/librfc6234 -o ../bin/sha512 sha2.c -L../lib -lrfc6234
$CC -o ../bin/sleep sleep.c
$CC -o ../bin/split split.c
$CC -o ../bin/sync sync.c
$CC -o ../bin/tac tac.c
$CC -o ../bin/tail tail.c
$CC -o ../bin/tee tee.c
$CC -o ../bin/test test.c
$CC -o ../bin/time time.c
$CC -o ../bin/touch touch.c
$CC -o ../bin/ts ts.c
$CC -o ../bin/tty tty.c
$CC -o ../bin/uname uname.c
$CC -o ../bin/unlink unlink.c
$CC -o ../bin/wc wc.c
[ "$UNAME" = OpenBSD ] || [ "$UNAME" = FreeBSD ] || $CC -o ../bin/who who.c
$CC -o ../bin/which which.c
$CC -o ../bin/yes yes.c
cd ../bin
./chmod +x true false mvdir
./rm -f \[ arch domainname hostname chgrp dirname egrep fgrep groups md5 pkill printenv sha1 sha224 sha256 sha384 sum whoami
./ln -s uname arch
./ln -s uname domainname
./ln -s uname hostname
./ln -s chown chgrp
./ln -s basename dirname
./ln -s grep egrep
./ln -s grep fgrep
./ln -s id groups
./ln -s id whoami
[ "$UNAME" = Linux ] && ./ln -s pgrep pkill
./ln -s env printenv
./ln -s cksum md5
./ln -s cksum sha1
./ln -s cksum sum
./ln -s sha512 sha224
./ln -s sha512 sha256
./ln -s sha512 sha384
./ln -s test \[
