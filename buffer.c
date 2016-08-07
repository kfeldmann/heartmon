/*
**
** An appendable, resizable character buffer.
** 
** KPF - 2014-07-26
** 
** create_char_buffer         (char_buffer_t *bufptr, size_t size)
** destroy_char_buffer        (char_buffer_t *bufptr)
** resize_char_buffer         (char_buffer_t *bufptr, long  sizedelta)
** append_to_char_buffer      (char_buffer_t *bufptr, char *message)
** read_fd_into_char_buffer   (char_buffer_t *bufptr, int fd)
** clear_char_buffer          (char_buffer_t *bufptr, size_t resize_to)
** get_char_buffer_size       (char_buffer_t *bufptr)
** get_char_buffer_space      (char_buffer_t *bufptr)
** get_char_buffer_contlen    (char_buffer_t *bufptr)
** get_char_buffer_read_ptr   (char_buffer_t *bufptr)
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


// #ifndef _BUFFER_C_ /* Brackets this whole file */
// #define _BUFFER_C_

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
//#include <malloc/malloc.h>
#include <malloc.h>
#include "buffer.h"


/**********************************************************************
** create_char_buffer ()
** 
** The parameter nullobj is a pointer to a null version of whatever
** type will be stored in the buffer. For example, for a character
** buffer, set nullobj to point to '\0'. For an int, use 0, etc.
** 
** Return values:
**   0  success
**   *  errno from malloc failure
*/
int
create_char_buffer (char_buffer_t *bufptr, size_t size)
{
	int status = 0;

	if (bufptr == NULL)
	{
		status = errno = EFAULT;
		goto end;
	}
	if (bufptr->memory != NULL) free (bufptr->memory);
	bufptr->memory = (char *)malloc (size);
	if (bufptr->memory == NULL) status = errno;
	else
	{
		*(bufptr->memory) = '\0';
		bufptr->ptr = bufptr->memory;
		bufptr->size = size;
		bufptr->space_remaining = size;
	}
end:
	return status;
}


/**********************************************************************
** destroy_char_buffer ()
*/
void
destroy_char_buffer (char_buffer_t *bufptr)
{
	if (bufptr != NULL)
	{
		if (bufptr->memory != NULL)
		{
			free (bufptr->memory);
		}
	}
}


/**********************************************************************
** resize_char_buffer ()
**
** Resize a buffer by sizedelta units. Use a negative sizedelta value
** to shrink the buffer. If shrinking, the value of sizedelta and
** space_remaining are compared. If data would be lost, the return
** value will be set to -1 and no change will be made.
** 
** Return values:
**  -1  requested shrink would cause data loss
**   0  success
**   *  errno from malloc failure
*/
int
resize_char_buffer (char_buffer_t *bufptr, long sizedelta)
{
	int status = 0;
	size_t newsize;
	size_t offset;

	if (!sizedelta) goto end;
	if (sizedelta < 0)
	{
		/* we can only shrink remaining space, not used space */
		if ((long)(bufptr->space_remaining) + sizedelta < 0)
		{
			status = -1;
			goto end;
		}
	}
	newsize = (size_t)((long)(bufptr->size) + sizedelta);
	offset = bufptr->ptr - bufptr->memory;

	bufptr->memory = (char *)realloc (bufptr->memory, newsize);
	if (bufptr->memory == NULL) status = errno;
	else
	{
		bufptr->size = newsize;
		bufptr->space_remaining += sizedelta;
		bufptr->ptr = bufptr->memory + offset;
	}
end:
	return status;
}


/**********************************************************************
** append_to_char_buffer ()
** 
** Return values:
**   0        success
**   ENOBUFS  not enough space remaining in the buffer
*/
int
append_to_char_buffer (char_buffer_t *bufptr, char *message)
{
	int status = 0;
	int slen = strlen (message);

	if (slen > bufptr->space_remaining)
	{
		status = errno = ENOBUFS;
	} else {
		bufptr->ptr = stpcpy (bufptr->ptr, message);
		bufptr->space_remaining -= slen;
	}
	return status;
}


/**********************************************************************
** read_fd_into_char_buffer ()
** 
** Return values:
**   0   success
**   *   errno from read()
*/
int
read_fd_into_char_buffer (char_buffer_t *bufptr, int fd)
{
	int status = 0;
	int readbytes;
	readbytes = read (fd, bufptr->ptr, bufptr->space_remaining);
	if (readbytes > 0)
	{
		bufptr->space_remaining -= readbytes;
		bufptr->ptr += readbytes;
		*(bufptr->ptr) = '\0';
	} else if (readbytes == -1) status = errno;
	return status;
}


/**********************************************************************
** clear_char_buffer ()
** 
** If resize_to is not 0, the buffer will be resized to resize_to
** bytes after clearing.
** 
** Return values:
**   0  success
**   *  errno from realloc failure
*/
int
clear_char_buffer (char_buffer_t *bufptr, size_t resize_to)
{
	int status = 0;

	if (resize_to > 0 && resize_to != bufptr->size)
	{
		bufptr->memory = (char *)realloc (bufptr->memory, resize_to);
		if (bufptr->memory == NULL)
		{
			status = errno;
			goto end;
		} else {
			bufptr->size = resize_to;
		}
	}
	*(bufptr->memory) = '\0';
	bufptr->ptr = bufptr->memory;
	bufptr->space_remaining = bufptr->size;
end:
	return status;
}


/**********************************************************************
** get_char_buffer_size ()
*/
size_t
get_char_buffer_size (char_buffer_t *bufptr)
{
	return bufptr->size;
}


/**********************************************************************
** get_char_buffer_space ()
*/
size_t
get_char_buffer_space (char_buffer_t *bufptr)
{
	return bufptr->space_remaining;
}


/**********************************************************************
** get_char_buffer_contlen ()
*/
size_t
get_char_buffer_contlen (char_buffer_t *bufptr)
{
	return bufptr->size - bufptr->space_remaining;
}


/**********************************************************************
** get_char_buffer_read_ptr ()
*/
char *
get_char_buffer_read_ptr (char_buffer_t *bufptr)
{
	return bufptr->memory;
}


// #endif /* _BUFFER_C_ Brackets this whole file */

