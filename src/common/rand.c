/*
 * Copyright (c) 2015 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 * Copyright (c) 2015 Holly Sipek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <nomad/types.h>
#include <nomad/rand.h>
#include <nomad/config.h>
#include <nomad/error.h>

uint32_t rand32(void)
{
#ifdef HAVE_ARC4RANDOM
	return arc4random();
#else
	uint32_t ret;
	int fd;

	fd = open("/dev/random", O_RDONLY);
	if (fd == -1)
		panic("Failed to get random number: %s", strerror(errno));

	if (read(fd, &ret, sizeof(ret)) != sizeof(ret))
		panic("Failed to get random number: %s", strerror(errno));

	close(fd);

	return ret;
#endif
}

uint64_t rand64(void)
{
	uint64_t tmp;

	tmp   = rand32();
	tmp <<= 32;
	tmp  |= rand32();

	return tmp;
}
