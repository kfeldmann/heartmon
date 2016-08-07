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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include "buffer.h"


int
main ()
{
	int status;
	char_buffer_t *buf
	 = malloc (sizeof (char_buffer_t));

	printf ("==== #010 Creating buffer with size=1024 ====\n");
	if (create_char_buffer (buf, 1024) != 0)
	{ err (errno, "ERROR: create_char_buffer"); }
	printf ("Size of buffer: %zu\n", get_char_buffer_size (buf));
	printf ("Space remaining: %zu\n", buf->space_remaining);
	printf ("Buffer contains: %s\n", get_char_buffer_read_ptr (buf));
	printf ("\n");
	
	while (1)
	{
		printf ("==== #020 Appending 'Hello world.' to buffer  ====\n");
		if (append_to_char_buffer (buf, "Hello world.") != 0)
		{ err (errno, "ERROR: append_to_char_buffer"); }
		printf ("Size of buffer: %zu\n", get_char_buffer_size (buf));
		printf ("Space remaining: %zu\n", buf->space_remaining);
		printf ("Buffer contains: %s\n", get_char_buffer_read_ptr (buf));
		printf ("\n");
	
		printf ("==== #070 Resizing buffer DOWN by 512 ====\n");
		status = resize_char_buffer (buf, -512);
		if (status > 0)
		{ err (errno, "ERROR: resize_char_buffer"); }
		if (status == -1)
		{ printf ("ABORT: resize would lose data (UNexpected).\n"); }
		printf ("Size of buffer: %zu\n", get_char_buffer_size (buf));
		printf ("Space remaining: %zu\n", buf->space_remaining);
		printf ("Buffer contains: %s\n", get_char_buffer_read_ptr (buf));
		printf ("\n");
	
		printf ("==== #080 Appending ' Here is yet another sentance.' to buffer  ====\n");
		if (append_to_char_buffer (buf, " Here is yet another sentance.") != 0)
		{ err (errno, "ERROR: append_to_char_buffer"); }
		printf ("Size of buffer: %zu\n", get_char_buffer_size (buf));
		printf ("Space remaining: %zu\n", buf->space_remaining);
		printf ("Buffer contains: %s\n", get_char_buffer_read_ptr (buf));
		printf ("\n");
	
		printf ("==== #090 Resizing buffer UP by 8192 ====\n");
		status = resize_char_buffer (buf, 8192);
		if (status > 0)
		{ err (errno, "ERROR: resize_char_buffer"); }
		if (status == -1)
		{ printf ("ABORT: resize would lose data (UNexpected).\n"); }
		printf ("Size of buffer: %zu\n", get_char_buffer_size (buf));
		printf ("Space remaining: %zu\n", buf->space_remaining);
		printf ("Buffer contains: %s\n", get_char_buffer_read_ptr (buf));
		printf ("\n");
	
		printf ("==== #100 Appending ' Doodle.' to buffer  ====\n");
		if (append_to_char_buffer (buf, " Doodle.") != 0)
		{ err (errno, "ERROR: append_to_char_buffer"); }
		printf ("Size of buffer: %zu\n", get_char_buffer_size (buf));
		printf ("Space remaining: %zu\n", buf->space_remaining);
		printf ("Buffer contains: %s\n", get_char_buffer_read_ptr (buf));
		printf ("\n");
	
		printf ("==== #110 Clearing the buffer WITH resize to 1024 ====\n");
		if (clear_char_buffer (buf, 1024) != 0)
		{ err (errno, "ERROR: clear_char_buffer"); }
		printf ("Size of buffer: %zu\n", get_char_buffer_size (buf));
		printf ("Space remaining: %zu\n", buf->space_remaining);
		printf ("Buffer contains: %s\n", get_char_buffer_read_ptr (buf));
		printf ("\n");
	}
	return 0;
}

