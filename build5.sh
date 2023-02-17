#!/bin/ksh
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

# Build system for ancient SVR4 unices.
# Do not use this on Linux or BSD.
#
# This assumes you at least have ksh88...some systems have a braindamaged
# Bourne shell.  Even with ksh88, test sometimes chokes on -z, so you'll have
# to manually edit the line here that sets the "CC" variable.
#   ...and yes, you NEED gcc on these unices.  The system C compiler is just
#   braindead, no two ways about it.

[ "$0" = "../build5.sh" ] && {
 cd ..
 exec ./build5.sh
}

[ "`basename "$0"`" = "build5.sh" ] || {
 echo "Cowardly refusing to run under a name other than build5.sh."
 exit 1
}

UNAME="`uname -s`"
[ "$UNAME" != Linux -a "$UNAME" != "OpenBSD" -a "$UNAME" != "NetBSD" -a "$UNAME" != "FreeBSD" ] || {
 echo "Please use build.sh"
 exit 1
}

[ -n "$1" ] && [ "$1" = "clean" ] && {
 rm -f support/signames.h
 rm -rf bin obj lib
 exit 0
}

CC=gcc

cp support/signames_svr4.h support/signames.h

mkdir bin 2>/dev/null
mkdir obj 2>/dev/null
mkdir lib 2>/dev/null
cd support/libregex
$CC -D__SVR4__ -c -o ../../obj/regcomp.o regcomp.c
$CC -D__SVR4__ -c -o ../../obj/regerror.o regerror.c
$CC -D__SVR4__ -c -o ../../obj/regexec.o regexec.c
$CC -D__SVR4__ -c -o ../../obj/regfree.o regfree.c
$CC -D__SVR4__ -c -o ../../obj/regsub.o regsub.c
cd ../../obj
ar r ../lib/libregex.a regcomp.o regerror.o regexec.o regfree.o regsub.o
cd ../src
cp true.sh ../bin/true
cp false.sh ../bin/false
cp mvdir.sh ../bin/mvdir
$CC -D__SVR4__ -o ../bin/asa asa.c
$CC -D__SVR4__ -o ../bin/banner banner.c
$CC -D__SVR4__ -I../support -o ../bin/basename basename.c ../support/base_dirname.c
$CC -D__SVR4__ -o ../bin/cal cal.c
$CC -D__SVR4__ -o ../bin/cat cat.c
$CC -D__SVR4__ -I../support -o ../bin/chmod chmod.c ../support/setmode.c
$CC -D__SVR4__ -o ../bin/chown chown.c
$CC -D__SVR4__ -o ../bin/chroot chroot.c
$CC -D__SVR4__ -o ../bin/cksum cksum.c
$CC -D__SVR4__ -o ../bin/cmp cmp.c
$CC -D__SVR4__ -o ../bin/date date.c
$CC -D__SVR4__ -o ../bin/dd dd.c
$CC -D__SVR4__ -I../support -o ../bin/comm comm.c ../support/getline.c
$CC -D__SVR4__ -o ../bin/dircmp dircmp.c
$CC -D__SVR4__ -o ../bin/dispgid dispgid.c
$CC -D__SVR4__ -o ../bin/dispuid dispuid.c
$CC -D__SVR4__ -o ../bin/du du.c
$CC -D__SVR4__ -o ../bin/echo echo.c
$CC -D__SVR4__ -o ../bin/env env.c
$CC -D__SVR4__ -o ../bin/factor factor.c
$CC -D__SVR4__ -o ../bin/fmtmsg fmtmsg.c
$CC -D__SVR4__ -I../support -o ../bin/fold fold.c ../support/getline.c
$CC -D__SVR4__ -o ../bin/getopt getopt.c
$CC -D__SVR4__ -I../support -I../support/libregex -o ../bin/grep grep.c ../support/getline.c -L../lib -lregex
$CC -D__SVR4__ -I../support -o ../bin/head head.c ../support/getline.c
$CC -D__SVR4__ -I../support -o ../bin/id id.c ../support/getgrouplist.c
$CC -D__SVR4__ -o ../bin/hostid hostid.c -lucb
$CC -D__SVR4__ -o ../bin/ipcrm ipcrm.c
$CC -D__SVR4__ -I../support -o ../bin/kill kill.c ../support/signames.c
$CC -D__SVR4__ -I../support -o ../bin/killall killall.c ../support/signames.c
$CC -D__SVR4__ -o ../bin/line line.c
$CC -D__SVR4__ -o ../bin/link link.c
$CC -D__SVR4__ -o ../bin/ln ln.c
$CC -D__SVR4__ -o ../bin/logname logname.c
$CC -D__SVR4__ -o ../bin/makekey makekey.c
$CC -D__SVR4__ -o ../bin/mesg mesg.c
$CC -D__SVR4__ -I../support -o ../bin/mkdir mkdir.c ../support/setmode.c
$CC -D__SVR4__ -I../support -o ../bin/mkfifo mkfifo.c ../support/setmode.c
$CC -D__SVR4__ -o ../bin/mknod mknod.c
$CC -D__SVR4__ -o ../bin/mv mv.c
$CC -D__SVR4__ -o ../bin/newer newer.c
$CC -D__SVR4__ -o ../bin/news news.c
$CC -D__SVR4__ -o ../bin/nice nice.c
$CC -D__SVR4__ -o ../bin/nohup nohup.c
$CC -D__SVR4__ -o ../bin/printf printf.c
$CC -D__SVR4__ -o ../bin/pwd pwd.c
$CC -D__SVR4__ -o ../bin/random random.c
$CC -D__SVR4__ -o ../bin/readlink readlink.c
$CC -D__SVR4__ -o ../bin/realpath realpath.c
$CC -D__SVR4__ -o ../bin/rename rename.c
$CC -D__SVR4__ -o ../bin/renice renice.c -lucb
$CC -D__SVR4__ -o ../bin/rm rm.c
$CC -D__SVR4__ -o ../bin/rmdir rmdir.c
$CC -D__SVR4__ -o ../bin/setpgrp setpgrp.c
$CC -D__SVR4__ -o ../bin/sleep sleep.c
$CC -D__SVR4__ -I../support -o ../bin/split split.c ../support/getline.c
$CC -D__SVR4__ -o ../bin/sync sync.c
$CC -D__SVR4__ -o ../bin/tac tac.c
$CC -D__SVR4__ -o ../bin/tail tail.c
$CC -D__SVR4__ -o ../bin/tee tee.c
$CC -D__SVR4__ -o ../bin/test test.c
$CC -D__SVR4__ -o ../bin/time time.c
$CC -D__SVR4__ -o ../bin/tty tty.c
$CC -D__SVR4__ -o ../bin/uname uname.c -lucb
$CC -D__SVR4__ -o ../bin/unlink unlink.c
$CC -D__SVR4__ -o ../bin/wc wc.c
$CC -D__SVR4__ -o ../bin/which which.c
$CC -D__SVR4__ -o ../bin/who who.c
$CC -D__SVR4__ -o ../bin/yes yes.c
cd ../bin
./chmod +x true false mvdir
./rm -f \[ arch hostname chgrp dirname egrep fgrep groups md5 printenv sha1 sum whoami
./ln -s uname arch
./ln -s uname hostname
./ln -s chown chgrp
./ln -s basename dirname
./ln -s grep egrep
./ln -s grep fgrep
./ln -s id groups
./ln -s id whoami
./ln -s env printenv
./ln -s cksum md5
./ln -s cksum sha1
./ln -s cksum sum
./ln -s test \[
