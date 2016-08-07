/*
** Copyright (c) 2016, Kris Feldmann
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 
**   1. Redistributions of source code must retain the above copyright
**      notice, this list of conditions and the following disclaimer.
** 
**   2. Redistributions in binary form must reproduce the above
**      copyright notice, this list of conditions and the following
**      disclaimer in the documentation and/or other materials provided
**      with the distribution.
** 
**   3. Neither the name of the copyright holder nor the names of its
**      contributors may be used to endorse or promote products derived
**      from this software without specific prior written permission.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
** TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
** PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
** OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <unistd.h> /* pipe, pid_t, fork, exec */
#include <stdlib.h> /* exit */
#include "spawn_process.h"


/**********************************************************************
** spawn_process (int*,int*,int*,char*const*)
** 
** Create pipes for stdin, stdout, stderr, and store them
** in the int* arrays (e.g.  int app_stdout[2]).
** Set int* to NULL for any pipes you don't need.
** 
** Fork() and exec() the application process described in the
** char*const* array (an argv[] array for execv).
** 
** Return values:
**   Success:
**     The pid of the child process
**   Failure:
**     -1
**     Global var errno will contain the relevant error code
*/
pid_t
spawn_process (int *app_stdin, int *app_stdout, int *app_stderr,
 char *const *app_argv)
{
	pid_t apppid;

	if (app_stdin && pipe (app_stdin)) return -1;
	if (app_stdout && pipe (app_stdout)) return -1;
	if (app_stderr && pipe (app_stderr)) return -1;

	apppid = fork ();
	if (apppid == -1) return -1;
	if (apppid == 0) /* child */
	{
		if (app_stdin)
		{
			dup2 (app_stdin[READ_END], 0);
			close (app_stdin[WRITE_END]);
		}
		if (app_stdout)
		{
			dup2 (app_stdout[WRITE_END], 1);
			close (app_stdout[READ_END]);
		}
		if (app_stderr)
		{
			dup2 (app_stderr[WRITE_END], 2);
			close (app_stderr[READ_END]);
		}

		if (execv (app_argv[0], app_argv) == -1) return -1;

	} else { /* parent */
		if (app_stdin) close (app_stdin[READ_END]);
		if (app_stdout) close (app_stdout[WRITE_END]);
		if (app_stderr) close (app_stderr[WRITE_END]);
	}
	return apppid;
}

