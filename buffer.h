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


#ifndef _BUFFER_H_ /* Brackets this whole file */
#define _BUFFER_H_

typedef struct
char_buffer_struct
{
	char *memory;
	size_t size;
	size_t space_remaining;
	char *ptr; /* append pointer */
}
char_buffer_t;

extern int create_char_buffer (char_buffer_t*, size_t);
extern void destroy_char_buffer (char_buffer_t*);
extern int resize_char_buffer (char_buffer_t*, long);
extern int append_to_char_buffer (char_buffer_t*, char*);
extern int read_fd_into_char_buffer (char_buffer_t*, int);
extern int clear_char_buffer (char_buffer_t*, size_t);
extern size_t get_char_buffer_size (char_buffer_t*);
extern size_t get_char_buffer_space (char_buffer_t*);
extern size_t get_char_buffer_contlen (char_buffer_t*);
extern char * get_char_buffer_read_ptr (char_buffer_t*);

#endif /* _BUFFER_H_ Brackets this whole file */

