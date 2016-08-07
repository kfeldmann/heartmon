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


#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "fifos.h"


/**********************************************************************
** is_fifo (const char*)
** 
** Determines whether the file at *fpath is a fifo or not. Does not
** follow symlinks.
** 
** Return value:
**   0 if the file is not a fifo
**   1 if the file is a fifo
**  -1 if an error occurs. The global var errno will contain
**     the relevant error code.
*/
int
is_fifo (const char *fpath)
{
	struct stat *statinfo;
	int result = 0;

	statinfo = malloc (sizeof(struct stat));
	if (lstat (fpath, statinfo) != 0) result = -1;
	else if (S_ISFIFO (statinfo->st_mode)) result = 1;
	free (statinfo);
	return result;
}


/**********************************************************************
** make_fifo (const char*)
** 
** Create a fifo at *fpath.
** 
** Return value:
**   0 if successful
**  -1 upon failure. An error code is stored in errno.
*/
int
make_fifo (const char *fpath)
{
	return (mkfifo (fpath, (mode_t) 0600));
}


/**********************************************************************
** open_fifo (const char*)
** 
** Opens the fifo (or file) at *fpath for reading, non-blocking.
** 
** Return value:
**   -1 upon failure
**    * file descriptor number if successful
*/
int
open_fifo (const char *fpath)
{
	return open (fpath, O_RDONLY | O_NONBLOCK);
}

