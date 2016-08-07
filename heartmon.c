/*
**
** Heartmon: An application heartbeat monitor.
** 
** KPF - 2014-06-15
**
** 2014-07-25 - dynamic argv buffers, rather than static
** 2014-07-28 - appendable, growable/shrinkable buffer (buffer.c)
**            - select timeout working now, removed sleep
**            - syslogging of heartmon's own output
** 2014-07-30 - implementation of searching the buffer for heartbeat,
**              heartbeat timers
**            - killing of app & logger based on proc status and
**              heartbeat timers
** 2014-08-06
**            - Starting work on v0.3
** 2014-08-13
**            - grace period is equal to the least of the non-zero
**              thresholds
** 2014-08-17
**            - added spawn_process.h & .c
** 2014-08-21
**            - added buffer.h
** 2014-08-23
**            - added io_select.h & .c
**            - added shutdown_handler() and registered with atexit()
** 2014-08-24
**            - added fifos.h & .c
**            - added fifo handling
**            - added signal handling for SIGTERM
**            - added killing of app and log hander in shutdown_handler()
** 
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


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <sys/wait.h>
#include <time.h>
//#include <malloc/malloc.h>
#include <malloc.h>
#include <signal.h>
#include "fifos.h"
#include "io_select.h"
#include "buffer.h"
#include "spawn_process.h"

#define MAXSTRLEN 128
#define MAXARGS 64
#define MAXFDS 32
#define BUFFERSIZE 8192

#define SELECT_TIMEOUT_SEC 1

#define SYSLOG_IDENT "heartmon"

/* globals */
pid_t apppid = -1;
pid_t logpid = -1;


/**********************************************************************
** usage ()
** 
** Print usage statement to STDERR and exit.
** The value of `appname' should be the `argv[0]' name of
** this executable.
** 
** Causes exit unconditionally.
*/
void
usage (char *appname)
{
	fprintf (stderr,
	 "Usage: %s -d heartmon_config_directory \\\n", appname);
	fprintf (stderr,
	 "       [-i include_filter] [-e exclude_filter] \\\n");
	fprintf (stderr,
	 "       [-w warn_seconds] [-c crit_seconds] [-r restart_seconds]\n");
	exit (EXIT_FAILURE);
}


/**********************************************************************
** set_str_optarg ()
** 
** Copy value of global variable `optarg' into string `var'.
** The `var_name' string is used in the error messaging.
** 
** Causes exit if `optarg' contains a string longer than MAXSTRLEN.
*/
void
set_str_optarg (char *var, const char *var_name) /* causes exit on failure */
{
	/* char *optarg - a global from unistd.h */
	/* MAXSTRLEN - a global constant defined in the top of this file */

	if (strlen (optarg) < MAXSTRLEN) strcpy (var, optarg);
	else
	{
		syslog (LOG_ALERT, "%s length cannot be longer than %d bytes.",
		 var_name, MAXSTRLEN - 1 );
		exit (EXIT_FAILURE);
	}
}


/**********************************************************************
** get_file_contents ()
** 
** Open a file and read its entire contents into *content.
** Intended for reading small files such as those specifying
** single environment variables, single command arguments, etc.
** 
** Returns 0 on success, or the value of `errno' on failure.
*/
int
get_file_contents (const char *path, const char *fname, char **content)
{
	char *fpath;
	FILE *fh;
	int status = 0; /* 0 = success */
	int len = 0;
	char *buf;
	/* MAXSTRLEN - a global constant defined in the top of this file */

	fpath = malloc (strlen (path) + strlen (fname) + 1);
	sprintf (fpath, "%s/%s", path, fname);

	buf = malloc (MAXSTRLEN);

	if ((fh = fopen (fpath, "r")) != NULL)
	{
		len = fread (buf, sizeof (char), MAXSTRLEN, fh);
		fclose (fh);
	} else {
		status = errno;
	}
	if (len > 0)
	{
		/* get rid of the newline, and a carriage return if there is one */
		if (buf[len - 1] == '\n') buf[len - 1]='\0';
		if (buf[len - 2] == '\r') buf[len - 2]='\0';

		*content = malloc (len);
		strncpy (*content, buf, len);
	}

	free (fpath);
	free (buf);
	return status;
}


