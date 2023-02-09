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

#ifndef __SVR4__
#error Please use mksignames to generate an appropriate file for your system.
#endif

#ifndef H_SIGNAMES
#define H_SIGNAMES
static const char *signals[]={
 NULL,
 "HUP",
 "INT",
 "QUIT",
 "ILL",
 "TRAP",
 "ABRT",
 "EMT",
 "FPE",
 "KILL",
 "BUS",
 "SEGV",
 "SYS",
 "PIPE",
 "ALRM",
 "TERM",
 "USR1",
 "USR2",
 "CLD",
 "PWR",
 "WINCH",
 "URG",
 "POLL",
 "STOP",
 "TSTP",
 "CONT",
 "TTIN",
 "TTOU",
 "VTALRM",
 "PROF",
 "XCPU",
 "XFSZ"
};
#endif /* H_SIGNAMES */
