# Copyright (c) 2016, Kris Feldmann
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
#   1. Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
# 
#   2. Redistributions in binary form must reproduce the above
#      copyright notice, this list of conditions and the following
#      disclaimer in the documentation and/or other materials provided
#      with the distribution.
# 
#   3. Neither the name of the copyright holder nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


heartmon :             fifos.o io_select.o buffer.o spawn_process.o heartmon.o
	gcc -g -o heartmon fifos.o io_select.o buffer.o spawn_process.o heartmon.o
heartmon.o :           fifos.h io_select.h buffer.h spawn_process.h heartmon.c
	gcc -g -c heartmon.c

fifos.o : fifos.h fifos.c
	gcc -g -c fifos.c

io_select.o : io_select.h io_select.c
	gcc -g -c io_select.c

buffer.o : buffer.h buffer.c
	gcc -g -c buffer.c

spawn_process.o : spawn_process.h spawn_process.c
	gcc -g -c spawn_process.c


buffer_leak_test : buffer.o buffer_leak_test.o
	gcc -g -o buffer_leak_test buffer.o buffer_leak_test.o
buffer_leak_test.o : buffer.h buffer.c buffer_leak_test.c
	gcc -g -c buffer_leak_test.c

buffer_test : buffer.o buffer_test.o
	gcc -g -o buffer_test buffer.o buffer_test.o
buffer_test.o : buffer.h buffer.c buffer_test.c
	gcc -g -c buffer_test.c

# argtest : argtest.o
# 	gcc -g -o argtest argtest.o
# argtest.o: argtest.c
# 	gcc -g -c argtest.c

# errtest : errtest.o
# 	gcc -g -o errtest errtest.o
# errtest.o : errtest.c
# 	gcc -g -c errtest.c

# fortest : fortest.o
# 	gcc -g -o fortest fortest.o
# fortest.o : fortest.c
# 	gcc -g -c fortest.c

# malloctest : malloctest.o
# 	gcc -g -o malloctest malloctest.o
# malloctest.o : malloctest.c
# 	gcc -g -c malloctest.c

.PHONY : clean
clean:
	rm -f *.o
	rm -f heartmon
	# rm -rf heartmon.dSYM 2>/dev/null
	rm -f buffer_leak_test
	# rm -rf buffer_leak_test.dSYM 2>/dev/null
	rm -f buffer_test
	# rm -rf buffer_test.dSYM 2>/dev/null
	# rm -f argtest
	# rm -rf argtest.dSYM 2>/dev/null
	# rm -f errtest
	# rm -rf errtest.dSYM 2>/dev/null
	# rm -f fortest
	# rm -rf fortest.dSYM 2>/dev/null
	# rm -f malloctest
	# rm -rf malloctest.dSYM 2>/dev/null