/**********************************************************************
** get_config ()
** 
** Read *sub section of configuration directory, populating argv[].
** Returns the number of args found.
*/
int
get_config (const char *root, const char *sub, char *argv[])
{
	char *path;
	char fname[3];
	int i;
	/* MAXARGS - a global constant defined in the top of this file */

	path = malloc (strlen (root) + strlen (sub) + 1);
	sprintf (path, "%s/%s", root, sub);

	for (i = 0; i < MAXARGS; i++)
	{
		sprintf (fname, "%d", i);
		if (get_file_contents (path, fname, &argv[i]) != 0) break;
	}
	free (path);
	return i;
}


/**********************************************************************
** contains_heartbeat ()
** 
** Use in_filter and ex_filter to examine the contents of ls_buffer
** line by line to see if a heartbeat is found.
** 
** Returns 1 if a heartbeat is found or 0 if not.
*/ 
int
contains_heartbeat (char_buffer_t *bufptr,
 const char *in_filter, const char *ex_filter)
{
	int status = 0;
	char *line, *content, *to_free;

	/* If no filters are specified, then any bytes in
	   the buffer qualify as a heartbeat. */
	if (*in_filter == '\0' && *ex_filter == '\0'
	 && get_char_buffer_contlen (bufptr) > 0)
	{
		status = 1;
		goto end;
	}
	
	/* Examine each line in the buffer separately. If any line
	   qualifies as a heartbeat, return true (1). */
	content = to_free = strdup (get_char_buffer_read_ptr (bufptr));
	while ((line = strsep (&content, "\n")) != NULL)
	{
		if (*ex_filter != '\0')
		{
			if (strstr (line, ex_filter) != NULL)
			{ continue; }
		}
		if (*in_filter != '\0')
		{
			if (strstr (line, in_filter) != NULL)
			{
				status = 1;
				break;
			}
		}
	}
	if (to_free) free (to_free);
end:
	return status;
}


/**********************************************************************
** min_non0_of3 ()
*/ 
int
min_non0_of3 (int a, int b, int c)
{
	int m;
	if (a == 0 && b == 0 && c == 0) return 0;
	if (a != 0) m = a;
	else if (b != 0) m = b;
	else if (c != 0) m = c;
	if (a != 0 && a < m) m = a;
	if (b != 0 && b < m) m = b;
	if (c != 0 && c < m) m = c;
	return m;
}


/**********************************************************************
** shutdown_handler ()
*/ 
void
shutdown_handler ()
{
	int result;
	if (apppid != -1)
	{
		result = kill (apppid, SIGTERM);
		if (result) syslog (LOG_WARNING,
		 "Killing application process failed: %m");
		else syslog (LOG_INFO, "Stopped application [%d].", apppid);
	}
	if (logpid != -1)
	{
		result = kill (logpid, SIGTERM);
		if (result) syslog (LOG_WARNING,
		 "Killing log handler process failed: %m");
		else syslog (LOG_INFO, "Stopped log handler [%d].", logpid);
	}
	syslog (LOG_INFO, "Stopping heartmon.");
	return ;
}


/**********************************************************************
** sigterm_handler ()
*/ 
void
sigterm_handler (int signo)
{
	if (signo == SIGTERM)
	{
		syslog (LOG_INFO, "Received TERM signal.");
		exit (EXIT_SUCCESS);
	} else {
		syslog (LOG_INFO, "Received signal %d.", signo);
	}
	return ;
}


/**********************************************************************
** main ()
*/ 
int
main (int argc, char *argv[])
{
	/* MAXSTRLEN - preproc define at the top of this file */
	/* MAXARGS - preproc define at the top of this file */
	/* MAXFDS - preproc define at the top of this file */
	/* BUFFERSIZE - preproc define at the top of this file */
	/* SELECT_TIMEOUT_SEC - preproc define at the top of this file */
	/* SYSLOG_IDENT - preproc define at the top of this file */

	int i, j;

	char opt;
	/* char *optarg - global from unistd.h */

	char hm_confdir[MAXSTRLEN];
	char in_filter[MAXSTRLEN];
	char ex_filter[MAXSTRLEN];
	int warn_thresh;
	int crit_thresh;
	int restart_thresh;
	int min_thresh;

	time_t now;
	time_t last_heartbeat;
	int warn_has_been_triggered;
	int crit_has_been_triggered;

	char *app_argv[MAXARGS + 1];
	char *log_argv[MAXARGS + 1];
	char *fifo_list[MAXARGS + 1];
	int argcount;

	int app_stdout[2];
	int app_stderr[2];
	int log_stdin[2];
	int fifo_fds[MAXFDS - 3];
	int readbytes;

	// pid_t apppid - global
	// pid_t logpid - global
	pid_t result;
	int statusinfo;

	int readycount;
	int io_status;
	int fds[MAXFDS];
	fd_set readfds, writefds, errorfds;
	struct timeval timeout;

	void (*shutdown_hdlr_ptr)(void);
	shutdown_hdlr_ptr = &shutdown_handler;

	/* log stream buffer */
	char_buffer_t *ls_buffer = malloc (sizeof (char_buffer_t));
	int resize_status;

	/* syslog settings */
	const char *syslog_ident = SYSLOG_IDENT;
	int syslog_logopt = LOG_CONS | LOG_PERROR | LOG_PID;
	int syslog_facility = LOG_DAEMON;


	openlog (syslog_ident, syslog_logopt, syslog_facility);

	/* initialize vars to zero/null */
	in_filter[0] = '\0';
	ex_filter[0] = '\0';
	hm_confdir[0] = '\0';
	for (i = 0; i < MAXARGS + 1; i++)
	{
		app_argv[i] = NULL;
		log_argv[i] = NULL;
		fifo_list[i] = NULL;
	}
	for (i = 0; i < MAXFDS - 3; i++)
	{
		fifo_fds[i] = -1;
	}
	if (create_char_buffer (ls_buffer, BUFFERSIZE) != 0)
	{
		syslog (LOG_ALERT, "create_char_buffer: %m");
		exit (errno);
	}
	warn_thresh = crit_thresh = restart_thresh = min_thresh = 0;

	while ((opt = getopt (argc, argv, "i:e:w:c:r:d:")) != -1)
	{
		switch (opt)
		{
			case 'i':
				set_str_optarg (in_filter, "include filter");
				break;
			case 'e':
				set_str_optarg (ex_filter, "exclude filter");
				break;
			case 'd':
				set_str_optarg (hm_confdir, "heartmon config directory");
				break;
			case 'w':
				warn_thresh = atoi (optarg);
				break;
			case 'c':
				crit_thresh = atoi (optarg);
				break;
			case 'r':
				restart_thresh = atoi (optarg);
				break;
			default: /* '?' */
				usage (argv[0]);
		}
	}

	if (*hm_confdir == '\0')
	{
		syslog (LOG_ERR, "Heartmon config directory (-d) is required.");
		usage (argv[0]);
	}

	/* process app command-line into app_argv[]. */
	argcount = get_config (hm_confdir, "app", app_argv);
	if (argcount == 0)
	{
		syslog (LOG_ERR,
		 "Application config must have at least arg 0 defined.");
		exit (EXIT_FAILURE);
	}
	
	/* process logger command-line into log_argv[]. */
	argcount = get_config (hm_confdir, "log", log_argv);
	if (argcount == 0)
	{
		syslog (LOG_ERR,
		 "Log handler config must have at least arg 0 defined.");
		exit (EXIT_FAILURE);
	}

	/* process list of fifos, create and open. */
	argcount = get_config (hm_confdir, "fifo", fifo_list);
	if (argcount != 0)
	{
		for (i = 0; i < argcount; i++)
		{
			// syslog (LOG_DEBUG, "Checking for fifo[%d]:[%s]",
			//  i, fifo_list[i]);
			/* Check for existing file at fifo location */
			io_status = is_fifo (fifo_list[i]);
			if (io_status == 0)
			{
				syslog (LOG_ALERT,
				 "Please move existing log file out of the way: %s",
				 fifo_list[i]);
				exit (EXIT_FAILURE);
			}
			else if (io_status == -1) /* no existing file */
			{
				if (make_fifo (fifo_list[i]))
				{
					syslog (LOG_ALERT,
					 "Failed to create fifo [%s]: %m",
					 fifo_list[i]);
					exit (errno);
				}
			}
			/* now we should have an existing or new fifo */
			fifo_fds[i] = open_fifo (fifo_list[i]);
			if (fifo_fds[i] == -1)
			{
				syslog (LOG_ALERT,
				 "Failed to open fifo [%s]: %m",
				 fifo_list[i]);
				exit (errno);
			}
		}
	}

	atexit (shutdown_hdlr_ptr);
	if (signal (SIGTERM, sigterm_handler) == SIG_ERR)
	{ syslog (LOG_WARNING, "Cannot catch SIGTERM."); }

	syslog (LOG_INFO, "======== STARTUP ========");

	/* spawn the logger process */
	logpid = spawn_process (log_stdin, NULL, NULL, log_argv);
	if (logpid == -1)
	{
		syslog (LOG_ALERT, "Failed to start log handler: %m");
		exit (errno);
	}
	syslog (LOG_NOTICE, "Started log handler [%d]: %s",
	 logpid, log_argv[0]);

	/* spawn the application process */
	apppid = spawn_process (NULL, app_stdout, app_stderr, app_argv);
	if (apppid == -1)
	{
		syslog (LOG_ALERT, "Failed to start application: %m");
		exit (errno);
	}
	syslog (LOG_NOTICE, "Started application [%d]: %s",
	 apppid, app_argv[0]);

	/*
	** Give a startup grace period equal to min_thresh,
	** so the first warning will come no earlier than
	** min_thresh * 2.
	*/
	min_thresh = min_non0_of3 (warn_thresh, crit_thresh,
	 restart_thresh);
	last_heartbeat = time (NULL) + min_thresh;
	warn_has_been_triggered = 0;
	crit_has_been_triggered = 0;

	/* main loop: read from app and write to log handler */
	while (1)
	{
		/* check for terminated app process. restart if needed */
		result = waitpid (apppid, &statusinfo, WNOHANG);
		if (result == -1)
		{
			syslog (LOG_ERR, "Problem checking status of app process: %m");
			/* kill the app process and restart it */
			if (kill (apppid, SIGKILL) != 0)
			{
				syslog (LOG_ALERT,
				 "kill(apppid,SIGKILL) failed: %m");
				exit (errno || EXIT_FAILURE);
			}
			apppid = spawn_process (NULL, app_stdout, app_stderr, app_argv);
			if (apppid == -1)
			{
				syslog (LOG_ALERT, "Failed to start application: %m");
				exit (errno);
			}
			syslog (LOG_NOTICE, "Started application [%d]: %s",
			 apppid, app_argv[0]);
			last_heartbeat = time (NULL) + min_thresh;
			warn_has_been_triggered = 0;
			crit_has_been_triggered = 0;
		}
		else if (result != 0)
		{
			syslog (LOG_ERR, "Application has terminated unexpectedly.");
			/* restart it */
			apppid = spawn_process (NULL, app_stdout, app_stderr, app_argv);
			if (apppid == -1)
			{
				syslog (LOG_ALERT, "Failed to start application: %m");
				exit (errno);
			}
			syslog (LOG_NOTICE, "Started application [%d]: %s",
			 apppid, app_argv[0]);
			last_heartbeat = time (NULL) + min_thresh;
			warn_has_been_triggered = 0;
			crit_has_been_triggered = 0;
		}

		/* shrink log stream buffer if needed */
		if (get_char_buffer_space (ls_buffer) >= BUFFERSIZE * 2)
		{
			resize_status = resize_char_buffer (ls_buffer, -1 * BUFFERSIZE);
			if (resize_status > 0)
			{
				syslog (LOG_ALERT, "resize_char_buffer: %m");
				exit (errno);
			}
			if (resize_status == -1)
			{
				syslog (LOG_ERR,
				 "Buffer resize would lose data (although we checked first).");
			}
		}

		/* extend log stream buffer if needed */
		if (get_char_buffer_space (ls_buffer) <= BUFFERSIZE / 2)
		{
			resize_status = resize_char_buffer (ls_buffer, BUFFERSIZE);
			if (resize_status > 0)
			{
				syslog (LOG_ALERT, "resize_char_buffer: %m");
				exit (errno);
			}
			if (resize_status == -1)
			{
				syslog (LOG_ERR,
	 "Buffer resize would lose data (although we asked to grow, not shrink).");
			}
		}

		/* Read from readable file descriptors */
		i = 0;
		fds[i++] = app_stdout[READ_END];
		fds[i++] = app_stderr[READ_END];
		for (j = 0; j < MAXFDS - 3; j++)
		{
			if (fifo_fds[j] != -1) fds[i++] = fifo_fds[j];
		}

		io_status = read_readable (fds, i, SELECT_TIMEOUT_SEC, ls_buffer);
		if (io_status == -1)
		{
			syslog (LOG_ERR,
			"Failed to read from app. Select() in read_readable() said: %m");
		}

		/* check for heartbeat, using in_filter, ex_filter */
		if (warn_thresh == 0 && crit_thresh == 0
		 && restart_thresh == 0) goto nohealthchecking;
		now = time(NULL);
		if (contains_heartbeat (ls_buffer, in_filter, ex_filter))
		{
			// syslog (LOG_DEBUG, "found healthcheck");
			if (warn_has_been_triggered || crit_has_been_triggered)
			{ syslog (LOG_NOTICE, "Heartbeat detected. Resetting timers."); }
			last_heartbeat = now;
			warn_has_been_triggered = 0;
			crit_has_been_triggered = 0;
		} else {
			// syslog (LOG_DEBUG, "found NO healthcheck. delta t = %ld",
			// now - last_heartbeat);
			// syslog (LOG_DEBUG, "last heartbeat = %ld", last_heartbeat);
			if (!(warn_has_been_triggered) && warn_thresh != 0
			  && now - last_heartbeat >= warn_thresh)
			{
				syslog (LOG_WARNING,
				 "Heartbeat warning threshold reached for %s",
				 app_argv[0]);
				warn_has_been_triggered = 1;
			}
			if (!(crit_has_been_triggered) && crit_thresh != 0
			  && now - last_heartbeat >= crit_thresh)
			{
				syslog (LOG_ERR,
				 "Heartbeat critical threshold reached for %s",
				 app_argv[0]);
				crit_has_been_triggered = 1;
			}
			if (restart_thresh != 0
			 && now - last_heartbeat >= restart_thresh)
			{
				syslog (LOG_ERR,
				 "KILLING APP: Heartbeat restart threshold reached for %s.",
				 app_argv[0]);
				if (kill (apppid, SIGKILL) != 0)
				{
					syslog (LOG_ALERT,
					 "kill(apppid,SIGKILL) failed: %m");
					exit (errno || EXIT_FAILURE);
				}
				apppid = spawn_process (NULL, app_stdout, app_stderr, app_argv);
				if (apppid == -1)
				{
					syslog (LOG_ALERT, "Failed to start application: %m");
					exit (errno);
				}
				syslog (LOG_NOTICE, "Started application [%d]: %s",
				 apppid, app_argv[0]);
				last_heartbeat = time (NULL) + min_thresh;
				warn_has_been_triggered = 0;
				crit_has_been_triggered = 0;
			}
		} /* else (not contains_heartbeat) */
nohealthchecking:

		/* check for terminated logger process. restart if needed */
		result = waitpid (logpid, &statusinfo, WNOHANG);
		if (result == -1)
		{
			syslog (LOG_ERR,
			 "Problem checking status of logger process: %m");
			/* kill the logger process and restart it */
			if (kill (logpid, SIGKILL) != 0)
			{
				syslog (LOG_ALERT, "kill(logpid,SIGKILL) failed: %m");
				exit (errno || EXIT_FAILURE);
			}
			logpid = spawn_process (log_stdin, NULL, NULL, log_argv);
			if (logpid == -1)
			{
				syslog (LOG_ALERT, "Failed to start log handler: %m");
				exit (errno);
			}
			syslog (LOG_NOTICE, "Started log handler [%d]: %s",
			 logpid, log_argv[0]);
		}
		else if (result != 0)
		{
			syslog (LOG_ERR, "Log handler has terminated unexpectedly.");
			logpid = spawn_process (log_stdin, NULL, NULL, log_argv);
			if (logpid == -1)
			{
				syslog (LOG_ALERT, "Failed to start log handler: %m");
				exit (errno);
			}
			syslog (LOG_NOTICE, "Started log handler [%d]: %s",
			 logpid, log_argv[0]);
		}

		/* write buffer to logging file descriptor if writable */
		i = 0;
		fds[i++] = log_stdin[WRITE_END];
		io_status = write_writable (fds, i, SELECT_TIMEOUT_SEC, ls_buffer);
		if (io_status == -1)
		{
			syslog (LOG_ERR,
	"Failed to write to log handler. Select() in write_writable() said: %m");
		}
		else if (io_status == -2)
		{ syslog (LOG_ERR, "Wrote partial data to log handler."); }
		else
		{
			if (clear_char_buffer (ls_buffer, 0) != 0)
			{
				syslog (LOG_ALERT, "clear_char_buffer: %m");
				exit (errno);
			}
		}
	} /* while loop */

	exit (EXIT_SUCCESS);
}

